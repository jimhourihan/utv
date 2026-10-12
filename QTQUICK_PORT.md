# QtQuick Port

## Ultimate Goal

RV's main window will become some variant of a QtQuick window. The HUD
elements of its UI will become QML components replacing the existing MuGL
UI (timeline, timeline magnifier, info, pixel inspector, etc). RV will
still allow the use of Qt widgets via scripting but future development will
ideally use QML.

Stretch goals:

1. Port to Vulkan (and/or Metal?) (high priority)
1. Contunue to support MuGL via emulation layer (low priority)

Contraints:

1. Maintain backwards compatibility with user plugins/scripts/packages and
   the session file format EXCLUDING the current ability of the user to add
   scripting code to render in the same context as the IPCore renderer
   (i.e. no HUD widgets unless they're QML or somehow tied directly to the
   graph).
1. Remote Sync should still work (as in peer-to-peer RV syncing)
1. Continue to use GLSL as the dominant GPU language (new additions are OK)
1. Playback performance must either improve or be equivalent to the
   existing code for > ~98% of use cases. If there is a seldom used edge
   case the suffers that is negotiable. Ideally decisions that improve
   performance over simplicity should be weighed as preferred.

Unshackling:

1. Its possible that the resulting changes causes the creation of a new
   "next-gen" product. That is OK, but ideally the path to adoption would
   minimize porting work for the users and the new product plays nice with
   its antecedent.

## Sub-Goals

1. Take advantage of QtQuick's seperation of event and rendering threads
1. Seperate graph evaluation and rendering into different
   "stages". Possibly cache IPImage or other graph products for later use.
1. Use the IPCoreCommands to defer state changes in the graph in
   conjunction with QtQuicks event/rendering threads.
1. Optionally use IPCoreCommands for undo/redo (a new feature). (Not high
   priority compared to backwards compatibility.)
1. Completely modularize UI, scripting, and IP (core image process).
1. Facilite the use of TextureOutputGroupIPNode in the graph to make QML
   items with "live" views into the graph.

## Decisions

1. Which type of main window (QQuickWindw/QMainWindow or whatever) should
   be used and what backwards compatibility tradeoffs are we making for
   each one?
1. Should QRhi be used or directly interface via the GPU API (Vulkan/GL/?)

## Some History WRT These Changes

A similar port of this code (RV) was done years ago using Qt5. That code is
not available to examine but I (the human) am familiar with how it was
done. It was not fully completed and there were some big unresolved
problems. Here are some things I remember so we can at least know what was
possible at that time:

- The main windows was QtQuick window of some sort. It might have been
  necessary to hack Qt to obtain > 8 bit framebuffer depth. Not sure we can
  do that here or if we'd need to fork QMainWindow/QQuickWindow and make
  our own version of it. I have a feeling that a raw QQuickWindow as used.
- The playback performance improved dramatically because the "render" event
  was no longer supported (no GC in Mu/Python could occur) and the thread
  seperation gave the app more time to work. The previous code's render
  timing was discarded in favor of whatever would make the QtQuick code
  work.
- GL had to be upgraded to 3.2 core profile -- I don't remember why but it
  wasn't too hard. Any code which required the compatibility profile was
  left behind (e.g. MuGL).
- As stated above there were many trade-offs and unresolved
  problems. Unfortuntately the project was canned before completion and
  that branch was not checked into the OpenRV repo.

## Prior Work: rv-foundation and rvfdemo

Two local repositories, looked at on 2026-10-12:

- `~/git/rv-foundation`: RV's base, image, movie, IP and `RvApp` libraries
  built on macOS without OpenGL, Qt, Mu or Python, with a native Metal
  renderer, an AVFoundation movie plugin instead of FFmpeg, and a C++
  facade framework (`TDFoundationFacade`) for host apps. Built with PBJ.
- `~/git/rvfdemo`: a SwiftUI/AppKit app ("RVF Demo") on top of that facade,
  through an Objective-C++ bridge: open/merge media, RV-like menus,
  transport, timeline scrubbing, audio, colour and view controls, pan/zoom.

### What they show

- **The core runs without the app.** `IPCore`, `IPBaseNodes` and `RvApp`
  work under a different host and renderer. That supports option D: an app
  over the shared core is practical.
