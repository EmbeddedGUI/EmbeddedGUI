"""User code preservation for EmbeddedGUI Designer code generation.

Implements STM32 CubeMX-style user code protection: regions between
``// USER CODE BEGIN <tag>`` and ``// USER CODE END <tag>`` markers are
extracted from existing files before regeneration and re-injected into
the freshly generated output, so that hand-written code is never lost.

Additionally provides:
  - File backup before overwrite
  - SHA-256 hash embedding for incremental generation (skip unchanged layouts)

Usage (inside code_generator / compiler):

    from ..generator.user_code_preserver import (
        extract_user_code, inject_user_code,
        backup_file, embed_source_hash, should_skip_generation,
    )

    # Before generating a file, read the old one
    old_content = read(filepath)
    user_blocks = extract_user_code(old_content)

    # Generate new content (with empty USER CODE placeholders)
    new_content = generate_page_header(page, project)

    # Re-inject user code
    final_content = inject_user_code(new_content, user_blocks)

    # Backup + write
    backup_file(filepath, backup_dir)
    write(filepath, final_content)
"""

import os
import re
import shutil
import hashlib
from datetime import datetime


# ── Regex for USER CODE markers ──────────────────────────────────

# Matches:  // USER CODE BEGIN <tag>
#           <anything — the user's code>
#           // USER CODE END <tag>
_USER_CODE_RE = re.compile(
    r"^([ \t]*)// USER CODE BEGIN (\S+)\s*\n"   # opening marker (capture indent + tag)
    r"(.*?)"                                      # user code body (non-greedy)
    r"^[ \t]*// USER CODE END \2\s*$",            # closing marker (back-ref tag)
    re.MULTILINE | re.DOTALL,
)

# Matches the source hash comment we embed in generated files:
#   // GENERATED_HASH: <hex>
_HASH_RE = re.compile(r"^// GENERATED_HASH: ([0-9a-f]+)\s*$", re.MULTILINE)


# ── Extract user code blocks from existing file ──────────────────

def extract_user_code(content):
    """Parse all ``USER CODE BEGIN/END`` regions from *content*.

    Returns:
        dict[str, str]: tag → body text (including original indentation
        and trailing newline).  If a region is empty (no code between
        markers), the value is an empty string ``""``.

    Example::

        >>> extract_user_code(
        ...     "// USER CODE BEGIN callbacks\\n"
        ...     "static void my_cb(void) {}\\n"
        ...     "// USER CODE END callbacks\\n"
        ... )
        {'callbacks': 'static void my_cb(void) {}\\n'}
    """
    blocks = {}
    for m in _USER_CODE_RE.finditer(content):
        tag = m.group(2)
        body = m.group(3)
        blocks[tag] = body
    return blocks


def inject_user_code(generated_content, user_blocks):
    """Replace empty ``USER CODE`` placeholders in *generated_content*
    with the corresponding bodies from *user_blocks*.

    Any tag present in *generated_content* but absent from *user_blocks*
    is left as-is (empty placeholder).  Tags in *user_blocks* that don't
    appear in *generated_content* are **appended** inside a special
    ``// USER CODE BEGIN _orphaned`` section at the end of the file so
    that nothing is silently lost.

    Returns:
        str: The final file content with user code restored.
    """
    if not user_blocks:
        return generated_content

    used_tags = set()

    def _replacer(m):
        indent = m.group(1)
        tag = m.group(2)
        used_tags.add(tag)
        body = user_blocks.get(tag, "")
        return (
            f"{indent}// USER CODE BEGIN {tag}\n"
            f"{body}"
            f"{indent}// USER CODE END {tag}"
        )

    result = _USER_CODE_RE.sub(_replacer, generated_content)

    # Append orphaned user code blocks (tags that were in the old file
    # but no longer have a placeholder in the new generated code).
    orphaned = {k: v for k, v in user_blocks.items()
                if k not in used_tags and v.strip()}
    if orphaned:
        lines = ["\n// -- Orphaned user code (tags removed in latest generation) --"]
        for tag, body in orphaned.items():
            lines.append(f"// USER CODE BEGIN {tag}")
            # Ensure body ends with newline
            if body and not body.endswith("\n"):
                body += "\n"
            lines.append(body.rstrip("\n"))
            lines.append(f"// USER CODE END {tag}")
        lines.append("")
        result = result.rstrip("\n") + "\n" + "\n".join(lines) + "\n"

    return result


