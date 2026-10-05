// The complete user manual. Build with, e.g.:
//   typst compile --root ../.. --input product=openutv rv-user-manual.typ
//   typst compile --root ../.. --features html --format html rv-user-manual.typ
// Each chapter file can still be built on its own.

#import "manual-lib.typ": *
#show: manual.with(master: true)

#set document(title: app + " User Manual")

#context if target() != "html" {
    set page(numbering: none)
    align(center + horizon)[
        #text(size: 32pt, weight: "bold")[#app User Manual]
    ]
    pagebreak()
}

#outline(depth: 2)

// page numbers for the body (PDF only; pages don't exist in HTML)
#show: body => context if target() == "html" { body } else {
    set page(numbering: "1")
    body
}

#include "rv-user-manual.01.typ"
#include "rv-user-manual.02.typ"
#include "rv-user-manual.03.typ"
#include "rv-user-manual.04.typ"
#include "rv-user-manual.05.typ"
#include "rv-user-manual.06.typ"
#include "rv-user-manual.07.typ"
#include "rv-user-manual.08.typ"
#include "rv-user-manual.09.typ"
#include "rv-user-manual.10.typ"
#include "rv-user-manual.11.typ"
#include "rv-user-manual.12.typ"
#include "rv-user-manual.13.typ"
#include "rv-user-manual.14.typ"
#include "rv-user-manual.15.typ"
#include "rv-user-manual.16.typ"
#include "rv-user-manual.17.typ"
#include "rv-user-manual.18.typ"

// Appendices restart the heading counter so the first one is "A".
#counter(heading).update(0)

#include "rv-user-manual.A.typ"
#include "rv-user-manual.B.typ"
#include "rv-user-manual.C.typ"
#include "rv-user-manual.D.typ"
#include "rv-user-manual.E.typ"
#include "rv-user-manual.F.typ"
#include "rv-user-manual.G.typ"
#include "rv-user-manual.H.typ"
#include "rv-user-manual.I.typ"
#include "rv-user-manual.J.typ"
#include "rv-user-manual.K.typ"