- **RV's GLSL survives a non-GL backend.** `TwkMetal/MetalMSLCompile.cpp`
  translates the runtime-assembled GLSL at runtime: glslang to SPIR-V, then
  SPIRV-Cross to MSL, with a cache of compiled libraries keyed by source
  hash (`MetalShaderCache`). Before compiling, it rewrites the GLSL:
  - promotes `#version 150` to 330 or later (glslang's minimum)
  - rewrites `sampler2DRect` to `sampler2D`, and marks the binding so the
    sampler uses unnormalized coordinates (texture rectangle semantics)
  - drops the `GL_ARB_texture_rectangle` extension line
  - gathers loose `uniform` declarations into one uniform block, and gives
    samplers explicit `layout(binding = N)`

  Those rewrites are what a Vulkan path needs too. Vulkan would stop at
  SPIR-V (glslang with the Vulkan client), without SPIRV-Cross.
- **The renderer was replaced, not abstracted.** `ImageRendererMetal.cpp`
  takes the place of `ImageRenderer.cpp`. GL enum values survive as format
  descriptors (`glInternalFormat`, `glFormat`), and `ImageTextureBackend.h`
  / `ImageFBOBackend.h` are the start of a backend seam. The upload plan
  (`MetalUploadPlan`) and GPU expansion of formats Metal can't sample
  directly (`MetalSourceGpuExpandPrivate`) cover the same format gaps
  Vulkan has (3-channel and some 16-bit formats are optional there).
- **The host owns the target.** The facade's supported model is "host-owned
  texture": the host acquires the drawable, binds its texture
  (`Core::bindMetalTexture()`), asks for a render, waits until the render
  is scheduled, then presents. That is the shape a QtQuick item (or
  `QSGRenderNode`) needs as well.
- **IPCoreCommands work as designed.** `TDFoundation/Core.cpp` sends
  property changes through `TwkApp::CommandHistory`, with an
  `ExecutionPolicy` of `Queued` or `Immediate`:
  - `setProperty()` (edit only) and `setPropertyUndoable()` (built on
    `IPCore::Commands::SetPropertyVector`)
  - `flushCommands()`, `undo()` and `redo()`
  - a wrapper command that invalidates render state after each do/undo

  So sub-goals 3 and 4 can be built on the existing TwkApp and
  IPCoreCommands code.
- **Measurement tools.** Validation programs and diagnostics for playback
  cadence, redraw attribution and presentation timing
  (`PlaybackRedrawDiagnostics`, `src/bin/validation`): a model for
  checking constraint 4.

### What they don't answer

- **Threading.** The facade has one owner thread for session and control,
  and checks that render/upload calls always come from one thread. Its
  header says queued execution "controls when graph mutation runs; it is
  not cross-thread marshaling". The demo does everything on the main
  thread. So decisions 3-5 are still open.
- Mu and Python. Foundation removes them, and RV packages it needs
  (`source_setup`, session lifecycle, progressive loading, transport
  events) were rewritten in C++ (`TDFoundationRuntime/packages`).
  Constraint 1 rules that approach out for this port.
- Vulkan, Linux, Windows, QtQuick and remote sync.

### How far the code has drifted

Lines that differ, ignoring whitespace, between rv-foundation and this
repo (`src/lib/ip/IPCore`):

| File | Differing lines |
| --- | --- |
| `IPNode.cpp` | 0 |
| `ShaderExpression.cpp` | 2 |
| `IPImage.cpp` | 18 |
| `FBCache.cpp` | 42 |
| `ShaderFunction.cpp` | 63 |
| `ShaderCommon.cpp` | 92 |
| `Session.cpp` | 493 |
| `IPGraph.cpp` | 1435 (includes our hover-preview and cache work) |

So the graph and shader-generation code is still close to ours: the
translator sees nearly the same GLSL it would get here. Most of the
divergence is the renderer, the runtime and the facade.

### Licences

- Apache-2.0 ("Open RV Contributors"), so reusable here with attribution:
  `TwkMetal` (including `MetalMSLCompile`), `ImageRendererMetal.cpp`,
  `ShaderProgramMetal.cpp`, the `Metal{FBO,Renderer,Shader,Texture,
  UploadTexture}Private` files and the `Image*Backend` headers.
- "Copyright (c) 2026 Seth Rosenthal. All rights reserved.": the facade,
  `TDFoundationRuntime` and its packages, `TDHostSupport*`, the AVFoundation
  plugin, two `TwkApp` files, `PlaybackRedrawDiagnostics`, and in `IPCore`
  `MetalUploadPlan`, `MetalSourceGpuExpandPrivate`,
  `MetalSourceLifecyclePrivate*`, `MetalSourceTexturePrivate` and
  `MetalSourcePathDiagnostics`. All of rvfdemo carries the same notice.
  These need the author's permission (or a relicence) before any code is
  copied into OpenUTV; until then they are reference only.

### Effect on the decisions

- Decision 1: rvfdemo is a working example of option D's shape (a native
  app over the shared core). Here the boundary would be the Mu/Python
  command modules rather than a C++ facade, since scripting stays.
