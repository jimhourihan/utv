# User Manual Typst Conversion: Status and Open Issues

Working notes for the experimental Typst version of the user manual. This is
still a test: the repo lead prefers Read the Docs (Sphinx), and this
conversion exists to compare the two approaches.

## Status

- All 18 chapters and appendices are converted from the Markdown files
  (`rv-user-manual-chapter-*.md`) to `rv-user-manual.NN.typ` /
  `rv-user-manual.X.typ`.
- The old Appendix H (Crash Reporting) was retired to
  `deprecated_docs/rv-user-manual.H.typ`, so the Typst appendices are now
  A–J: old I → H (PySide), old J → I (multichannel audio), old K → J (path
  localization). The Markdown appendix letters are unchanged.
- `rv-user-manual.typ` is the master file (title page, contents, all chapters
  and appendices). It builds to PDF and HTML for all three products.
- Every chapter file also builds on its own.
- Nothing is committed yet. The original Markdown files are untouched.

## Building

Use `build-docs.sh` in this directory; it holds all the `typst` command-line
details (root, features, product input), so if the invocation changes only the
script needs updating. It can be run from anywhere.

```sh
./build-docs.sh                     # all products, PDF + HTML, into ./build
./build-docs.sh -p openutv -f pdf   # one product, one format
./build-docs.sh -c 07               # one chapter (or appendix, e.g. -c H)
./build-docs.sh --check             # build every chapter alone, report failures
./build-docs.sh -h                  # all options
```

Output files are named `<product>-user-manual.pdf|html` (or
`<product>-chapter-NN...` with `-c`). `build/` is git-ignored.

For reference, the script runs `typst compile --root <repo root>
--input product=<product> [--features html --format html] <file> <out>`.
The root must be the repository root because the manual reads `docs/images/`
and `src/plugins/` (Appendix H).

## Files

| File | Purpose |
| --- | --- |
| `manual-lib.typ` | Product table, helper functions, `manual` template |
| `manual.css` | HTML stylesheet (read by the template) |
| `rv-user-manual.typ` | Master file |
| `build-docs.sh` | Builds PDF/HTML for any product; `--check` tests every chapter |
| `rv-user-manual.01.typ` … `.18.typ` | Chapters |
| `rv-user-manual.A.typ` … `.J.typ` | Appendices |
| `deprecated_docs/` | Retired chapters (builds on its own, not in the master) |
| `backup.typ`, `mathtest.typ` | Early experiments; can be deleted |

### Helpers in manual-lib.typ

- Product names: `#app`, `#appcmd`, `#ioapp`, `#iocmd`, `#lsapp`, `#lscmd`,
  `#pkgcmd`, `#pushapp`, `#pushcmd`, `#org`; `#a-app` / `#A-app` give
  "a UTV" / "an RV" with the right article.
- Conditional content: `#only-for("rv", "openrv")[...]`,
  `#if-feature("name")[...]` (no features are defined yet).
- `#key("Shift+C")`, `#menu(("File", "Open"))`, `#key-table(...)`;
  `#key-table(compact: true, ...)` uses a smaller font and tighter rows.
- `#appshell(...)`, `#ioshell(...)`, `#lsshell(...)`: shaded command blocks
  with a `shell>` prompt; lines starting with spaces are continuations.
- `#code-block(lang: none, ...)`: shaded block without a prompt; accepts
  strings or a raw block.
- `#inline-shell("...")`: inline code.
- `#img("file.png")`, `#image-row(...)`: screenshots from `docs/images/`;
  `image-row` keeps side-by-side images working in HTML (Typst drops `grid`).
  All screenshots share one pixel scale (see "Image sizing" below).
- `#xref(<label>)[Chapter 7]`: cross-chapter link; shows the fallback text
  when a chapter is built alone.
- Template: `#show: manual` (chapters), `manual.with(appendix: true)`,
  `manual.with(master: true)`.

## Decisions (resolved 2026-10-06)

1. **Appendix I (PySide examples) is obsolete.** Both listings are Python 2
   with PySide 1 / Qt4 imports; neither runs on UTV's Python 3.14 + PySide6.
   The second listing also defines `F` twice in `spinChanged`. The package
   that actually ships, `src/plugins/rv-packages/pyside_example/pyside_example.py`,
   is a different PySide6 + OpenGL example. Options: show the shipped file,
   port the old example, or drop the appendix. Currently converted verbatim.
2. **Appendix H (crash reporting) is probably wrong for UTV.** It says Breakpad
   writes `.dmp` files to `/tmp` on Linux. The only Breakpad mention in the
   source is a comment on a test crash command
   (`src/lib/ip/IPMu/CommandsModule.cpp:5237`); no Breakpad library appears to
   be built.
3. **Chapter 15 contradicts itself.** "Audio" under QuickTime says #app does
   not handle more than two audio channels; section 15.3 and Appendix J
   describe multichannel playback. Left as-is.
4. **Read the Docs vs Typst.** Still undecided. A test showed
   `pandoc -f typst -t rst` converts the manual well if given a simplified
   copy of `manual-lib.typ` (Pandoc doesn't support `context`, `target()`,
   `html.elem` or `sys.inputs`). Remaining gaps: keys come out as code rather
   than `:kbd:`, section numbering, curly-quote handling of `#app's`.

