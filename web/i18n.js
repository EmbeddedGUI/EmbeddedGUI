(function() {
    var STORAGE_KEY = "embeddedgui.web.lang";
    var DEFAULT_LANG = "zh-CN";

    function normalizeLanguage(value) {
        if (!value) {
            return DEFAULT_LANG;
        }
        var lower = String(value).trim().toLowerCase();
        if (!lower) {
            return DEFAULT_LANG;
        }
        if (lower === "zh" || lower === "zh-cn" || lower === "cn" || lower.indexOf("zh-") === 0) {
            return "zh-CN";
        }
        if (lower === "en" || lower.indexOf("en-") === 0) {
            return "en";
        }
        return DEFAULT_LANG;
    }

    function getQueryLanguage() {
        try {
            var params = new URLSearchParams(window.location.search);
            var queryLang = params.get("lang");
            return queryLang ? normalizeLanguage(queryLang) : "";
        } catch (error) {
            return "";
        }
    }

    function getStoredLanguage() {
        try {
            return localStorage.getItem(STORAGE_KEY) || "";
        } catch (error) {
            return "";
        }
    }

    function getLanguage() {
        return normalizeLanguage(getQueryLanguage() || getStoredLanguage() || DEFAULT_LANG);
    }

    function interpolate(text, params) {
        if (!params) {
            return text;
        }
        return String(text).replace(/\{(\w+)\}/g, function(match, key) {
            return Object.prototype.hasOwnProperty.call(params, key) ? params[key] : match;
        });
    }

    function translateEntry(entry, params, lang) {
        var resolvedLang = normalizeLanguage(lang || getLanguage());
        var text = "";

        if (typeof entry === "string") {
            text = entry;
        } else if (entry && typeof entry === "object") {
            text = entry[resolvedLang] || entry[DEFAULT_LANG] || entry.en || "";
        }

        return interpolate(text, params);
    }

    function updateToggleState(root) {
        var resolvedRoot = root || document;
        var lang = getLanguage();
        resolvedRoot.querySelectorAll("[data-lang-switch]").forEach(function(button) {
            var active = normalizeLanguage(button.getAttribute("data-lang-switch")) === lang;
            button.classList.toggle("active", active);
            button.setAttribute("aria-pressed", active ? "true" : "false");
        });
    }

    function updateCurrentUrl(lang) {
        try {
            var url = new URL(window.location.href);
            url.searchParams.set("lang", lang);
            history.replaceState(null, "", url.pathname + url.search + url.hash);
        } catch (error) {
            // Ignore URL update failures.
        }
    }

    function isLocalHtmlLink(href) {
        if (!href) {
            return false;
        }
        if (href.indexOf("http://") === 0 || href.indexOf("https://") === 0 || href.indexOf("mailto:") === 0 || href.indexOf("tel:") === 0) {
            return false;
        }
        var cleanHref = href.split("#")[0];
        if (!cleanHref) {
            return false;
        }
        return cleanHref.indexOf(".html") !== -1;
    }

    function syncInternalLinks(root) {
        var resolvedRoot = root || document;
        var lang = getLanguage();

        resolvedRoot.querySelectorAll("a[href]").forEach(function(link) {
            var rawHref = link.getAttribute("href");
            if (!isLocalHtmlLink(rawHref)) {
                return;
            }

            try {
                var url = new URL(rawHref, window.location.href);
                if (url.origin !== window.location.origin) {
                    return;
                }
                url.searchParams.set("lang", lang);
                var relative = url.pathname.replace(/.*\/web\//, "");
                link.setAttribute("href", relative + url.search + url.hash);
            } catch (error) {
                // Ignore malformed href values.
            }
        });
    }

    function syncDocumentLanguage() {
        var lang = getLanguage();
        document.documentElement.lang = lang;
        document.documentElement.setAttribute("data-lang", lang);
    }

    function setLanguage(lang, persist) {
        var resolvedLang = normalizeLanguage(lang);
        if (persist !== false) {
            try {
                localStorage.setItem(STORAGE_KEY, resolvedLang);
            } catch (error) {
                // Ignore storage failures and continue with in-memory language state.
            }
        }
        updateCurrentUrl(resolvedLang);
        syncDocumentLanguage();
        updateToggleState(document);
        syncInternalLinks(document);
        document.dispatchEvent(new CustomEvent("embeddedgui:languagechange", {
            detail: { lang: resolvedLang }
        }));
        return resolvedLang;
    }

    function bindLanguageToggle(root) {
        var resolvedRoot = root || document;
        resolvedRoot.querySelectorAll("[data-lang-switch]").forEach(function(button) {
            if (button.dataset.langBound === "1") {
                return;
            }
            button.dataset.langBound = "1";
            button.addEventListener("click", function() {
                setLanguage(button.getAttribute("data-lang-switch"));
            });
        });
        updateToggleState(resolvedRoot);
        syncInternalLinks(resolvedRoot);
    }

    function applyTranslations(translations, root) {
        var resolvedRoot = root || document;

        resolvedRoot.querySelectorAll("[data-i18n]").forEach(function(node) {
            var key = node.getAttribute("data-i18n");
            if (translations[key]) {
                node.textContent = translateEntry(translations[key]);
            }
        });

        resolvedRoot.querySelectorAll("[data-i18n-html]").forEach(function(node) {
            var key = node.getAttribute("data-i18n-html");
            if (translations[key]) {
                node.innerHTML = translateEntry(translations[key]);
            }
        });

        resolvedRoot.querySelectorAll("[data-i18n-placeholder]").forEach(function(node) {
            var key = node.getAttribute("data-i18n-placeholder");
            if (translations[key]) {
                node.setAttribute("placeholder", translateEntry(translations[key]));
            }
        });

        resolvedRoot.querySelectorAll("[data-i18n-aria-label]").forEach(function(node) {
            var key = node.getAttribute("data-i18n-aria-label");
            if (translations[key]) {
                node.setAttribute("aria-label", translateEntry(translations[key]));
            }
        });
    }

    window.EmbeddedGUII18n = {
        defaultLanguage: DEFAULT_LANG,
        normalizeLanguage: normalizeLanguage,
        getLanguage: getLanguage,
        setLanguage: setLanguage,
        bindLanguageToggle: bindLanguageToggle,
        updateToggleState: updateToggleState,
        syncInternalLinks: syncInternalLinks,
        applyTranslations: applyTranslations,
        format: function(entry, params) {
            return translateEntry(entry, params, getLanguage());
        },
        formatFor: function(entry, lang, params) {
            return translateEntry(entry, params, lang);
        }
    };

    if (typeof document !== "undefined") {
        syncDocumentLanguage();
    }
})();