- Decision 2: the GLSL rewrite passes are the reusable part for Vulkan,
  and the Metal renderer shows how far `ImageRenderer` has to change. It
  also adds an option to weigh against MoltenVK: native Metal on macOS
  (starting from rv-foundation's renderer, which already works) and
  Vulkan on Linux and Windows. That is two backends, but EDR/HDR on macOS
  is already proven on Metal and not yet on MoltenVK.
- Sub-goals 3 and 4: use rv-foundation's `CommandHistory` design
  (`Queued`/`Immediate`, undoable wrapper, flush) as the starting point,
  then add what it lacks: handing state across threads.

## Current State (as of 2026-10-12)

Facts from the code that the plan has to work around. File and line
references are approximate.

### Presentation and rendering

- Vulkan and Metal are already used, but only for presentation. OpenGL
  does all the rendering, and the result is passed across:
  - macOS: `MetalView` / `QTMetalVideoDevice` blit the GL render into an
    IOSurface shown by a CALayer (see `RvCommon/MetalCpuFallback.md`).
  - Linux and Windows: `VulkanWindow` / `QTVulkanVideoDevice` import
    Vulkan memory into GL with `GL_EXT_memory_object_{fd,win32}`.
  - Both provide 10-bit output. On macOS they also provide EDR and HDR
    PQ/HLG. `GLView` (`QOpenGLWidget`) is the 8-bit fallback.
- `VulkanView` already puts a `QWindow` inside the `QMainWindow` with
  `QWidget::createWindowContainer`. The keyboard focus, mouse tracking and
  drag-and-drop fixes for that kind of setup are in the history of
  `MetalView.mm` and `VulkanView.cpp`.
- The GL context is a compatibility profile (OpenGL 2.1 on macOS).
  TwkGLF and `HoverPreviewOverlayGL.cpp` still use some fixed-function GL.
- Shaders are assembled at runtime: `ShaderFunction` / `ShaderProgram`
  build programs from about 135 GLSL function files in `IPCore/glsl`,
  depending on the graph. Any non-GL backend needs runtime shader
  compilation, not build-time `qsb`.

### The frame

- `Session::render_v2()` sends `pre-render`, calls
  `evaluateForDisplay()`, which produces an `IPImage` tree, then
  `ImageRenderer::render(frame, root, aux, ...)`, then `post-render`.
  Evaluation and rendering are already separate calls on the same thread
  (sub-goal 2).
- `ImageRenderer` hardly looks at graph nodes while rendering: the only
  uses of `IPImage::node` are for names (debug output, render info). So the
  `IPImage` tree is a natural boundary between threads. To check: it must
  not point to anything the GUI thread can change during a render
  (properties read through the image, shared FrameBuffers).
- Script drawing happens inside the render: `AuxUserRender`
  (`Session.cpp:172`) sends the `render` event to Mu and Python while the
  GL context is current. Constraint 1 now drops this ability.
- Cache threads (`IPGraph::evalThreadMain`) already evaluate graph
  products off the GUI thread, including `TextureOutputGroupIPNode`s
  queued with `requestTextureOutput()`.
- Playback pacing is RV's own: timers in `RvDocument` and `DisplayLink`
  (CVDisplayLink) on macOS, with frame skipping in `render_v2`. QtQuick's
  render loop is driven by vsync. See decision 5.

### Things that use the dropped render handlers

These have to be ported to QML, moved into the graph, or dropped:

- Mu UI (`src/lib/app/mu_rvui`): `timeline.mu`, `motion_scope.mu`,
  `inspector.mu`, `presentation_mode.mu`, `wipes.mu`, `rvui.mu`,
  `mode_manager.mu`, with `glyph.mu` / `gltexture.mu` as drawing helpers.
  About 238 `glBegin`/`glVertex` calls in all.
- Bundled packages with render handlers: `data_display_indicators`,
  `lat_long_viewer`, `sync` (remote pointer/annotation feedback),
  `rvnuke`, `pyside_example`. Nine plugin files use `glBegin`.
- Annotation (paint) is drawn by `ImageRenderer::renderPaint` from the
  graph, so it is not affected.

### Things that use Qt widgets (constraint 1)

- At least these bundled packages use QtWidgets or
  `rv.qtutils.sessionWindow()`: `annotate`, `media_info`,
  `multiple_source_media_rep`, `pyside_example`, `rvnuke`,
  `session_manager`, `webview2`, `window_title`. They add dock widgets,
  menus and toolbars, and expect `sessionWindow()` to be a `QMainWindow`.

### IPCoreCommands

- The commands exist (`SetProperty`, `NewNode`, `SetInputs`,
  `SetSessionState`, ...), and TwkApp has `Undoable` / `CompoundCommand`
  support. Nothing uses them: no code calls `INIT_IPCORE_COMMANDS`, and the
  Mu and Python command modules (`IPMu`, `RvApp` `CommandsModule.cpp`)
  change the Session and properties directly.
- Scripts expect to read back a value right after setting it
  (`setIntProperty` then `getIntProperty`). Whatever defers state changes
  has to keep that working (decision 3).

### Live views (sub-goal 6)

- `TextureOutputGroupIPNode` is a `DisplayGroupIPNode` with its own
  size, data type, frame, flip/flop and tag. It is evaluated on a cache
  thread when queued with `IPGraph::requestTextureOutput()`, and the
  result is checked into the FBCache.
- The only user so far is the hover preview: `IPGraph::hoverPreviewNode()`,
  `RvDocument::showHoverPreview()`, drawn by `HoverPreviewOverlayGL`.
  That is the model for a QML item showing a live view of the graph.

### Offscreen rendering: `utvio` / `rvio`

`utvio` (also installed as `rvio`) is a problem for any move to newer GL
or to Vulkan. It uses the same renderer as the viewer, but has its own
context creation, its own software-rendering setup, and overlays drawn
with MuGL. All of it has to change along with the viewer, and its output
has to stay close to the viewer's.

`utvio` and the other tools don't guarantee pixel-identical output across
binaries (different GPUs, drivers, software renderers, builds); they only
assume the differences are reasonably small. The same applies to a new GL
profile or a new backend: the check is "within a small RMS difference",
for example with `rmsImageDiff` (`src/bin/imgtools/rmsImageDiff`), not an
exact match. The tolerance still needs choosing.

- Context creation is in `TwkGLFFBO/FBOVideoDevice.cpp`, and every
  platform creates a legacy (compatibility) context:
  - Linux: GLX (`glXCreateContext`) with a display, otherwise EGL with no
    context attributes, on the GPU (`EGL_PLATFORM_DEVICE_EXT`) or on
    Mesa's surfaceless platform
  - macOS: CGL with no profile attribute (so GL 2.1). It asks for an
    accelerated renderer by default (what the old separate `rvio_hw` did;
    the session_manager `makepng`/`maketif` scripts still name it) and
    falls back to Apple's software renderer (`kCGLPFARendererID`) only
    when there is none, or with `UTV_SOFTWARE_GL`
  - Windows: `wglCreateContext`
- Software rendering, for machines without a usable GPU (render farms,
  containers, VMs):
  - Linux: Mesa llvmpipe through EGL surfaceless. `utvio_sw` / `rvio_sw`
    (`bin/imgtools/rvio/utvio_sw.sh`) force it, replacing the old
    OSMesa build; see `docs/headless-rendering.md` and the `UTV_GL_*`
    environment variables
  - Windows: the `opengl32.dll` forwarder and `UTV_OPENGL=software`
    (AGENTS.md 2.2), which switch to Mesa llvmpipe from OpenUTVDeps
  - macOS: Apple's software renderer, now only a fallback (headless VMs,
    CI runners)
