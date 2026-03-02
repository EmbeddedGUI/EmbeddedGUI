"""Resource config generator for EmbeddedGUI Designer.

Scans all layout XMLs (via the Project model), collects widget-level resource
configurations, deduplicates, and generates ``app_resource_config.json``
compatible with the existing ``app_resource_generate.py`` pipeline.

Data flow:
    layout/*.xml (widget-level params)
        -> ResourceConfigGenerator.generate(project)
        -> app_resource_config.json
        -> app_resource_generate.py (unchanged)
        -> C source files
"""

import json
import os

from ..model.widget_model import format_file_name, _file_stem
from ..model.string_resource import parse_string_ref


class ResourceConfigGenerator:
    """Scans project widgets and generates app_resource_config.json."""

    def generate(self, project):
        """Scan all pages and generate a resource config dict.

        Returns:
            dict: ``{"img": [...], "font": [...]}`` suitable for JSON output.
        """
        img_configs = []
        font_configs = []
        string_catalog = getattr(project, 'string_catalog', None)

        for page in project.pages:
            for widget in page.get_all_widgets():
                if widget.widget_type == "image":
                    cfg = self._collect_image_config(widget)
                    if cfg:
                        img_configs.append(cfg)
                elif widget.widget_type in ("label", "button"):
                    cfg = self._collect_font_config(widget, string_catalog)
                    if cfg:
                        font_configs.append(cfg)

        # Deduplicate images and merge fonts
        img_configs = self._deduplicate_images(img_configs)
        font_configs = self._merge_font_configs(font_configs)

        return {"img": img_configs, "font": font_configs}

    def generate_and_save(self, project, src_dir):
        """Generate config and write to app_resource_config.json in src_dir.

        Also writes any generated text files for font text content.

        Args:
            project: Project model instance.
            src_dir: Path to resource/src/ directory.
        """
        config = self.generate(project)

        # Write generated text files for inline font text
        self._write_font_text_files(config, src_dir)

        os.makedirs(src_dir, exist_ok=True)
        config_path = os.path.join(src_dir, "app_resource_config.json")
        with open(config_path, "w", encoding="utf-8") as f:
            json.dump(config, f, indent=4, ensure_ascii=False)

    # ── Internal methods ──────────────────────────────────────────

    def _collect_image_config(self, widget):
        """Collect image resource config from a widget's properties."""
        image_file = widget.properties.get("image_file", "")
        if not image_file:
            return None

        cfg = {"file": image_file}

        image_format = widget.properties.get("image_format", "rgb565")
        if image_format:
            cfg["format"] = image_format

        image_alpha = widget.properties.get("image_alpha", "4")
        if image_alpha:
            cfg["alpha"] = image_alpha

        image_dim = widget.properties.get("image_dim", "")
        if image_dim:
            cfg["dim"] = image_dim

        image_external = widget.properties.get("image_external", "0")
        cfg["external"] = image_external

        image_swap = widget.properties.get("image_swap", False)
        cfg["swap"] = "1" if image_swap else "0"

        image_rot = widget.properties.get("image_rot", 0)
        if image_rot:
            cfg["rot"] = str(image_rot)

        return cfg

    def _collect_font_config(self, widget, string_catalog=None):
        """Collect font resource config from a widget's properties."""
        font_file = widget.properties.get("font_file", "")
        if not font_file:
            return None

        cfg = {"file": font_file}

        pixelsize = widget.properties.get("font_pixelsize", "16")
        cfg["pixelsize"] = pixelsize

        fontbitsize = widget.properties.get("font_fontbitsize", "4")
        cfg["fontbitsize"] = fontbitsize

        external = widget.properties.get("font_external", "0")
        cfg["external"] = external

        # Collect text content from widget
        texts = set()

        # The widget's display text — resolve @string/ references
        text = widget.properties.get("text", "")
        if text:
            str_key = parse_string_ref(text)
            if str_key is not None and string_catalog is not None:
                # Collect all characters from all locales for this key
                for locale in string_catalog.locales:
                    value = string_catalog.get(str_key, locale)
                    if value:
                        texts.update(value)
            else:
                texts.update(text)

        # Additional runtime text
        font_text = widget.properties.get("font_text", "")
        if font_text:
            texts.update(font_text)

        cfg["_inline_text"] = "".join(sorted(texts)) if texts else ""

        # Text file reference
        text_file = widget.properties.get("font_text_file", "")
        cfg["_text_files"] = [text_file] if text_file else []

        return cfg

    def _deduplicate_images(self, configs):
        """Deduplicate image configs; resolve name collisions for different dims."""
        # Group by (file, format, alpha, dim, external, swap, rot)
        seen = {}
        for cfg in configs:
            key = (
                cfg.get("file", ""),
                cfg.get("format", "rgb565"),
                cfg.get("alpha", "4"),
                cfg.get("dim", ""),
                cfg.get("external", "0"),
                cfg.get("swap", "0"),
                cfg.get("rot", ""),
            )
            seen[key] = cfg

        # Check for name collisions: same file+format+alpha but different dim
        # Group by (file, format, alpha) to detect dim conflicts
        name_groups = {}
        for key, cfg in seen.items():
            name_key = (cfg["file"], cfg.get("format", "rgb565"), cfg.get("alpha", "4"))
            if name_key not in name_groups:
                name_groups[name_key] = []
            name_groups[name_key].append(cfg)

        result = []
        for name_key, group in name_groups.items():
            if len(group) == 1:
                result.append(group[0])
            else:
                # Multiple dims for same file+format+alpha -> add size suffix to name
                for cfg in group:
                    dim = cfg.get("dim", "")
                    if dim:
                        stem = _file_stem(cfg["file"])
                        dim_suffix = dim.replace(",", "x")
                        cfg["name"] = format_file_name(f"{stem}_{dim_suffix}")
                    result.append(cfg)

        return result

    def _merge_font_configs(self, configs):
        """Merge font configs with same (file, pixelsize, fontbitsize, external).

        Inline text characters are unioned. Text file references are collected.
        """
        merged = {}
        for cfg in configs:
            key = (
                cfg["file"],
                cfg.get("pixelsize", "16"),
                cfg.get("fontbitsize", "4"),
                cfg.get("external", "0"),
            )
            if key not in merged:
                merged[key] = {
                    "file": cfg["file"],
                    "pixelsize": cfg.get("pixelsize", "16"),
                    "fontbitsize": cfg.get("fontbitsize", "4"),
                    "external": cfg.get("external", "0"),
                    "_inline_chars": set(),
                    "_text_files": [],
                }
            entry = merged[key]

            # Merge inline text characters
            inline_text = cfg.get("_inline_text", "")
            if inline_text:
                entry["_inline_chars"].update(inline_text)

            # Merge text file references (unique)
            for tf in cfg.get("_text_files", []):
                if tf and tf not in entry["_text_files"]:
                    entry["_text_files"].append(tf)

        # Convert merged entries to final config format
        result = []
        for key, entry in merged.items():
            cfg = {
                "file": entry["file"],
                "pixelsize": entry["pixelsize"],
                "fontbitsize": entry["fontbitsize"],
                "external": entry["external"],
            }

            # Determine text reference
            inline_chars = entry["_inline_chars"]
            text_files = entry["_text_files"]

            if inline_chars and not text_files:
                # Only inline text: generate a text file
                stem = format_file_name(_file_stem(entry["file"]))
                ps = entry["pixelsize"]
                bs = entry["fontbitsize"]
                gen_filename = f"_generated_text_{stem}_{ps}_{bs}.txt"
                cfg["text"] = gen_filename
                cfg["_generated_text_content"] = "".join(sorted(inline_chars))
            elif text_files and not inline_chars:
                # Only text file references
                cfg["text"] = text_files[0]
                # Additional text files added as _extra_text_files
                if len(text_files) > 1:
                    cfg["_extra_text_files"] = text_files[1:]
            elif inline_chars and text_files:
                # Both: generate a file for inline chars + reference existing files
                stem = format_file_name(_file_stem(entry["file"]))
                ps = entry["pixelsize"]
                bs = entry["fontbitsize"]
                gen_filename = f"_generated_text_{stem}_{ps}_{bs}.txt"
                cfg["text"] = gen_filename
                cfg["_generated_text_content"] = "".join(sorted(inline_chars))
                cfg["_extra_text_files"] = list(text_files)
            else:
                # No text at all - skip this font (nothing to render)
                continue

            result.append(cfg)

        return result

    def _write_font_text_files(self, config, src_dir):
        """Write generated text files for inline font text content.

        Characters outside the Basic Latin range (U+0080+) are written as
        ``&#xHHHH;`` XML-style entities so the file stays readable in any
        text editor.  The resource pipeline (ttf2c) resolves them back to
        Unicode codepoints.
        """
        os.makedirs(src_dir, exist_ok=True)
        for font_cfg in config.get("font", []):
            content = font_cfg.pop("_generated_text_content", None)
            extra_files = font_cfg.pop("_extra_text_files", None)
            if content is not None:
                text_filename = font_cfg.get("text", "")
                if text_filename:
                    text_path = os.path.join(src_dir, text_filename)
                    with open(text_path, "w", encoding="utf-8") as f:
                        for ch in content:
                            if ord(ch) >= 0x80:
                                f.write(f"&#x{ord(ch):04X};")
                            else:
                                f.write(ch)
            # If there are extra text files, store as comma-separated in "text" field
            # The existing app_resource_generate.py supports text_file_list
            if extra_files:
                current_text = font_cfg.get("text", "")
                all_texts = [current_text] + extra_files if current_text else extra_files
                font_cfg["text"] = ",".join(all_texts)