# ── File-level helpers ───────────────────────────────────────────

def read_existing_file(filepath):
    """Read a file if it exists, return its content or ``None``."""
    if not os.path.isfile(filepath):
        return None
    try:
        with open(filepath, "r", encoding="utf-8") as f:
            return f.read()
    except (IOError, UnicodeDecodeError):
        return None


def preserve_user_code(filepath, new_content):
    """High-level helper: read old file → extract user blocks → inject into
    *new_content*.

    If the old file does not exist, returns *new_content* unchanged.
    """
    old = read_existing_file(filepath)
    if old is None:
        return new_content
    blocks = extract_user_code(old)
    if not blocks:
        return new_content
    return inject_user_code(new_content, blocks)


# ── Backup ───────────────────────────────────────────────────────

def backup_file(filepath, backup_root):
    """Copy *filepath* into *backup_root*/<timestamp>/<relative_name>.

    Does nothing if the source file does not exist.

    Args:
        filepath:    Absolute path to the file about to be overwritten.
        backup_root: Directory for backups (e.g. ``.eguiproject/backup``).

    Returns:
        str | None: Path to the backup copy, or None if nothing was backed up.
    """
    if not os.path.isfile(filepath):
        return None

    timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
    basename = os.path.basename(filepath)
    dest_dir = os.path.join(backup_root, timestamp)
    os.makedirs(dest_dir, exist_ok=True)
    dest = os.path.join(dest_dir, basename)

    # Avoid duplicate backups in the same second
    if os.path.exists(dest):
        return dest

    shutil.copy2(filepath, dest)
    return dest


def cleanup_old_backups(backup_root, keep=20):
    """Keep only the *keep* most recent backup directories.

    Args:
        backup_root: Directory containing timestamped subdirs.
        keep:        Number of subdirectories to retain.
    """
    if not os.path.isdir(backup_root):
        return
    dirs = sorted(
        [d for d in os.listdir(backup_root)
         if os.path.isdir(os.path.join(backup_root, d))],
        reverse=True,
    )
    for old_dir in dirs[keep:]:
        shutil.rmtree(os.path.join(backup_root, old_dir), ignore_errors=True)


# ── Source hash for incremental generation ───────────────────────

def compute_source_hash(source_text):
    """Compute a short SHA-256 hex digest of *source_text* (first 16 chars)."""
    return hashlib.sha256(source_text.encode("utf-8")).hexdigest()[:16]


def embed_source_hash(generated_content, source_hash):
    """Prepend a ``// GENERATED_HASH: <hash>`` comment to *generated_content*.

    If the content already has a hash line, it is replaced.
    """
    # Remove existing hash line if present
    content = _HASH_RE.sub("", generated_content).lstrip("\n")
    return f"// GENERATED_HASH: {source_hash}\n{content}"


def get_embedded_hash(filepath):
    """Read the ``GENERATED_HASH`` from an existing file, or return None."""
    content = read_existing_file(filepath)
    if content is None:
        return None
    m = _HASH_RE.search(content)
    return m.group(1) if m else None


def should_skip_generation(filepath, current_source_hash):
    """Return True if *filepath* already contains *current_source_hash*,
    meaning the layout XML has not changed and regeneration can be skipped.

    Note: even when skipping the full file, user code preservation is still
    not needed because nothing changed.
    """
    existing_hash = get_embedded_hash(filepath)
    return existing_hash == current_source_hash
