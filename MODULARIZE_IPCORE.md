# Modularize IPCore

Plan for moving IPCore's rendering behind an interface, so the renderer is
a library loaded at run time (OpenGL today; Vulkan, Metal or QRhi later),
and for keeping MuGL scripts working without OpenGL. Companion to
`QTQUICK_PORT.md`: this is the "Renderer backend" work (Phase 4 there,
decision 2) made into steps that can start before the backend is chosen.

Status: proposal (2026-10-19). Nothing here is implemented.

## Goals

- IPCore does not link OpenGL. Rendering goes through a `Renderer`
  interface, with each implementation in its own library, loaded at run
  time.
- The current GL renderer becomes the first backend with identical output
  (the test renders and the viewer are unchanged).
- A second backend (Vulkan, Metal, or QRhi over both) can be added
  without touching the image nodes, `Session` or the app.
- Mu scripts that draw with MuGL keep working on a non-GL backend, for
  the subset of GL that scripts actually use.
- `utvio` and the viewer can choose a backend at run time.

Not goals: changing what the image nodes compute, changing the session
format, or rewriting the shaders by hand.

## Current State (measured 2026-10-19, branch `gl-core-profile`)

### The image nodes are already API-neutral

Nodes (`IPBaseNodes` and the others) describe their processing as
`Shader::Expression` graphs built from GLSL function bodies
(`src/lib/ip/IPCore/glsl`, about 135 files) and make no GL calls. The
expression graph is effectively a renderer-independent IR; only its leaf
functions are written in GLSL.

### GL inside IPCore

GL calls and types are concentrated in a few files:

| File | Lines | Role |
| --- | --- | --- |
| `ImageRenderer.cpp` / `.h` | 5383 / 1066 | uploads, PBOs, FBOs, drawing, render passes, caches |
| `ShaderProgram.cpp`, `ShaderFunction.cpp` | 1123, 1702 | compiling and linking programs, binding uniforms and samplers (34 GL calls) |
| `ImageFBO.cpp` | 594 | FBO cache |
| `PaintCommand.cpp` | 914 | annotation drawing (strokes, text) |
| `HoverPreviewOverlayGL.cpp` | 486 | hover preview |
| `Session.cpp` | 6493 | 16 GL calls, nearly all `glFinish` |
| `ShaderCommon.cpp` | 5630 | builds the GLSL text; no GL calls |

The other 55 `.cpp` files in IPCore have no GL.

### GL in the interface

`ImageRenderer.h` has about 180 public declarations and exposes GL types
(`TwkGLF::GLFBO`, `GLuint`) to code outside IPCore:

- `src/bin/apps/rv/main.cpp`, `src/bin/imgtools/rvio/main.cpp`
- `RvCommon`: `RvDocument`, `RvPreferences`, `RvTopViewToolBar`,
  `DesktopVideoModule`, `VulkanWindow`, `QTVulkanVideoDevice`
- `RvApp`: `RvSession`, `Options`, `FormatIPNode`, `CommandsModule`

### Presentation and video devices are GL classes

Every video device derives from `TwkGLF::GLVideoDevice` or
`GLBindableVideoDevice`:

- viewer: `QTGLVideoDevice`, `QTMetalVideoDevice` (plus
  `QTMetalSharedContextWorkerDevice`), `QTVulkanVideoDevice`. The Metal
  and Vulkan ones present a GL render through IOSurface or Vulkan memory
  interop
- desktop and output plugins: `DesktopVideoDevice` /
  `CGDesktopVideoDevice`, `KonaVideoDevice` (AJA), Blackmagic,
  `NDIVideoDevice`
- offscreen: `FBOVideoDevice` (`utvio`)

These are app-level classes, not part of IPCore, so they don't need to
become API-neutral: each backend has its own devices (see A3).

### Scripts that draw with GL

- MuGL (`src/lib/mu/MuGL`) exposes about 130 GL 1.x functions: immediate
  mode, matrix stack, attribute stacks, textures, display lists,
  evaluators, lighting, accumulation, selection and feedback,
  `glDrawPixels` / `glCopyPixels`. MuGLU adds 8 GLU functions.
