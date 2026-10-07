# User Manual in Typst: Working Notes

Shared notes (human + Claude) for the Typst version of the user manual. The
whole manual is converted and builds to PDF and HTML for all three products.
All manuals in `docs/rv-manuals/` will be converted the same way; this one
is the first and sets the conventions. Read the Docs output will be generated
from the Typst source in a later phase. This repo is a dev/testing fork, so
broken links or docs builds during the conversion are acceptable.

## Building

`build-docs.sh` holds all the `typst` command-line details; update it, not
the docs, if the invocation changes. It runs from any directory.

```sh
./build-docs.sh                     # all products, PDF + HTML, into ./build
./build-docs.sh -p openutv -f pdf   # one product, one format
./build-docs.sh -c 07               # one chapter or appendix (e.g. -c H)
./build-docs.sh --check             # build every chapter alone, report failures
```

The Typst root must be the repository root: the manual reads `docs/images/`
and Appendix H reads `src/plugins/rv-packages/pyside_example/pyside_example.py`.
Products: `rv`, `openrv`, `openutv` (default). `build/` is git-ignored.

## Layout

| File | Purpose |
| --- | --- |
| `rv-user-manual.typ` | Master: title, contents, includes every chapter |
| `rv-user-manual.01.typ` … `.18.typ` | Chapters (each also builds alone) |
| `rv-user-manual.A.typ` … `.J.typ` | Appendices |
| `manual-lib.typ` | Product table, helpers, `manual` template |
| `manual.css` | HTML stylesheet (embedded by the template); pins the contents as a sidebar on wide screens |
| `manual.js` | Contents sidebar behavior: current-section highlight, folding chapters, overlay on narrow screens (embedded by the template; does nothing without contents) |
| `deprecated_docs/rv-user-manual.H.typ` | Retired Typst appendix (old H, Crash Reporting) |
| `deprecated_docs/rv-user-manual-chapter-*.md` | Original Markdown manual; still published by the Sphinx build (`docs/index.md`) until the Typst→RTD output replaces it |
| `backup.typ`, `mathtest.typ` | Early experiments; can be deleted |

Typst appendix letters differ from the Markdown ones after H was retired:
Markdown I/J/K = Typst H/I/J.

## Conventions

- **Product names:** `#app`, `#appcmd`, `#ioapp`, `#iocmd`, `#lsapp`,
  `#lscmd`, `#pkgcmd`, `#pushapp`, `#pushcmd`, `#org`; `#a-app` / `#A-app`
  for "a UTV" / "an RV". Per-product values live in `products` in
  `manual-lib.typ`.
- **Conditional text:** `#only-for("rv", "openrv")[...]`,
  `#if-feature("name")[...]` (no features defined yet).
- **UI:** `#key("Shift+C")`, `#menu(("File", "Open"))`,
  `#key-table(compact: true, ...)` for long reference tables.
- **Code:** `#appshell(...)` / `#ioshell(...)` / `#lsshell(...)` add a
  `shell>` prompt (lines starting with spaces are continuations);
  `#code-block(lang: none, ...)` has no prompt; `#inline-shell("...")` inline.
- **Images:** always `#img("file.png")`, and `#image-row(...)` for side by
  side (Typst drops `grid` in HTML). All screenshots share one pixel scale,
  using logical pixels (adjusted for DPI, so 144 DPI captures count half):
  PDF = `image-scale` pt per pixel (0.5), HTML = 1 CSS px per pixel. Images
  never enlarge, only shrink to fit, are capped at 90% of the available
  width (`image-max-width`), and are centered. `img(..., width: ...)`
  overrides for one image.
- **Cross-references:** `@label` within a chapter; `#xref(<label>)[Chapter 7]`
  across chapters (shows the fallback text when a chapter is built alone).
  Chapter labels are `<ch-…>`, appendix labels `<app-…>`.
- **Templates:** chapters `#show: manual`, appendices
  `manual.with(appendix: true)`, master `manual.with(master: true)`.
- **Styling knobs:** PDF table look is the `set table(...)` in the template;
  HTML base text size is 112.5% at the top of `manual.css` (browser default
  line spacing).