### ANSWERS from Human

1. Show the shipped example instead
2. Move appendix H to a subdir called "deprecated_docs" and remove
   references to it from other parts of the manual if they exist
3. Update chapter 15 to indicate multiple audio channels are supported
4. We'll need to target readthedocs but we can do this after everything
   else is completed

### What was done

1. **Done.** Appendix H (was I) now reads
   `src/plugins/rv-packages/pyside_example/pyside_example.py` at build time,
   minus its license header, so it can't go stale again. The first
   (standalone `py-interp`) example was ported to Python 3 / PySide6.
2. **Done.** The Typst file moved to `deprecated_docs/` and its include was
   removed from the master. No other Typst chapter referred to it. The
   Markdown original `rv-user-manual-chapter-h.md` was left in place because
   `docs/index.md` (the existing docs' toctree) lists it; remove both
   together if the old docs should drop it too.
3. **Done.** Chapter 15 now says #app and #ioapp handle mono, stereo and
   multichannel audio, pointing at Appendix I and `-audiochannels`.
4. **Pending** until the rest is finished.

## Values to verify

- **RV / OpenRV product entries** in `manual-lib.typ` (app names, commands,
  `org`, `prefs` paths) are guesses, marked TODO.
- **UTV preferences file on Windows**: `%APPDATA%\OpenUTV\UTV\OpenUTV\UTV.ini`
  was worked out from the QSettings code, not checked on a real machine.
  macOS and Linux paths should be right.
- **Log file paths** (chapter 3) now use the `OpenUTV` organization name
  (`CMakeLists.txt:110`) instead of `ASWF`.
- **macOS bundle path** in chapter 3 (`/Applications/#app.app/...`) gives
  `OpenRV.app` for OpenRV, which may be wrong.
- **Chapter 7 inversion matrix**: the 1s were moved from the bottom row to the
  last column (R′ = 1 − R), matching the other matrices. Check against a
  trusted source.
- **Chapter 8 3D LUT memory**: changed "64³ × 4 bytes" to "64³ × 3 × 4 bytes"
  so it matches the stated 3 MB.
- **Chapter 2**: "Refer to the #app README to learn how to build and install"
  is wrong for commercial RV; probably needs `#only-for`.

## Code issues found along the way

- `src/plugins/rv-packages/pyside_example/pyside_example.py`: `spinChanged`
  defines `F` twice; the first (which uses an undefined `p`) is dead code.
  Since Appendix H now shows this file, fixing it fixes the manual.
- `src/lib/image/IOdpx/IOdpx.cpp:512`: help text lists `source/input_dev`
  twice; the second should be `source/input_serial` (accepted at line 1886).
- `src/bin/apps/rvpush/main.cpp:16`: usage text says `rvpush`, but the UTV
  binary is `utvpush`.

## Dated content to review

- Chapter 3: Windows advice (Cygwin, tcsh, command.com).
- Chapter 6: Frame Packed mode sections (SwitchResX, OS X 10.7, Quadro 4000,
  old `xorg.conf` nVidia options).
- Chapter 12: SpectronIQ displays.
- Chapter 13: Hamachi link replaced with vpn.net; may not be worth keeping.
- Appendix A: tuning table from CentOS 6/7 era hardware, several "TBD" cells.
- Appendix B: NVIDIA README text (Quadro, TwinView, Xinerama).
- Appendix D: `rv_this.py` link pointed to a forum post outside the manual;
  now refers to the Nuke integration package docs.
- Appendix E: CentOS 4.6 / Fedora Core 4, ALSA Old/Safe modules (still built
  on Linux).

## Known limitations

- Combined HTML is ~10 MB because every screenshot is embedded as base64.
  Splitting per chapter or serving images as separate files would fix that.
- A chapter built alone is numbered "1." (appendices "A"), and its
  cross-chapter links show fallback text; only the master build is fully
  correct.
- Typst HTML export is still marked experimental (Typst 0.15.1 used here).

## Typst gotchas hit during conversion

- `;` right after a code expression is swallowed: write `#raw(app)\;`.
- `#app-specific` reads as a variable named `app-specific`; write
  `#(app)-specific`.
- In markup, `_ * @ < # $` and a line starting with `- `, `+ `, `1. `, `= `
  have meaning; put such text in backticks or a string.
- In math, multi-letter words are variables: write `"where"`.
- `grid` and `rect` are dropped in HTML export; use `html.elem` wrappers.

## Next steps

- Resolve the decisions above.
- Have someone review the converted chapters against the old manual.
- Decide what happens to the Markdown originals and `backup.typ`/`mathtest.typ`.
- If staying with Typst: add a CI job that builds all three products, and
  consider per-chapter HTML output.

## Additional Notes from Human 

- Images (figures) scale non-uniformly in the HTML output. Need to maintain
  aspect ratio.
- Consider adding margins for images to make them look polished
- Make the typst PDF tables look more refined (thinner lines, possibly grey
  instead of black and differentiated headers)
- HTML tables need some styling
- Should large tables of key bindings use samller fonts? They're acting as
  reference material not a user manual. Perhaps they shouldn't even be
  there since the app now has an interactive mechanism which serves the
  same purpose.
- LOW PRIORITY: Access difficulty of inserting some javascript to make the
  TOC in the HTML easily accessible anywhere.
- VERY LOW PRIORITY: Access difficulty of creating a baked javascript
  indexing facility for the standalone HTML docs. By this I mean a very
  simple JS interface that is a single live keyboard input which lists
  possible links to whatever the user types. E.g. they type "709" and links
  to everyplace Rec.709 is referenced are displayed. If this is doable and
  not a huge project perhaps it could span both the user and reference
  manuals. I'm thinking something like emacs' apropos help. Another
  possiblity is putting something like this in the app itself.

### Responses to the additional notes (2026-10-06)

- **Images scaling non-uniformly in HTML — fixed.** Typst writes fixed
  `width`/`height` attributes on each `<img>`; `max-width: 100%` shrank only
  the width. `manual.css` now sets `height: auto` on figure images.
- **Image margins — done.** Figures get more space above and below (PDF and
  HTML), smaller captions, and a thin light-grey border (0.5pt in PDF, 1px in
  HTML). Remove the `stroke` in `img()` / the `border` in `manual.css` if the
  border isn't wanted.
- **PDF tables — done.** Default tables now use thin grey horizontal rules
  only, a slightly darker rule under the header, and a light grey header
  row. Key tables keep their teal style.
- **HTML tables — done.** `manual.css` now styles all tables the same way
  (collapsed borders, light rules, shaded header).
- **Large key binding tables — smaller font done; removal is your call.**
  The two big hotkey tables in chapter 4 use `key-table(compact: true)`.
  Recommendation: drop them and point readers at Help → Show Current
  Bindings / Describe Key Binding. The bindings are defined in `rvui.mu`,
  so a hand-copied table will drift, and the in-app list is always right
  (including user overrides). The short tables of the most useful keys in
  chapters 1 and 4 are worth keeping.
- **Floating HTML TOC (low priority) — easy, roughly an hour or two.**
  Typst already emits the contents as `<nav role="doc-toc">`. Making it a
  fixed sidebar is about 20 lines of CSS, plus about 10 lines of JS for a
  show/hide button on narrow screens; the template can inject both with
  `html.elem("script", ...)`. No build-step changes needed.
- **Live search index (very low priority) — moderate, about half a day for
  one manual.** Simplest version needs no build step: JS that, on page
  load, walks the headings, paragraphs and table cells, gives every heading
  an `id` (Typst only adds ids to labelled headings), and filters that list
  as the user types (e.g. "709" → links to every Rec. 709 mention, grouped
  under the nearest heading). Spanning the user and reference manuals
  needs a shared index: either a small script that extracts headings and
  text from both builds into one JSON file, or one HTML that fetches the
  other. An in-app version could reuse the same JSON in the app's help
  browser. Note `typst query` is deprecated in Typst 0.15, so a build-time
  extractor should parse the HTML instead.

### Also changed

- `manual.css`: `span.menuitem` used `weight: bold`, which isn't valid CSS,
  so menu items were never bold in HTML; now `font-weight: bold`.

### Image sizing (2026-10-06)

Screenshots now share one pixel scale, so small menus no longer look
oversized next to full-window captures. Sizes use *logical* pixels: pixel
count adjusted for the file's DPI, so the 144 DPI Retina captures count at
half size.

- **PDF:** one logical pixel = `image-scale` pt (0.5, set in `manual-lib.typ`).
  An image only shrinks further if it's wider than its space (the page, or
  its `image-row` cell). Before, every image was 1 pt per pixel, so large
  screenshots were shrunk to fit the page while small ones stayed full size.
  The manual dropped from 167 to 155 pages.
