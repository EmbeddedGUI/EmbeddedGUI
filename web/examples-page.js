(function() {
    var hasCurrentShell = document.querySelector(".sidebar-shell") && document.querySelector("#demo-list");
    if (hasCurrentShell) {
        return;
    }

    var url = new URL(window.location.href);
    if (url.searchParams.has("__fresh")) {
        return;
    }

    url.searchParams.set("__fresh", String(Date.now()));
    window.location.replace(url.toString());
})();