### Typst gotchas

- `;` right after a code expression is swallowed: write `#raw(app)\;`.
- `#app-specific` reads as variable `app-specific`: write `#(app)-specific`.
- A multi-line `#let x = foo()` chain needs parentheses, or the line break
  ends the expression and the rest prints as text.
- In markup, `_ * @ < # $` and lines starting with `- `, `+ `, `1. `, `= `
  are syntax; put such text in backticks or a string.
- In math, multi-letter words are variables: write `"where"`.
- `grid` and `rect` are dropped in HTML export; use `html.elem` wrappers.
- `typst query` is deprecated as of Typst 0.15.

## Read the Docs output

A working proof of concept is in `docs/rv-manuals/rtd/` (see its README):
`./build-rtd.sh` converts every chapter with Pandoc and builds a Read the
Docs-themed Sphinx site with no warnings (`-W`). Keys, menus, cross-chapter
refs, figures, image sizing, rowspan tables, math, footnotes and term lists
all come through. No chapter file needed changing.

Rule this imposes: chapter files use only library helpers and plain Typst
markup (no `context`, `target()`, `html.elem`, `measure`, `layout`, `state`,
`query`). Any new helper in `manual-lib.typ` needs a plain twin in
`rtd/manual-lib-pandoc.typ`.

Both manuals are wired in. Appendices are lettered (A, A.1, Table A.1),
side-by-side image figures are numbered (`multi-figure` in
`rtd/manual_ext.py`), and figure/table numbers restart per chapter. Sphinx
versions are pinned in `rtd/requirements.txt`. `build-docs.sh --check` also
runs the RTD build (when `pandoc` and `sphinx-build` are installed), and the
`docs-check` job in `pr-checks.yml` runs it on PRs that touch the manuals.

Remaining work: hosting. There's no OpenUTV site on Read the Docs yet, and
the repo's `.readthedocs.yaml` is OpenRV's (it builds the old Markdown site
from `docs/conf.py`, which crashes in search indexing with a fresh
`docs/requirements.txt` install: `KeyError: 'classes'`). `rtd/README.md`
has a suggested replacement config.

## Open items

### Decisions

- **Big key-binding tables (chapter 4):** currently compact. Recommendation:
  remove them and point readers at Help → Show Current Bindings / Describe
  Key Binding, since the bindings live in `rvui.mu` and a copied table will
  drift. Keep the short "most useful keys" tables.
- **Old Appendix H Markdown:** `deprecated_docs/rv-user-manual-chapter-h.md`
  is still listed in `docs/index.md`; remove both if the old docs should drop
  it too.
- **Retiring the Markdown manual:** when the Typst→RTD output is ready,
  replace the `deprecated_docs/rv-user-manual-chapter-*` entries in
  `docs/index.md` and repoint the links in `docs/rv-manuals/rv-luts.md`,
  `rv-reference-manual-chapter-sixteen.md`, `docs/rv-packages/rv-nuke-integration.md`
  and the two GitHub URLs in `packages/rv/README.regfiles`.
- **What happens to the Markdown originals** once the Typst version is
  adopted.

### Values to verify

- RV / OpenRV entries in `products` (`manual-lib.typ`): app names, commands,
  `org`, `prefs` paths are guesses (marked TODO).
- UTV preferences file on Windows (`%APPDATA%\OpenUTV\UTV\OpenUTV\UTV.ini`):
  derived from the QSettings code, not checked on a machine.
- Chapter 3: log paths use org `OpenUTV`; the macOS bundle path
  (`/Applications/#app.app/...`) gives `OpenRV.app` for OpenRV, possibly
  wrong.
- Chapter 7 inversion matrix: 1s moved from the bottom row to the last column
  (R′ = 1 − R) to match the other matrices; check against a trusted source.
- Chapter 8 3D LUT memory: changed to "64³ × 3 × 4 bytes" to match the stated
  3 MB.
- Chapter 2: "Refer to the #app README to build and install" is wrong for
  commercial RV; probably needs `#only-for`.

### Code issues found (not fixed)