- The bundled Mu code (18 files: `HUD.mu`, `timeline.mu`, `wipes.mu`,
  `inspector.mu`, `motion_scope.mu`, `presentation_mode.mu`, `rvui.mu`,
  `glyph.mu`, `gltexture.mu`, `annotate_mode.mu`, `matte.mu`,
  `watermark.mu`, `frameburn.mu`, `simpleslate.mu`, `sync.mu`,
  `transform_manip.mu`, `layer_select_mode.mu`,
  `missing_frame_bling_mode.mu`) uses about 30 of them, all 2D:
  - `glBegin` / `glEnd` with `glVertex` (277 calls), `glColor` (139),
    `glTexCoord` (12)
  - `glEnable` / `glDisable`, `glBlendFunc`, `glLineWidth` (54),
    `glPointSize`, `glPolygonMode`, `glPolygonStipple` (1), `glHint`
  - `glPushAttrib` / `glPopAttrib`, the matrix stack (`glMatrixMode`,
    `glPushMatrix`, `glLoadIdentity`, `glTranslate`, `glRotate`,
    `glScale`), `gluOrtho2D`, `glViewport`
  - textures: `glGenTextures`, `glBindTexture`, `glTexParameter`,
    `glTexEnv`, `gluBuild2DMipmaps` (`gltexture.mu`)
  - `glReadPixels` (1), `glClear` / `glClearColor` (1)
  - none use display lists, lighting, evaluators, accumulation or
    selection
- MuGLText (`gltext`) has 13 functions; the bundled code uses `bounds`,
  `size`, `color`, `writeAt`, `writeAtNL`, `ascenderHeight`,
  `descenderDepth`, `init`, `boundsNL`. It draws with FTGL.
- Python: two bundled packages use PyOpenGL directly
  (`data_display_indicators`, `pyside_example`). These bypass MuGL.

## Plan A: the Renderer interface, GL as the first backend

Each step keeps the output identical; checked with the `utvio` test
renders (macOS, Linux; Windows when available) and the viewer.

### A1. Name what callers need

List what `Session`, the apps and the video devices call on
`ImageRenderer` today and group it:

- render an image tree (`IPImage`) to a target, with the aux render
  and aux audio arguments
- targets: the device's default target, offscreen targets, read back
  (`utvio`, thumbnails, `TextureOutputGroupIPNode` live views)
- caches: texture and program cache control, memory limits, flush
- capabilities and settings: formats, float support, PBO / upload
  options, debug reports (`printGLInfo`)
- per-device state: viewport, background, stereo mode, device pixel
  ratio
- render events for scripts (see Plan C)

Most of the 180 public declarations become backend internals.

### A2. `IPCore::Renderer`

An abstract class in IPCore with the A1 operations, using IPCore types
only: no `GLuint`, no `GLFBO`. Targets are opaque handles created by
the backend (`RenderTarget`), with a way for a video device to get the
native object (GL FBO id, `VkImage`, `MTLTexture`) through a typed
accessor.

`ImageRenderer` implements it unchanged. Callers switch to `Renderer`
one at a time; `Session` holds a `Renderer*`.

`Session`'s `glFinish` calls become `Renderer::finish()` (or go away
where they were only for timing).

### A3. Video devices per backend

The video devices stay at the app level and stay specific to a backend:
the GL devices (`GLVideoDevice` and its subclasses) are used with the GL
backend as they are now, and a Vulkan or Metal backend gets its own
devices. The app selects the devices that match the loaded backend. The
devices can be plugins themselves, loaded with or alongside their
backend, as the output devices (AJA, Blackmagic, NDI) already are.

What IPCore needs is only the common `TwkApp::VideoDevice` base, which
has no GL types or calls (one capability flag and some comments speak
in GL texture-target terms), and a way for a device to hand the backend its target
(through the typed native accessor from A2). The output devices can be
ported to a new backend when it is needed, or keep working through a
read back.

### A4. GL backend as a library

Move `ImageRenderer`, `ShaderProgram` / `ShaderFunction` GL code,
`ImageFBO`, the GL parts of `PaintCommand` and `HoverPreviewOverlayGL`
into `libIPRendererGL`. IPCore links no GL.