- macOS is a special case: all current Mac hardware can render offscreen
  on the GPU, so `utvio` there doesn't depend on software rendering the
  way Linux render farms and containers do. The software fallback only
  matters for VMs and CI runners.
- Overlays: `-overlay` / `-leader` scripts are Mu and draw with MuGL. Four
  of the five bundled ones (`rvio_basic_scripts`: `frameburn`, `matte`,
  `watermark`, `simpleslate`) use GL calls, and users have their own.
  Dropping script drawing (constraint 1) removes these, and they are a
  documented `utvio` feature used on render farms. They need a graph-based
  replacement before the render events go, e.g. on `OverlayIPNode` (which
  already draws rectangles and text in the graph) or paint nodes.

What each step needs:

- GL core profile: create core contexts on every platform
  (`glXCreateContextAttribsARB`, EGL with `EGL_CONTEXT_OPENGL_PROFILE_MASK`
  and a version, CGL with `kCGLPFAOpenGLProfile`,
  `wglCreateContextAttribsARB` through the `opengl32.dll` forwarder), and
  check the software renderers give a core context: llvmpipe supports
  core 4.5; Apple's software renderer and the Windows forwarder path need
  checking.
- Vulkan: headless is simpler than GL (render into a `VkImage`, no
  surface), but the software path changes completely:
  - Linux: Mesa lavapipe (`mesa-vulkan-drivers` on Debian/Ubuntu,
    `mesa-vulkan-drivers` on RHEL-family) instead of llvmpipe GL
  - Windows: lavapipe and its ICD manifest, plus the Vulkan loader, would
    have to be added to OpenUTVDeps; the `opengl32.dll` forwarder approach
    doesn't apply
  - macOS: MoltenVK (or native Metal) needs a Metal device, and there is
    no software fallback like Apple's GL software renderer. Since all
    current Mac hardware renders offscreen on the GPU, that is acceptable
    for users; what needs checking is VMs and CI runners (e.g. GitHub's
    macOS runners), which today may rely on the software fallback
  - containers: the NVIDIA container runtime must expose the Vulkan ICD
    (the `graphics` driver capability), not just EGL
- The launcher logic, environment variables, `utvio_sw`, the GPU probe in
  the Windows launchers and `docs/headless-rendering.md` all have to be
  redone for whatever the viewer ends up using.

Other users of the same offscreen path: the viewer starts `utvio` for
thumbnails, and output video device plugins read back from the renderer.

## Decision Notes

Analysis for the decisions above, plus others the plan depends on.

### 1. Main window

| Option | Plugins / `sessionWindow()` | Performance, HDR | Problems |
| --- | --- | --- | --- |
| A. `QMainWindow`, central `QQuickWindow` in `createWindowContainer` | Unchanged: docks, menus, toolbars | Native surface; same as `VulkanView`/`MetalView` today | Widgets can't overlap the view; focus, keys and drag-and-drop across the container need work (already done once for `VulkanView`/`MetalView`) |
| B. `QMainWindow`, central `QQuickWidget` | Unchanged | Renders to an offscreen texture that the widget stack composites: an extra copy every frame, no 10-bit or HDR | Fails constraint 4 for playback |
| C. Only a `QQuickWindow` / `ApplicationWindow` | Broken: no `QMainWindow`, no docks; script widgets only as separate windows | Best | Fails constraint 1 unless a shim reproduces `QMainWindow` |

Recommendation: A. It keeps constraint 1, has the same performance as
today, and the QML HUD lives inside the QtQuick window, so widgets not
being able to overlap it doesn't matter. C can come later, once QML has
equivalents for docks and menus.

#### Option D: a separate next-gen app (after "Unshackling" and the history)

The "Unshackling" section allows a new product, and the earlier port used
a raw `QQuickWindow`. That suggests a fourth option:

- D. Two apps sharing the core: the current app (`QMainWindow`,
  `GLView`/`MetalView`/`VulkanView`, Mu HUD, render events, compatibility
  profile GL) carries on unchanged, and a new app is a raw `QQuickWindow`
  (option C) built on the same IP and scripting libraries.
- What D gets:
  - Constraint 1 holds for the current app without compromise. The new
    app only has to follow it minus render handlers and `QMainWindow`
    widgets, and both apps read and write the same session files and
    load the same packages.
  - The new app can start on GL 3.2+ core (and later Vulkan), the
    threaded render loop and the QML HUD from day one. Nothing has to be
    built twice to keep the old path working inside one binary (the Mu HUD
    toggles of Phase 2, compatibility and core profile in one process).
  - Every phase of the current plan stays shippable for free: the current
    app is what ships until the new one is ready.
  - "Plays nice with its antecedent" becomes concrete: same session
    format, same packages where possible, and remote sync between the two
    apps (constraint 2 then covers mixed sessions).
