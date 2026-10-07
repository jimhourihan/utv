// Interactive contents for the HTML manuals, embedded by manual() in
// manual-lib.typ. manual.css pins the contents as a sidebar on wide screens;
// this script adds the rest:
//   - the section you are reading is highlighted and kept in view
//   - each chapter's sections fold away; the current chapter is open
//   - on narrow screens a "Contents" button shows the list as an overlay
// It only relies on Typst's <nav role="doc-toc">, its nested <ol>s and the
// links' #ids, and does nothing on pages without contents (single
// chapters). Without JavaScript the contents is a plain list.
// Typst puts the script at the top of <body>, before the contents exists,
// so wait for the whole page.
document.addEventListener("DOMContentLoaded", function () {
    "use strict";
    const nav = document.querySelector('nav[role="doc-toc"]');
    if (!nav) return;
    document.documentElement.classList.add("toc-js");

    // heading element -> its contents link, in document order
    const entries = [];
    for (const a of nav.querySelectorAll('a[href^="#"]')) {
        const target = document.getElementById(decodeURIComponent(a.hash.slice(1)));
        if (target) entries.push({ target, link: a });
    }
    if (entries.length === 0) return;

    // Fold: a toggle in front of every entry that has sub-entries.
    // Chapters the reader opened by hand stay open.
    const openedByHand = new Set();
    for (const li of nav.querySelectorAll("li")) {
        if (!li.querySelector(":scope > ol")) continue;
        li.classList.add("toc-folder", "toc-closed");
        const button = document.createElement("button");
        button.type = "button";
        button.className = "toc-toggle";
        button.setAttribute("aria-label", "Show or hide sections");
        button.addEventListener("click", function () {
            const closed = li.classList.toggle("toc-closed");
            if (closed) openedByHand.delete(li); else openedByHand.add(li);
            button.setAttribute("aria-expanded", String(!closed));
        });
        button.setAttribute("aria-expanded", "false");
        li.prepend(button);
    }

    // Narrow screens: a button that shows the contents as an overlay.
    const show = document.createElement("button");
    show.type = "button";
    show.className = "toc-show";
    show.textContent = "Contents";
    show.addEventListener("click", function () {
        document.documentElement.classList.toggle("toc-overlay");
        current = null;                   // now visible: scroll to it again
        update();
    });
    document.body.append(show);
    nav.addEventListener("click", function (e) {
        if (e.target.closest("a")) document.documentElement.classList.remove("toc-overlay");
    });

    // Highlight: the current section is the last heading above the top of
    // the window. Recomputed at most once per frame while scrolling.
    let current = null;
    function update() {
        const top = 80;
        let lo = 0, hi = entries.length - 1, found = 0;
        while (lo <= hi) {                // binary search: headings are in order
            const mid = (lo + hi) >> 1;
            if (entries[mid].target.getBoundingClientRect().top <= top) {
                found = mid; lo = mid + 1;
            } else {
                hi = mid - 1;
            }
        }
        const entry = entries[found];
        if (entry === current) return;
        if (current) current.link.classList.remove("toc-current");
        current = entry;
        entry.link.classList.add("toc-current");

        // open the chapters containing it, close the others
        const chain = new Set();
        for (let li = entry.link.closest("li"); li; li = li.parentElement.closest("li")) chain.add(li);
        for (const li of nav.querySelectorAll(".toc-folder")) {
            const open = chain.has(li) || openedByHand.has(li);
            li.classList.toggle("toc-closed", !open);
            li.querySelector(":scope > .toc-toggle").setAttribute("aria-expanded", String(open));
        }

        // keep it in view inside the sidebar without scrolling the page
        const n = nav.getBoundingClientRect(), l = entry.link.getBoundingClientRect();
        if (l.top < n.top || l.bottom > n.bottom) {
            nav.scrollTop += l.top - n.top - n.height / 3;
        }
    }
    let pending = false;
    window.addEventListener("scroll", function () {
        if (pending) return;
        pending = true;
        requestAnimationFrame(function () { pending = false; update(); });
    }, { passive: true });
    window.addEventListener("resize", update);
    update();
});
