# msdfgen

Vendored copy of [msdfgen](https://github.com/Chlumsky/msdfgen) by Viktor
Chlumský, MIT licence (`LICENSE.txt`). OpenUTV uses it to generate MTSDF
glyphs for text drawn by the renderer.

It lives here, next to TwkText, for now. Third-party code usually goes in
`src/pub` (the openrv-pub repository), so it should probably move there
eventually; it is in this repository to keep the branch to one repository
while the work is in progress.

- Version: 1.13 (`v1.13` tag), from
  `https://github.com/Chlumsky/msdfgen/archive/refs/tags/v1.13.tar.gz`
  (SHA-256 `93cd1ad8918c1a78c5c96e82d4f4c77f0eb86c2e7e8579a0967e54196c4b7167`).
- Included: `msdfgen.h`, all of `core/`, and `ext/import-font.{h,cpp}`
  (glyph outlines through FreeType). Not included: the command line tool,
  the SVG and PNG extensions, and Skia-based geometry resolution.
- The files are unmodified. `.clang-format` here turns formatting off so
  the copy stays identical to upstream.
- `CMakeLists.txt` replaces upstream's build. Upstream generates
  `msdfgen-config.h`; this build defines `MSDFGEN_PUBLIC` and
  `MSDFGEN_EXT_PUBLIC` as empty instead, which `core/base.h` checks first.

To update: replace `msdfgen.h`, `core/` and `ext/import-font.*` with the
new release's files, and update the version and checksum above.
