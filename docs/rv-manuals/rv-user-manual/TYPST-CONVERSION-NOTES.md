# User Manual Typst Conversion: Status and Open Issues

Working notes for the experimental Typst version of the user manual. This is
still a test: the repo lead prefers Read the Docs (Sphinx), and this
conversion exists to compare the two approaches.

## Status

- All 18 chapters and appendices A–K are converted from the Markdown files
  (`rv-user-manual-chapter-*.md`) to `rv-user-manual.NN.typ` /
  `rv-user-manual.X.typ`.
- `rv-user-manual.typ` is the master file (title page, contents, all chapters
  and appendices). It builds to PDF and HTML for all three products.
- Every chapter file also builds on its own.
- Nothing is committed yet. The original Markdown files are untouched.

## Building

Run from `docs/rv-manuals/rv-user-manual/`. `--root ../..` is required so
Typst can read the screenshots in `docs/images/`.

```sh
# whole manual, PDF
typst compile --root ../.. --input product=openutv rv-user-manual.typ
# whole manual, HTML (native MathML for equations)
typst compile --root ../.. --features html --format html rv-user-manual.typ
# one chapter
typst compile --root ../.. rv-user-manual.07.typ
```

`product` is one of `rv`, `openrv`, `openutv` (default).

## Files

| File | Purpose |
| --- | --- |
| `manual-lib.typ` | Product table, helper functions, `manual` template |
| `manual.css` | HTML stylesheet (read by the template) |
| `rv-user-manual.typ` | Master file |
| `rv-user-manual.01.typ` … `.18.typ` | Chapters |
| `rv-user-manual.A.typ` … `.K.typ` | Appendices |
| `backup.typ`, `mathtest.typ` | Early experiments; can be deleted |

### Helpers in manual-lib.typ

- Product names: `#app`, `#appcmd`, `#ioapp`, `#iocmd`, `#lsapp`, `#lscmd`,
  `#pkgcmd`, `#pushapp`, `#pushcmd`, `#org`; `#a-app` / `#A-app` give
  "a UTV" / "an RV" with the right article.
- Conditional content: `#only-for("rv", "openrv")[...]`,
  `#if-feature("name")[...]` (no features are defined yet).
- `#key("Shift+C")`, `#menu(("File", "Open"))`, `#key-table(...)`.
- `#appshell(...)`, `#ioshell(...)`, `#lsshell(...)`: shaded command blocks
  with a `shell>` prompt; lines starting with spaces are continuations.
- `#code-block(lang: none, ...)`: shaded block without a prompt; accepts
  strings or a raw block.
- `#inline-shell("...")`: inline code.
- `#img("file.png")`, `#image-row(...)`: screenshots from `docs/images/`;
  `image-row` keeps side-by-side images working in HTML (Typst drops `grid`).
- `#xref(<label>)[Chapter 7]`: cross-chapter link; shows the fallback text
  when a chapter is built alone.
- Template: `#show: manual` (chapters), `manual.with(appendix: true)`,
  `manual.with(master: true)`.

## Decisions needed

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