- What D costs:
  - Sub-goal 5 (modularizing UI, scripting and IP) becomes a prerequisite
    rather than a side goal: the scripting modules (`IPMu`, `RvApp`
    `CommandsModule`, `MuUICommands`, `PyUICommands`) must work without
    `RvCommon`'s widgets.
  - Two apps to build, package and test while both exist.
  - Packages that use `sessionWindow()` don't work in the new app until it
    has some replacement (a QML dock area, or widgets in separate windows).

If a new product is acceptable, I'd recommend D over A: it removes most of
the compatibility work from the new code path instead of carrying it.

### 2. QRhi or the GPU APIs directly

- QRhi:
  - one API for Vulkan, Metal, D3D11/12 and GL, and the one QtQuick
    already uses (`QQuickRhiItem`, `QSGRenderNode`)
  - GLSL (Vulkan style) stays the source language and is compiled to
    SPIR-V / MSL / HLSL, which suits constraint 3
  - Downsides:
    - QRhi has limited compatibility guarantees between Qt minor
      versions
    - runtime compilation needs `QShaderBaker` (QtShaderTools)
    - no PBO-style upload control
    - its texture formats may not cover every format RV uploads today
      (3-channel, 16-bit integer, planar YUV), so some would need shader
      unpacking
- Native Vulkan and Metal (plus GL):
  - full control of uploads, synchronization and formats, which matters
    for constraint 4
  - but two or three backends to write and maintain, and glslang +
    SPIRV-Cross for shaders
- Not decided yet. The way to decide is a spike: upload and display a 4K
  16-bit-float EXR sequence and a 10-bit YUV movie through QRhi, compare
  frame times and upload bandwidth with the GL path, and list which RV
  formats QRhi lacks. Either way, `ImageRenderer` first needs an internal
  backend interface with GL as the first backend, so this decision
  doesn't block the earlier phases.

#### Human Notes on these

I have serious reservations about relying on Qt for the GPU interface
(QRhi). In the past the Qt developers have not been willing to extend their
APIs to support use cases beyond their own (for good reason). I know they
advertise Qt as a full cross platform interface but in practice there are
caveats caused by their business concerns and willingness. For that reason
I would prefer to use Vulkan directly (plus MoltenVK on Mac and whatever
passes for standard APIs on Windows these days). I'm not wedded to these
opinions.

That being said if we can port to a newer GL (3.2 or greater core profile)
first as a stepping stone that would be preferred.

#### Notes on Vulkan directly

- QtQuick can use Vulkan without RV depending on QRhi for its own
  rendering: Qt's scene graph uses QRhi to draw the QML items, but RV can
  create the `VkInstance`/`VkDevice` itself and hand them to the window
  with `QQuickGraphicsDevice::fromDeviceObjects()`. RV then records its own
  Vulkan commands for the image (`QSGRenderNode`, or
  `QQuickWindow::beginExternalCommands()` with the command buffer from
  `QSGRendererInterface`), or renders into its own `VkImage` shown by a QML
  item. Qt's part is limited to drawing the HUD and presenting.
- The same pattern exists for GL: `QQuickGraphicsDevice::fromOpenGLContext()`
  lets RV own the GL context during the GL core stepping stone.
- macOS: MoltenVK from Homebrew (`molten-vk`), which fits AGENTS.md 1.5
  (linked from the user's Homebrew, not bundled). To check: EDR and HDR
  PQ/HLG through MoltenVK's `VK_EXT_swapchain_colorspace` (extended sRGB
  linear, HDR10 ST2084, HLG) and `VK_EXT_hdr_metadata`, to match what
  `MetalView` does now.
- Windows: NVIDIA, AMD and Intel desktop drivers all ship Vulkan, and RV
  already uses it for 10-bit presentation. The gap is machines with no GPU
  driver (Hyper-V, VirtualBox without 3D, RDP; AGENTS.md 2.2), which get
  Mesa llvmpipe for GL today. The Vulkan equivalent is Mesa's lavapipe,
  which would have to be added to OpenUTVDeps, or those machines keep the
  GL path.
- Shaders: glslang compiles the runtime-assembled GLSL to SPIR-V. GLSL
  stays the source language (constraint 3); the GL and Vulkan dialects
  differ mainly in bindings (`layout(set, binding)`, uniform blocks
  instead of loose uniforms), which the shader generator would emit.

#### GL 3.2+ core profile as the stepping stone

The renderer was already close to core: the GLSL function library
(`IPCore/glsl`, about 135 files) uses no removed built-ins, the generator in
`ShaderProgram.cpp` writes `#version 150` with `in`/`out`, its own
`FragColor`, matrix uniforms and vertex attributes whenever the context
has GLSL 1.50 (its `varying`/`attribute` branch is only for the macOS 2.1
context), and drawing uses VBOs and `glVertexAttribPointer`. Rectangle
textures and pixel buffer objects are core and need no change.

The work is split so that each step can be merged on its own while RV
still runs in the compatibility context, so MuGL keeps working. Only the
last step switches the context.