The library exports one factory function (for example
`IPCore::Renderer* createRenderer(const RendererOptions&)`). It is
loaded with `dlopen` / `LoadLibrary`, as the image format plugins are
(`TwkFB/IO.cpp`). The backend is chosen by a command line option or
setting, with GL the default. The interface is C++: the backends are
built with the same compiler and flags as the app, so a C++ ABI across
the boundary is fine.

### A5. Shaders: keep GLSL, translate per backend

`ShaderCommon` keeps building GLSL from the expression graph. Each
backend turns it into what it needs:

- GL: as today (GLSL 1.20 or 1.50 by context)
- Vulkan: glslang to SPIR-V
- Metal: SPIR-V to MSL with SPIRV-Cross, or rv-foundation's GLSL to MSL
  passes (`MetalMSLCompile.cpp`, Apache-licensed; see `QTQUICK_PORT.md`)
- QRhi: `.qsb` packs from the same SPIR-V (`QShaderBaker`)

Known dialect work:

- rectangle textures (`sampler2DRect`) don't exist in Vulkan or Metal:
  either normalized coordinates in the generated code, or a rewrite pass
  that divides by the texture size
- uniforms move into uniform blocks for Vulkan / Metal
- samplers become separate texture and sampler bindings
- compile at run time, with an on-disk cache keyed by the program text

