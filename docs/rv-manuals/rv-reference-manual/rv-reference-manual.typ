// The complete reference manual. Build it with ../build-docs.sh -m reference
// (see -h), which holds the typst command-line details. Each chapter file can
// still be built on its own (../build-docs.sh -m reference -c 07).

#import "../common/manual-lib.typ": *
#show: manual.with(master: true)

#set document(title: app + " Reference Manual")

#context if target() != "html" {
    set page(numbering: none)
    align(center + horizon)[
        #text(size: 32pt, weight: "bold")[#app Reference Manual]
    ]
    pagebreak()
}

#outline(depth: 2)

// page numbers for the body (PDF only; pages don't exist in HTML)
#show: body => context if target() == "html" { body } else {
    set page(numbering: "1")
    body
}

#include "rv-reference-manual.01.typ"
#include "rv-reference-manual.02.typ"
#include "rv-reference-manual.03.typ"
#include "rv-reference-manual.04.typ"
#include "rv-reference-manual.05.typ"
#include "rv-reference-manual.06.typ"
#include "rv-reference-manual.07.typ"
#include "rv-reference-manual.08.typ"
#include "rv-reference-manual.09.typ"
#include "rv-reference-manual.10.typ"
#include "rv-reference-manual.11.typ"
#include "rv-reference-manual.12.typ"
#include "rv-reference-manual.13.typ"
#include "rv-reference-manual.14.typ"
#include "rv-reference-manual.15.typ"
#include "rv-reference-manual.16.typ"
#include "rv-reference-manual.17.typ"