Each step is checked with `utvio` renders compared against the previous
commit with `rmsImageDiff` (plain, scaled, resized, stack, layout, an
annotated session, and 8/16-bit, half and float images with 1, 2 and 3
channels), and with standalone GL programs where a code path doesn't run
in macOS's 2.1 context. The viewer-only paths (`MetalView`, `GLView`,
`VulkanView`, hover preview, desktop output) need a manual check, and Linux
and Windows are first checked by CI.

##### Done

| Step | Commit | Notes |
| --- | --- | --- |
| `GL_QUADS` drawn as `GL_TRIANGLE_FAN` (11 call sites, mostly through `RenderPrimitives`); unused `glDrawBox` removed | `21f008a4` | |
| No-op fixed-function calls removed from the renderer: `glTexEnvi`, `glColor4f`, texture-target `glEnable`s; unused `glJitterFrustum` / `glJitterPerspective` removed | `5e5709c9` | The `TwkGLF/GL.h` helpers stay for MuGL |
| `GL_RED` / `GL_RG` with texture swizzles instead of luminance formats | `cd78427e` | Needs GL 3.0 (or `ARB_texture_rg`) and GL 3.3 (or a swizzle extension). macOS's 2.1 context has no swizzle, so it keeps luminance until the switch; Linux and Windows use the new path. `RV_RENDERING_USE_TEXTURE_SWIZZLE=0` turns it off |
| Extension queries that work in core contexts: `TwkGLF::glExtensions()` (`glGetStringi` on GL 3.0+), `TwkGLF::hasExtension()` with a table of core-promoted extensions; `TWK_GL_SUPPORTS` uses it on every platform | `7df3e36e` | A macOS 4.1 core context lists only 43 extensions, without `ARB_sync`, `ARB_texture_rectangle`, `ARB_pixel_buffer_object` or float textures |
| Numeric internal format `3` replaced with `GL_RGB8` | `3a373b2d` | |
| A VAO for every draw in `RenderPrimitives` on GL 3.0+ | `6c375aef` | Made per draw (a VAO belongs to one context) and the previous binding restored. macOS's 2.1 context skips it |
| Core names for the framebuffer and renderbuffer functions and `glActiveTexture`; the `glBlitFramebufferEXT` stub in `utvio`'s `glfix.c` removed | (pending) | Needs GL 3.0 or `ARB_framebuffer_object` (macOS 2.1 has it). A driver with only `EXT_framebuffer_object` would no longer work |
| `GL_APPLE_` extensions in a macOS core context checked (2026-10-12, Apple M1 Pro, 4.1 Metal) | | `client_storage`, `texture_range`, `rgb_422` and `row_bytes` remain, so the upload fast path survives. `ycbcr_422` and `float_pixels` are gone; core has float textures |

##### Remaining

1. Text: replace the renderer's FTGL text with MTSDF (decided
   2026-10-12). See "Text rendering" below.
1. State saving. `ImageRenderer.cpp` wraps rendering and uploads in
   `GLPushAttrib(GL_ALL_ATTRIB_BITS)` and
   `GLPushClientAttrib(GL_CLIENT_ALL_ATTRIB_BITS)` (five places), and
   `Session.cpp` wraps the `render` event in a sixth. A comment says they
   exist so FTGL text (the Mu HUD's) renders correctly after the image.
   Core removes both. They go once nothing draws with FTGL after the
   renderer, or are replaced by restoring the state the renderer changes
   (pixel store, blend, scissor, bound textures).
1. Measure texture upload speed in a macOS core context, against the 2.1
   context (constraint 4).
1. Switch the context to core:
   - viewer: `GLView`'s `QSurfaceFormat`, the offscreen GL context behind
     `MetalView` (`QTMetalVideoDevice.mm`) and behind `VulkanView`
   - `utvio`: `TwkGLFFBO/FBOVideoDevice.cpp` for GLX, EGL, CGL and WGL
     (see "Offscreen rendering" above)
   - the shader generator switches to its GLSL 1.50 branch by itself
   - this is the step that needs the HUD decision: the Mu HUD, MuGL and
     render-event drawing can't run in a core context (or, with option D,
     only the new app uses core)

Problems found along the way, not caused by these steps:

- Annotation text doesn't render in `utvio` output, not even with the
  code from before the first step. (Possibly the hand-written test session
  is missing something the annotate tool writes; not yet checked against
  a session saved from the viewer.)
- `utvio -debug gpu` doesn't print the renderer's GL capability report.

#### Text rendering

Decided 2026-10-12: replace FTGL with multi-channel signed distance
field text (MTSDF: three distance channels plus a true distance in the
fourth), generated from FreeType outlines with `msdfgen` (MIT).

- Today: annotation text (`PaintCommand` `Text`) and the hover-preview
  label draw with TwkGLText (`src/lib/ui/TwkGLText`), built on FTGL
  (`FTTextureFont`/`FTPixmapFont`) and the fixed-function pipeline:
  `glRasterPos`, `glPixelTransfer`, `glPixelZoom`, the matrix stack, and
  FTGL's own immediate-mode drawing. Around it are
  `GLState::FixedFunctionPipeline` (unbinds the shader),
  `GLPipeline::useCurrentProjectionGL2()` / `useCurrentModelviewGL2()`,
  `glEnable(GL_TEXTURE_2D)`, and the matrix push/pop in
  `HoverPreviewOverlayGL.cpp`.
