#!/usr/bin/env bash
#
# Build the Typst user manual to PDF and/or HTML.
#
# All typst invocation details (root, features, inputs) live here, so if the
# command line changes only this script needs updating.
#
#   ./build-docs.sh                    all products, PDF + HTML, into ./build
#   ./build-docs.sh -p openutv -f pdf  one product, PDF only
#   ./build-docs.sh -c 07              just chapter 7 (all products/formats)
#   ./build-docs.sh --check            build every chapter alone, report errors
#
set -euo pipefail

HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
# Repository root: Typst may only read files below --root, and the manual reads
# docs/images/ and src/plugins/ (Appendix H).
ROOT="$(cd "$HERE/../../.." && pwd)"

ALL_PRODUCTS=(openutv openrv rv)
PRODUCTS=()
FORMAT=all
OUTDIR="$HERE/build"
CHAPTER=""
CHECK=0

usage() {
    cat <<EOF
usage: $(basename "$0") [options]

  -p, --product NAME   rv, openrv or openutv (repeatable or comma separated;
                       default: all)
  -f, --format FMT     pdf, html or all (default: all)
  -o, --outdir DIR     output directory (default: $HERE/build)
  -c, --chapter ID     build one chapter/appendix file instead of the whole
                       manual, e.g. 07 or H
      --check          build every chapter and appendix on its own, in both
                       formats, and report any that fail (no output kept)
  -h, --help           show this help
EOF
}

while [[ $# -gt 0 ]]; do
    case "$1" in
        -p|--product)  IFS=, read -r -a p <<< "$2"; PRODUCTS+=("${p[@]}"); shift 2 ;;
        -f|--format)   FORMAT="$2"; shift 2 ;;
        -o|--outdir)   OUTDIR="$2"; shift 2 ;;
        -c|--chapter)  CHAPTER="$2"; shift 2 ;;
        --check)       CHECK=1; shift ;;
        -h|--help)     usage; exit 0 ;;
        *)             echo "unknown option: $1" >&2; usage >&2; exit 2 ;;
    esac
done

[[ ${#PRODUCTS[@]} -eq 0 ]] && PRODUCTS=("${ALL_PRODUCTS[@]}")

for p in "${PRODUCTS[@]}"; do
    case "$p" in rv|openrv|openutv) ;; *) echo "unknown product: $p" >&2; exit 2 ;; esac
done

case "$FORMAT" in
    pdf)  FORMATS=(pdf) ;;
    html) FORMATS=(html) ;;
    all)  FORMATS=(pdf html) ;;
    *)    echo "unknown format: $FORMAT" >&2; exit 2 ;;
esac

if ! command -v typst >/dev/null 2>&1; then
    echo "typst not found; install it (e.g. brew install typst)" >&2
    exit 1
fi

# compile SOURCE OUTPUT FORMAT PRODUCT
compile() {
    local src="$1" out="$2" fmt="$3" product="$4"
    local args=(compile --root "$ROOT" --input "product=$product")
    if [[ "$fmt" == html ]]; then
        args+=(--features html --format html)
    fi
    typst "${args[@]}" "$src" "$out"
}

cd "$HERE"

if [[ $CHECK -eq 1 ]]; then
    tmp="$(mktemp -d)"
    trap 'rm -rf "$tmp"' EXIT
    failures=0
    for src in rv-user-manual.[0-9][0-9].typ rv-user-manual.[A-Z].typ deprecated_docs/*.typ; do
        for fmt in pdf html; do
            if ! compile "$src" "$tmp/out.$fmt" "$fmt" openutv >"$tmp/log" 2>&1; then
                echo "FAIL  $src ($fmt)"
                sed 's/^/      /' "$tmp/log"
                failures=$((failures + 1))
            fi
        done
    done
    if [[ $failures -eq 0 ]]; then
        echo "all chapters build (pdf + html)"
    else
        echo "$failures build(s) failed" >&2
        exit 1
    fi
    exit 0
fi

if [[ -n "$CHAPTER" ]]; then
    SRC="rv-user-manual.$CHAPTER.typ"
    [[ -f "$SRC" ]] || { echo "no such chapter file: $SRC" >&2; exit 2; }
    STEM="chapter-$CHAPTER"
else
    SRC="rv-user-manual.typ"
    STEM="user-manual"
fi

mkdir -p "$OUTDIR"
for product in "${PRODUCTS[@]}"; do
    for fmt in "${FORMATS[@]}"; do
        out="$OUTDIR/$product-$STEM.$fmt"
        echo "building $out"
        compile "$SRC" "$out" "$fmt" "$product"
    done
done