- `src/plugins/rv-packages/pyside_example/pyside_example.py`: `spinChanged`
  defines `F` twice; the first (using an undefined `p`) is dead code.
  Appendix H shows this file, so fixing it fixes the manual.
- `src/lib/image/IOdpx/IOdpx.cpp:512`: help text lists `source/input_dev`
  twice; the second should be `source/input_serial` (accepted at line 1886).
- `src/bin/apps/rvpush/main.cpp:16`: usage text says `rvpush`; the UTV binary
  is `utvpush`.

### Dated content

- Chapter 3: Windows advice (Cygwin, tcsh, command.com).
- Chapter 6: Frame Packed mode (SwitchResX, OS X 10.7, Quadro 4000, old
  `xorg.conf` nVidia options).
- Chapter 12: SpectronIQ displays.
- Chapter 13: Hamachi/vpn.net suggestion.
- Appendix A: CentOS 6/7-era tuning table with "TBD" cells.
- Appendix B: NVIDIA README text (Quadro, TwinView, Xinerama).
- Appendix D: `rv_this.py` now just refers to the Nuke integration package
  docs.
- Appendix E: CentOS 4.6 / Fedora Core 4, ALSA Old/Safe modules (still built
  on Linux).
- Some screenshots were captured large or upscaled; only recapturing fixes
  them (`img(..., width: ...)` can shrink one meanwhile).

### Possible improvements

- Floating HTML TOC: Typst already emits `<nav role="doc-toc">`; ~20 lines
  of CSS plus a small show/hide script injected via `html.elem("script")`.
  Estimate an hour or two.
- Live search for the HTML ("709" → every Rec. 709 mention): client-side JS
  indexing headings/paragraphs/table cells at page load, adding ids to
  headings (Typst only ids labelled ones). About half a day for one manual;
  spanning user + reference manuals needs a shared JSON index (extract from
  the built HTML). Could also feed an in-app help search.
- Max-width text column (~50em) for HTML: shorter lines in wide windows, but
  images wider than the column would shrink to fit it.
- Per-chapter HTML or external image files: the combined HTML is ~10 MB
  because screenshots are embedded as base64.
- CI job running `./build-docs.sh --check` and the full build.

## Known limitations

- A chapter built alone is numbered "1." (appendices "A") and its
  cross-chapter links show fallback text; only the master build is fully
  correct.
- Typst HTML export is experimental (Typst 0.15.1 used).
- Browser zoom enlarges images along with text (expected browser behavior).

## Reference manual (converted 2026-10-07)

`rv-reference-manual/rv-reference-manual.typ` (master) and chapters
`.01.typ` … `.17.typ`, converted from the Markdown (scripted first pass, then
hand review of every chapter). Builds with `./build-docs.sh -m reference`, and
`rtd/build-rtd.sh` puts both manuals in one Sphinx site (clean under `-W`,
including reference → user manual links). Markdown originals are in
`rv-reference-manual/deprecated_docs/`; `docs/index.md` points there.

Conventions specific to this manual:
- Labels are prefixed `ref-` (chapters `ref-ch-…`) so they can't collide with
  user-manual labels in a combined site.
- Code blocks: Mu, GTO and plain listings are `lang: none` (no highlighter
  exists for Mu); Python, GLSL, bash, XML, JavaScript are tagged.
- `rvpkg`/`rv`/`rvio` command examples use `#shell(pkgcmd, …)` /
  `#appshell` / `#ioshell` so the command follows the product. File
  extensions (`.rvpkg`, `.rv`) and PACKAGE fields (`rv`, `openrv`) stay
  literal.
- API tables with long signatures use proportional column widths (e.g.
  `(2fr, 1fr, 1fr, 2.5fr)`); `auto` columns let one long cell crush the
  description column.

Flagged for the human:
- Ch 16 RVLayoutGroup: description is a copy of RVSourceGroup's (original error).
- Ch 13 "Using rvNetwork.py": section was never written ("document here"); placeholder text now says so.
- Ch 14: QRegExp link points to Qt 4.8 docs.
- Ch 4: PySide2 example updated to PySide6; `createMode()` indentation bug fixed.
- `src/plugins/rv-packages/pyside_example`: same dead `F` in `spinChanged` (see code issues above).