- The `GLtext` API in use is small: choose a font (`init`, `init(name)`)
  and size, set a colour, measure (`bounds`, `boundsNL`), font metrics
  (`globalAscenderHeight`, `globalDescenderHeight`), draw (`writeAt`,
  `writeAtNL`), and per-GL-context state (`newContext`, `setContext`,
  `deleteContext`, `clear`, used by `GLVideoDevice` and `rvprof`). Users:
  `PaintCommand`, `HoverPreviewOverlayGL`, `MuGLText` (the Mu `gltext`
  module), `GLVideoDevice`, `rvprof`.
- Plan:
  - layout with no GL in it: font loading, metrics, glyph quads and the
    CPU side of the glyph atlas (MTSDF generated on demand, since CJK
    fonts have about 30,000 glyphs). Usable unchanged by a Vulkan or
    Metal renderer later.
  - one small draw function per backend; for GL, through
    `RenderPrimitives` with a shader, taking the matrix and colour as
    arguments instead of global state. Atlas textures exist per GL
    context; glyph bitmaps can be shared.
  - `PaintCommand` and `HoverPreviewOverlayGL` move to it, which removes
    their fixed-function setup.
  - `GLtext` stays on FTGL for MuGL and `rvprof` at first. Later it can be
    reimplemented on the new code (in the compatibility context it can
    read the fixed-function matrices), removing FTGL and the reason for
    the state saving above.
- Chinese text: TwkGLText already decodes UTF-8 (with `utf8::unchecked`,
  so invalid UTF-8 is undefined behaviour), but the embedded fonts (Luxi
  Sans by default, Courier, the Tweak font) have no CJK glyphs and there is
  no font fallback. MTSDF handles complex glyphs with larger cells (about
  48-64 px per em for CJK, against about 32 for Latin).
- Open decisions:
  - fonts: bundle a CJK font (Noto Sans CJK, OFL, about 16 MB per weight,
    with regional variants for Chinese, Japanese and Korean) or use system
    fonts with fallback (CoreText, DirectWrite, fontconfig), which can make
    `utvio` output differ between machines
  - text shaping: Chinese needs almost none, but Arabic, Hebrew,
    Devanagari and Thai need HarfBuzz and bidirectional text. Build layout
    on HarfBuzz from the start or not
- Expected output change: glyph positions, kerning and anti-aliasing will
  differ slightly from FTGL's, which is within the "reasonably small
  differences" `utvio` allows.

So the renderer itself is a modest port. What can't move to core is
everything that draws in the render events: the Mu HUD and MuGL (238
`glBegin`/`glVertex` calls) and the packages listed under "Things that use
the dropped render handlers". The core profile therefore arrives together
with dropping the render events, as in the earlier port, or, with option
D, the new app uses core from the start while the current app stays on
the compatibility profile.

### 3. Handing state from the GUI thread to the render thread

- Snapshot: scripts and commands change the graph on the GUI thread right
  away. During QtQuick's sync phase, when the GUI thread is blocked, the
  frame's `IPImage` tree is evaluated, or the state it needs is copied,
  and handed to the render thread. Reading back a value just set works as
  it does today.
- Queue: changes are IPCoreCommands queued on the GUI thread and applied
  at sync. Reads have to check the pending queue first, or scripts that
  read back values break.
- Snapshot keeps constraint 1 more easily. The queue fits sub-goals 3 and 4
  (undo) better. The two can be combined: commands are applied right away
  and also recorded (for undo and sync), and the render thread gets a
  snapshot.

### 4. Where evaluation runs (sub-goal 2)

- In the sync phase on the GUI thread: simple, but the GUI thread is
  blocked while it runs.
- On the render thread: the graph must not change while it runs, which
  needs the queue from decision 3.
- On a dedicated eval thread one or more frames ahead, with `IPImage`
  trees cached for the render thread to pick up. This is the "cache
  IPImage" idea in sub-goal 2, and probably the best for constraint 4.
  The cache must be invalidated when state changes.

### 5. Who drives playback timing

- QtQuick's threaded render loop paces itself on vsync. RV paces playback
  itself (timers, CVDisplayLink, frame skipping), including rates that
  don't match the display (23.976 on 60 Hz).
- Options: RV's clock picks the frame and the QtQuick loop just presents
  the latest one; or a custom render loop / `QQuickRenderControl`.
- To check: which render loop Qt 6.11 uses for each backend and platform
  (`QSG_RENDER_LOOP`), especially OpenGL on macOS.
- The earlier port dropped RV's render timing and used whatever made the
  QtQuick loop work, and playback got much faster (no Mu/Python GC during
  the render, more time between frames). So the default should be: the
  QtQuick loop presents on vsync, and RV only picks which frame to show
  from its playback clock (audio or timer) during the sync phase. RV's
  pacing code is kept only where measurements show it is needed.

### 6. Input and event compatibility

- Mu and Python event bindings (keys, pointer, `render`-free modes) must
  keep working. The translation from Qt events to TwkApp events, now in
  `GLView` / `MetalView` / `VulkanWindow`, moves to the QtQuick view.
- QML HUD items get the first chance at pointer events in their area, and
  the rest go to RV's event tables. Keyboard input goes to RV unless a QML
  item has focus.
- Sync (constraint 2): QML components must change state through the same
  paths (commands, events) the Mu HUD uses, so `sync` sees them, and must
  respond to the same events and mode names (`timeline`, ...), which are
  effectively API.

## Proposed Phases

Each phase leaves the app shippable. Exit criteria are checked on macOS,
Linux and Windows.

