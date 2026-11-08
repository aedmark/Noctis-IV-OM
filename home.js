/* Noctis IV OM — handbook enhancements.
   Everything here is optional: the page reads and navigates without JavaScript. */

(function () {
    "use strict";

    document.documentElement.classList.add("js");

    function legacyCopy(text) {
        var area = document.createElement("textarea");
        area.value = text;
        area.setAttribute("readonly", "");
        area.style.position = "fixed";
        area.style.top = "-1000px";
        area.style.opacity = "0";
        document.body.appendChild(area);
        area.select();
        var copied = false;
        try {
            copied = document.execCommand("copy");
        } catch (error) {
            copied = false;
        }
        area.remove();
        if (!copied) throw new Error("copy command was refused");
    }

    function copyText(text) {
        if (navigator.clipboard && window.isSecureContext) {
            return navigator.clipboard.writeText(text);
        }
        return Promise.resolve().then(function () {
            legacyCopy(text);
        });
    }

    document.querySelectorAll("[data-copy-target]").forEach(function (button) {
        var target = document.querySelector(button.getAttribute("data-copy-target"));
        if (!target) return;

        var status = button.parentElement.querySelector(".copy-status");
        var original = button.textContent;

        button.addEventListener("click", function () {
            var text = target.textContent.replace(/^\$\s?/gm, "").trim();

            copyText(text).then(function () {
                button.textContent = "Copied";
                if (status) status.textContent = "COMMANDS COPIED";
                window.setTimeout(function () {
                    button.textContent = original;
                    if (status) status.textContent = "";
                }, 1800);
            }).catch(function () {
                if (status) status.textContent = "COPY BLOCKED — SELECT THE TEXT";
            });
        });
    });

    var links = Array.prototype.slice.call(document.querySelectorAll('.rail-nav a[href^="#"]'));
    var sections = links.map(function (link) {
        return document.querySelector(link.getAttribute("href"));
    }).filter(Boolean);

    if (!sections.length) return;

    var ticking = false;

    function markCurrent() {
        ticking = false;
        var current = sections[0];
        var line = 110;

        sections.forEach(function (section) {
            if (section.getBoundingClientRect().top <= line) current = section;
        });

        links.forEach(function (link) {
            var section = document.querySelector(link.getAttribute("href"));
            if (section === current) {
                link.setAttribute("aria-current", "location");
            } else {
                link.removeAttribute("aria-current");
            }
        });
    }

    function requestMark() {
        if (ticking) return;
        ticking = true;
        window.requestAnimationFrame(markCurrent);
    }

    window.addEventListener("scroll", requestMark, { passive: true });
    window.addEventListener("resize", requestMark);
    markCurrent();
})();