The Phase 0 spike in `QTQUICK_PORT.md` (compile the generated GLSL with
glslang's Vulkan client) measures how much of this is needed.

### A6. Paint and overlays through the backend

Annotation strokes and text (`PaintCommand`), the hover preview and
other renderer-drawn overlays become backend draw calls with a small
vocabulary: textured and coloured triangles, blend modes, scissor. Text
already goes through TwkText (MTSDF atlas), which needs only a textured
draw with a distance-field shader.

## Plan B: a second backend

Once Plan A is done, a second backend implements `Renderer`:

- uploads: staging buffers instead of PBOs; the macOS client-storage /
  IOSurface fast path maps to shared-memory buffers on Metal
- render passes and intermediate targets (today's FBO cache)
- the shaders from A5
- present: on Vulkan or Metal, the video device's surface is native,
  with no interop

Choices, from decision 2 in `QTQUICK_PORT.md`:

- **QRhi**: one backend for Metal, Vulkan and D3D, native to Qt Quick.
  Limits: what QRhi exposes (formats, 10-bit / HDR presentation,
  external memory).
- **Vulkan directly**, MoltenVK on macOS: full control, one backend.
  Qt Quick can share the `VkDevice` (`QQuickGraphicsDevice::fromDeviceObjects`)
  and show the renderer's `VkImage` as a texture. HDR / EDR on macOS
  depends on MoltenVK's swapchain colour spaces; check in the spike.
- **Metal directly** on macOS plus Vulkan elsewhere: best macOS
  integration, two backends.

With A done, this is "add a backend" rather than an up-front choice;
the GL backend stays available while a new one matures.

## Plan C: MuGL without OpenGL

### Record and replay

MuGL keeps its Mu API. Behind it, a recorder replaces the GL calls:

- `glBegin` ... `glEnd` collect vertices with the current colour and
  texture coordinate, transformed by the emulated matrix stack, into a
  draw list
- state calls (`glEnable`, `glBlendFunc`, `glLineWidth`, `glPointSize`,
  `glPolygonMode`, textures, `glPushAttrib` / `glPopAttrib`, matrix
  modes) change the recorder's state; `glGet*` and `glIsEnabled` answer
  from it
- lines and points are expanded to triangles on the CPU: Metal and
  MoltenVK have no wide lines and core GL clamps them; this also gives
  consistent joins on every backend
- `GL_LINE` polygon mode, line and polygon stipple: emulated in the
  expansion or in the fragment shader
- `glReadPixels`: a read back from the backend, synchronous for the
  caller
- textures (`gltexture.mu`): uploads through the backend

At the overlay stage the backend replays the draw list with a few
pipelines (coloured, textured, text). On QRhi one implementation covers
Metal, Vulkan and D3D. The GL backend can replay with plain GL, or keep
calling GL directly as today.

A recorded list also suits the threaded render loop (Phase 3 in
`QTQUICK_PORT.md`): scripts record on the GUI thread, the render thread
replays.

MuGLText maps onto TwkText: `bounds`, `size`, `writeAt` and the rest
are layout plus textured quads.

### Supported subset

Supported: the functions the bundled code uses (listed above), plus
cheap 2D neighbours (`glVertexPointer` / `glDrawArrays`, `glScissor`,
`glOrtho`, `glFrustum`, `gluPerspective`, `glLoadMatrix`, `glMultMatrix`,
`glClear`). Display lists can be added cheaply (record a sub-list) if
packages use them.

Not supported: lighting, evaluators, accumulation, selection / feedback
(`glRenderMode`), `glCopyPixels`, `glDrawPixels` (could map to a textured
quad later). Unsupported calls warn once per function and do nothing.

### Ordering

Render events that draw before or after the image, or between sources,
keep their position: the draw lists are recorded per event and the
backend replays each at the matching point of the frame.

### Size

Recorder and state tracking: roughly 1.5 to 2.5 thousand lines of C++.
Replay: a few hundred lines per backend (one on QRhi). Getting the
bundled packages identical: about one to two weeks, mostly line widths
and joins, stipple, attribute semantics and read back. Checked by
rendering the HUD and the bundled packages through both paths (GL direct
and record / replay on GL) and comparing images.

### Python with PyOpenGL

Packages that call PyOpenGL bypass MuGL and can't be emulated. Options:
port the two bundled ones to Mu drawing or QML, and for third-party
packages keep an optional offscreen GL context whose output is
composited through the existing interop. Decide when a non-GL backend is
close to shipping.

### Alternatives considered

- **Zink** (Mesa's OpenGL on Vulkan): runs any GL unchanged, but means
  shipping Mesa, and on macOS GL on Vulkan on Metal (MoltenVK). Heavy and
  fragile.
- **Keep GL for scripts only**: an offscreen GL context for MuGL and
  render events, composited by interop. Everything keeps working, but GL
  stays in the process indefinitely.
- **Drop MuGL**: the QML HUD (Phases 2 and 3 in `QTQUICK_PORT.md`)
  replaces the bundled Mu HUD anyway; third-party packages would break.
  Record / replay keeps them working at modest cost.

## Qt Quick and the GL context (notes)

- Qt Quick does not need a core profile. Its OpenGL backend (QRhi) works
  with OpenGL 2.1 / ES 2.0 and uses 3.x features when present; its
  shaders carry GLSL 1.20 and 1.50 variants. Not rechecked against the
  Qt 6.12 documentation.
- Qt Quick defaults to Metal on macOS, D3D 11 on Windows and OpenGL on
  Linux. `QQuickWindow::setGraphicsApi()` or `QSG_RHI_BACKEND` selects
  OpenGL or Vulkan instead.
- Vulkan on macOS goes through MoltenVK. Homebrew's Qt is built with
  Vulkan (`QT_FEATURE_vulkan`); the Vulkan loader and MoltenVK must be
  installed at run time.
- While the renderer is GL, the simplest host keeps Qt Quick on its
  native API and shows the GL render as a shared texture (IOSurface on
  macOS, Vulkan memory elsewhere), as `MetalView` and `VulkanView` do
  now. The GL context's profile then doesn't matter to Qt Quick, so the
  QtQuick host and the HUD replacement don't depend on each other.

## Order of Work

1. Phase 0 spikes from `QTQUICK_PORT.md`, in particular the shader
   translation one (sizes A5 and B).
2. A1 to A4: the interface with GL behind it, identical output. IPCore
   stops linking GL.
3. C: MuGL record / replay on the GL backend, compared against GL direct.
4. A5 and A6, then B: a second backend.

Steps 2 and 3 are useful on their own: they give the threaded render
loop a clean boundary even if the GL backend is the only one for a
while.

## Open Questions

- For a second backend: port the output devices (AJA, Blackmagic, NDI)
  to it, or have them read back from it?
- Do any third-party packages rely on MuGL features outside the
  supported subset (display lists, `glDrawPixels`)?
- Per-backend shader cache: where it lives, and how it is invalidated
  across releases.
- How `TextureOutputGroupIPNode` live views hand a texture to QML
  without a copy on each backend.
- 10-bit and HDR presentation on each candidate backend (MoltenVK
  colour spaces, QRhi swapchain formats).
