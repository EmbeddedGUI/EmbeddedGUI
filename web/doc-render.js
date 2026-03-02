// doc-render.js - Shared markdown doc rendering for demo pages
function renderDoc(container, docPath) {
    if (!docPath) {
        container.innerHTML = '';
        container.style.display = 'none';
        return;
    }
    container.style.display = 'block';
    container.innerHTML = '<p class="doc-loading">Loading...</p>';
    fetch(docPath)
        .then(function(r) {
            if (!r.ok) throw new Error('Not found');
            return r.text();
        })
        .then(function(md) {
            container.innerHTML = marked.parse(md);
        })
        .catch(function() {
            container.innerHTML = '';
            container.style.display = 'none';
        });
}

// Summary mode: extract first paragraph only (for grid cards)
function renderDocSummary(container, docPath) {
    if (!docPath) {
        container.innerHTML = '';
        container.style.display = 'none';
        return;
    }
    container.style.display = 'block';
    fetch(docPath)
        .then(function(r) {
            if (!r.ok) throw new Error('Not found');
            return r.text();
        })
        .then(function(md) {
            // Extract first non-heading, non-empty paragraph
            var lines = md.split('\n');
            var para = '';
            for (var i = 0; i < lines.length; i++) {
                var line = lines[i].trim();
                if (!line || line.charAt(0) === '#' || line.charAt(0) === '-' || line.charAt(0) === '|') continue;
                para = line;
                break;
            }
            if (para) {
                container.innerHTML = '<p>' + para + '</p>';
            } else {
                container.style.display = 'none';
            }
        })
        .catch(function() {
            container.innerHTML = '';
            container.style.display = 'none';
        });
}