- **HTML:** one logical pixel = one CSS pixel (1:1 on screen), shrinking only
  on narrow windows. Before, Retina captures showed at double size and
  side-by-side images were stretched to equal widths.
- Images are capped at 90% of the available width (`image-max-width` in
  `manual-lib.typ`, `max-width: 90%` in `manual.css`), leaving a 5% margin
  each side so wide screenshots don't run the full text width. Images are
  centered in both PDF and HTML.
- `img(..., width: ...)` still overrides the rule for a single image. The two
  `width: 60%` overrides in chapter 4 were removed.
- Trade-off: at 0.5, UI text in screenshots prints at roughly 6–7 pt. Raise
  `image-scale` for larger small images (big screenshots stay page-width
  either way, so they'd become relatively smaller again).


### HTML text size (2026-10-06)

- Base text size raised to 112.5% (18px at the browser default), at the
  top of `manual.css`. Line spacing is left at the browser default.
- Browser page zoom also enlarges images; that's expected browser behavior
  and was left alone (Safari's View → Zoom Text Only avoids it).
- Not done: a max-width text column (~50em) would shorten long lines in wide
  windows, but images wider than the column would shrink to fit it.
- Some source images were captured large or upscaled; the only real fix is
  recapturing them (`img(..., width: ...)` can shrink one as a stopgap).