These phases assume option A (one app). With option D the order is
different: Phase 0, then the modularization from sub-goal 5, then the new
app goes straight to Phases 2-3 on GL core with the threaded loop (no Mu
HUD toggles, no Phase 1), and the current app is left as it is.

### Phase 0: Decisions and spikes

- Settle decisions 1-6.
- Spikes:
  - a `QQuickWindow` in a `QMainWindow` on each platform, showing the GL
    render through the existing interop (IOSurface, Vulkan memory) as a
    texture, with 10-bit and HDR on macOS
  - the QRhi upload and format spike from decision 2
  - render loop and pacing behavior from decision 5
  - run rv-foundation's GLSL rewrite passes (`MetalMSLCompile.cpp`) over
    the shaders this repo generates, compiled with glslang's Vulkan client,
    to see how much of the Vulkan shader path already exists
- List the dependencies to cut for sub-goal 5 (UI, scripting and IP) as
  CMake link rules, as was done for the ip libraries.
- Exit: written decisions in this file; measured spike results.

### Phase 1: QtQuick host, nothing else changes

- A QtQuick view replaces `GLView` / `MetalView` / `VulkanView` as the
  central widget. GL keeps rendering on the GUI thread (basic render
  loop), and QtQuick shows the result as a texture.
- The Mu HUD keeps drawing in the same GL pass for now.
- Exit: no regressions in playback frame rate, 10-bit / HDR output, input,
  drag-and-drop, sync, presentation mode, and the bundled packages.

### Phase 2: QML HUD alongside the Mu HUD

- A C++ QObject layer exposes Session state to QML (frame, range, marks,
  sources, cache state, info).
- Port the HUD one component at a time, each behind a toggle with the Mu
  version as fallback: timeline, timeline magnifier, info, pixel
  inspector, motion scope, wipes, presentation mode.
- Replacements or migration notes for the bundled packages that draw in
  render handlers.
- Live views: a QML item backed by a `TextureOutputGroupIPNode`, starting
  with the hover preview (sub-goal 6).
- Exit: every Mu HUD component has a QML replacement with the same events
  and modes; sync works with the QML HUD.

### Phase 3: Threading

- Remove the render events (constraint 1) and the Mu HUD.
- Evaluation and render split and state handoff (decisions 3 and 4).
- Script and UI changes go through IPCoreCommands where decision 3 needs
  it. Optional undo/redo (sub-goal 4).
- Turn on the threaded render loop.
- `utvio` overlays (`-overlay`, `-leader`) replaced by graph-based
  versions before the render events are removed.
- Exit: playback frame times equal or better than Phase 1 (constraint 4);
  bundled packages and session files unchanged; `utvio` output is within a
  small RMS difference of the viewer's and of the previous release's,
  including with software rendering on each platform.

### Phase 4: Renderer backend (stretch goal 1)

- Port `ImageRenderer` / TwkGLF to the backend chosen in decision 2,
  with runtime shader compilation from the existing GLSL.
- QtQuick then uses the same API natively, with no GL interop.
- Exit: same as Phase 3, without GL on the platforms that have the new
  backend.

## To Verify

- `QQuickWindow` HDR / 10-bit swapchain support in Qt 6.11 for each RHI
  backend, and how it compares with the current `MetalView` EDR setup.
- The render loop Qt 6.11 picks for OpenGL on macOS, and for each backend
  elsewhere.
- Whether `IPImage` trees are safe to read on another thread while the
  GUI thread changes the graph.
- Which RV texture formats QRhi supports, and whether `QShaderBaker`
  handles the runtime-assembled shaders and how fast it is.
- On Windows, QtQuick defaults to D3D. Phase 1 needs GL → D3D interop
  (`WGL_NV_DX_interop2`) or forcing the Vulkan RHI, where the Vulkan
  interop already exists.
- HDR and 10-bit in QtQuick's own swapchain: the earlier port may have
  needed a patched Qt for more than 8 bits. Qt 6 has HDR swapchain formats
  in QRhi (extended sRGB linear, HDR10) and, I believe, a `QSG_RHI_HDR`
  setting for `QQuickWindow`; check what 6.11 actually offers on each
  platform before assuming a Qt patch is needed.
- `QQuickGraphicsDevice::fromDeviceObjects()` with an RV-owned
  `VkDevice`, on MoltenVK as well as native Vulkan.
- MoltenVK EDR / HDR10 / HLG colour spaces against the current `MetalView`.
- Upload speed on macOS in a core context. (It keeps
  `APPLE_client_storage` / `APPLE_texture_range`; checked 2026-10-12.)
- Mesa lavapipe for machines without a GPU driver (Windows, Linux VMs).
- Vulkan samplers with unnormalized coordinates (needed for texture
  rectangle semantics) have limits: no mipmaps, clamp-to-edge or border
  addressing only, and restrictions on which sampling functions can be used.
  Check that RV's rectangle-texture shaders fit them, or switch to
  normalized coordinates.
- Whether Apple's GL software renderer gives a 3.2+ core context, and
  whether the Windows `opengl32.dll` forwarder passes through
  `wglCreateContextAttribsARB` to Mesa llvmpipe.
- Vulkan software rendering for `utvio`: lavapipe on Linux distributions
  and in containers, packaging it for Windows, and whether macOS VMs and CI runners
  (GitHub's macOS runners) have a usable Metal device.
- What `OverlayIPNode` can draw, compared with what the bundled `utvio`
  overlay scripts do.
