"""Project save/load for EmbeddedGUI Designer — XML format (.egui).

Project structure on disk (inside app directory, e.g. example/HelloDesigner/):

    HelloDesigner.egui      - project metadata (XML, single entry file)
    .eguiproject/
        layout/
            main_page.xml   - one file per page
        resources/
            resources.xml   - resource catalog
            images/         - source image files (.png, .bmp, .jpg)
            values/
                strings.xml - default locale i18n strings
            values-zh/
                strings.xml - Chinese locale i18n strings
            *.ttf, *.otf    - font source files
            *.txt           - text source files
        mockup/             - mockup images (design-time only)
    resource/
        src/                - synced from .eguiproject/resources/ for generation
        img/                - generated
        font/               - generated
"""

import os
import shutil
import xml.etree.ElementTree as ET

from .widget_model import WidgetModel
from .page import Page
from .resource_catalog import ResourceCatalog
from .string_resource import StringResourceCatalog
from .workspace import normalize_path, resolve_project_sdk_root, serialize_sdk_root


class Project:
    """Represents a designer project with multiple pages.

    Project files are stored in the app directory (e.g., example/HelloDesigner/).
    """

    def __init__(self, screen_width=240, screen_height=320, app_name="HelloDesigner"):
        self.screen_width = screen_width
        self.screen_height = screen_height
        self.app_name = app_name
        self.sdk_root = ""  # Path to EmbeddedGUI SDK root directory
        self.project_dir = ""  # Path to the app/project directory
        self.page_mode = "easy_page"  # "easy_page" or "activity"
        self.startup_page = "main_page"  # filename without extension
        self.resource_config = ""  # relative path to resource config (legacy)
        self.resource_catalog = ResourceCatalog()  # project resource catalog
        self.string_catalog = StringResourceCatalog()  # i18n string resources
        self.pages = []  # list[Page]

    @property
    def egui_root(self):
        """Legacy alias for ``sdk_root``."""
        return self.sdk_root

    @egui_root.setter
    def egui_root(self, value):
        self.sdk_root = normalize_path(value)

    @property
    def root_widgets(self):
        """Compatibility shim: return root widgets of the startup page.

        Used by layout_engine.compute_layout() and widget_tree/preview.
        """
        page = self.get_startup_page()
        if page and page.root_widget:
            return [page.root_widget]
        return []

    # ── Page management ────────────────────────────────────────────

    def get_page_by_name(self, name):
        """Find a page by its derived name (filename without extension)."""
        for page in self.pages:
            if page.name == name:
                return page
        return None

    def add_page(self, page):
        """Add a page to the project."""
        self.pages.append(page)

    def remove_page(self, page):
        """Remove a page from the project."""
        self.pages.remove(page)

    def get_startup_page(self):
        """Get the startup page object."""
        page = self.get_page_by_name(self.startup_page)
        if page is None and self.pages:
            return self.pages[0]
        return page

    def create_new_page(self, page_name):
        """Create and add a new page with default root group."""
        page = Page.create_default(
            page_name,
            screen_width=self.screen_width,
            screen_height=self.screen_height,
        )
        self.add_page(page)
        return page

    def duplicate_page(self, source_name, new_name):
        """Duplicate an existing page under a new page name."""
        source_page = self.get_page_by_name(source_name)
        if source_page is None:
            raise ValueError(f"Page '{source_name}' does not exist.")
        if self.get_page_by_name(new_name) is not None:
            raise ValueError(f"Page '{new_name}' already exists.")

        page = Page.from_xml_string(
            source_page.to_xml_string(),
            file_path=f"layout/{new_name}.xml",
        )
        page.dirty = True
        self.add_page(page)
        return page

    # ── Path helpers ──────────────────────────────────────────────

    def get_app_dir(self):
        """Get the app directory path."""
        if self.project_dir:
            return self.project_dir
        if self.sdk_root:
            return os.path.join(self.sdk_root, "example", self.app_name)
        return ""

    def get_resource_dir(self):
        """Get the resource directory path (resource/).

        Used for the generation pipeline (generated output in resource/img/,
        resource/font/) and for property_panel to scan generated fonts.
        """
        app_dir = self.get_app_dir()
        if not app_dir:
            return ""
        return os.path.join(app_dir, "resource")

    def get_eguiproject_dir(self):
        """Get the .eguiproject config directory path."""
        app_dir = self.get_app_dir()
        if not app_dir:
            return ""
        return os.path.join(app_dir, ".eguiproject")

    def get_eguiproject_resource_dir(self):
        """Get the .eguiproject/resources/ directory path.

        This is the authoritative location for all resource files.
        Contains: resources.xml, images/, values*/, fonts, text files.
        """
        eguiproject_dir = self.get_eguiproject_dir()
        if not eguiproject_dir:
            return ""
        return os.path.join(eguiproject_dir, "resources")

    def get_eguiproject_images_dir(self):
        """Get the .eguiproject/resources/images/ directory path.

        Authoritative location for source image files.
        """
        res_dir = self.get_eguiproject_resource_dir()
        if not res_dir:
            return ""
        return os.path.join(res_dir, "images")

    # ── Widgets ────────────────────────────────────────────────────

    def get_all_widgets(self):
        """Return flat list of all widgets across all pages."""
        result = []
        for page in self.pages:
            result.extend(page.get_all_widgets())
        return result

    # ── Save / Load (.egui XML) ───────────────────────────────────

    def save(self, project_dir):
        """Save project to directory.

        Creates:
            project_dir/{app_name}.egui        - project metadata
            project_dir/.eguiproject/resources.xml  - resource catalog
            project_dir/.eguiproject/layout/*.xml   - one file per page
        """
        project_dir = normalize_path(project_dir)
        self.project_dir = project_dir
        os.makedirs(project_dir, exist_ok=True)
        eguiproject_dir = os.path.join(project_dir, ".eguiproject")
        os.makedirs(eguiproject_dir, exist_ok=True)

        # Save each page XML to .eguiproject/layout/
        for page in self.pages:
            page.save(eguiproject_dir)

        # Save resource catalog to .eguiproject/resources/resources.xml
        resources_dir = os.path.join(eguiproject_dir, "resources")
        os.makedirs(resources_dir, exist_ok=True)
        self.resource_catalog.save(resources_dir)

        # Save i18n string resources to .eguiproject/resources/values*/strings.xml
        if self.string_catalog.has_strings:
            self.string_catalog.save(resources_dir)

        # Sync .eguiproject/resources/ -> resource/src/ for generation pipeline
        self.sync_resources_to_src(project_dir)

        # Build {app_name}.egui
        root = ET.Element("Project")
        root.set("app_name", self.app_name)
        root.set("screen_width", str(self.screen_width))
        root.set("screen_height", str(self.screen_height))
        root.set("page_mode", self.page_mode)
        root.set("startup", self.startup_page)
        if self.sdk_root:
            sdk_value = serialize_sdk_root(project_dir, self.sdk_root)
            root.set("sdk_root", sdk_value)
            root.set("egui_root", sdk_value)

        pages_elem = ET.SubElement(root, "Pages")
        for page in self.pages:
            ref = ET.SubElement(pages_elem, "PageRef")
            ref.set("file", page.file_path)

        res = ET.SubElement(root, "Resources")
        res.set("catalog", "resources.xml")

        ET.indent(root, space="    ")
        eui_path = os.path.join(project_dir, f"{self.app_name}.egui")
        tree = ET.ElementTree(root)
        with open(eui_path, "w", encoding="utf-8") as f:
            f.write('<?xml version="1.0" encoding="utf-8"?>\n')
            tree.write(f, encoding="unicode", xml_declaration=False)

    @classmethod
    def _find_project_file(cls, directory):
        """Find the project file (.egui) in a directory.

        Returns (file_path, True) or (None, False) if not found.
        """
        if not os.path.isdir(directory):
            return None, False
        for fname in os.listdir(directory):
            if fname.endswith(".egui"):
                return os.path.join(directory, fname), True
        return None, False

    @classmethod
    def load(cls, project_path):
        """Load project from .egui file or from a directory containing one."""
        if os.path.isdir(project_path):
            project_file, found = cls._find_project_file(project_path)
            if not found:
                raise FileNotFoundError(
                    f"No .egui project file found in {project_path}"
                )
        else:
            project_file = project_path

        project_dir = os.path.dirname(os.path.abspath(project_file))
        config_dir = os.path.join(project_dir, ".eguiproject")

        tree = ET.parse(project_file)
        root = tree.getroot()

        proj = cls(
            screen_width=int(root.get("screen_width", "240")),
            screen_height=int(root.get("screen_height", "320")),
            app_name=root.get("app_name", "HelloDesigner"),
        )
        proj.project_dir = project_dir
        proj.sdk_root = resolve_project_sdk_root(project_dir, root.get("sdk_root", root.get("egui_root", "")))
        proj.page_mode = root.get("page_mode", "easy_page")
        proj.startup_page = root.get("startup", "main_page")

        # Resource config
        res_elem = root.find("Resources")
        if res_elem is not None:
            proj.resource_config = res_elem.get("config", "")

        # Determine directories
        src_dir = os.path.join(project_dir, "resource", "src")  # legacy
        eguiproject_res_dir = os.path.join(config_dir, "resources")  # new
        eguiproject_images_dir = os.path.join(eguiproject_res_dir, "images")

        # Load resource catalog
        # Try .eguiproject/resources/resources.xml first, then .eguiproject/resources.xml (old)
        catalog = ResourceCatalog.load(eguiproject_res_dir)
        if catalog is None:
            catalog = ResourceCatalog.load(config_dir)  # legacy location
        if catalog is not None:
            proj.resource_catalog = catalog
        else:
            # Auto-create catalog from existing resource directory
            if os.path.isdir(eguiproject_res_dir):
                proj.resource_catalog = ResourceCatalog.from_directory(eguiproject_res_dir)
            elif os.path.isdir(src_dir):
                proj.resource_catalog = ResourceCatalog.from_resource_config(src_dir)
            else:
                proj.resource_catalog = ResourceCatalog()

        # Load i18n string resources
        # Try .eguiproject/resources/values*/ first, then .eguiproject/values*/, then resource/src/values*/
        proj.string_catalog = StringResourceCatalog.scan_and_load(eguiproject_res_dir)
        if not proj.string_catalog.has_strings:
            proj.string_catalog = StringResourceCatalog.scan_and_load(config_dir)
        if not proj.string_catalog.has_strings:
            proj.string_catalog = StringResourceCatalog.scan_and_load(src_dir)

        # Migration: copy source files from old locations to .eguiproject/resources/
        proj._migrate_resources_if_needed(project_dir)

        # Determine the authoritative source dir for page loading
        # (used for image path validation — point to images/ subfolder)
        effective_src_dir = eguiproject_images_dir if os.path.isdir(eguiproject_images_dir) else (
            eguiproject_res_dir if os.path.isdir(eguiproject_res_dir) else src_dir
        )

        # Load pages
        pages_elem = root.find("Pages")
        if pages_elem is not None:
            WidgetModel.reset_counter()
            for ref in pages_elem.findall("PageRef"):
                file_path = ref.get("file", "")
                if not file_path:
                    continue
                try:
                    page = Page.load(config_dir, file_path, src_dir=effective_src_dir)
                    proj.pages.append(page)
                except Exception as e:
                    print(f"Warning: Failed to load page {file_path}: {e}")

        return proj

    def _migrate_resources_if_needed(self, project_dir):
        """One-time migration: copy resource files from old locations to the
        current canonical layout under .eguiproject/resources/.

        Handles three generations of layout:
          1. resource/src/  (original legacy)
          2. .eguiproject/resources/ flat + .eguiproject/values*/  (intermediate)
          3. .eguiproject/resources/images/ + .eguiproject/resources/values*/ (current)
        """
        import re
        _VALUES_DIR_RE = re.compile(r'^values(?:-[a-zA-Z]{2,8})?$')
        _IMAGE_EXTS = ('.png', '.bmp', '.jpg', '.jpeg')
        _FONT_EXTS = ('.ttf', '.otf')

        config_dir = os.path.join(project_dir, ".eguiproject")
        eguiproject_res_dir = os.path.join(config_dir, "resources")
        images_dir = os.path.join(eguiproject_res_dir, "images")
        old_src_dir = os.path.join(project_dir, "resource", "src")

        os.makedirs(eguiproject_res_dir, exist_ok=True)

        # --- Step A: migrate from resource/src/ (generation 1 -> 3) ---
        if os.path.isdir(old_src_dir):
            os.makedirs(images_dir, exist_ok=True)
            for fname in os.listdir(old_src_dir):
                old_path = os.path.join(old_src_dir, fname)
                if os.path.isfile(old_path):
                    ext = os.path.splitext(fname)[1].lower()
                    if ext in _IMAGE_EXTS:
                        new_path = os.path.join(images_dir, fname)
                    else:
                        new_path = os.path.join(eguiproject_res_dir, fname)
                    if not os.path.exists(new_path):
                        shutil.copy2(old_path, new_path)
                elif os.path.isdir(old_path) and _VALUES_DIR_RE.match(fname):
                    old_xml = os.path.join(old_path, "strings.xml")
                    if os.path.isfile(old_xml):
                        new_dir = os.path.join(eguiproject_res_dir, fname)
                        os.makedirs(new_dir, exist_ok=True)
                        new_xml = os.path.join(new_dir, "strings.xml")
                        if not os.path.exists(new_xml):
                            shutil.copy2(old_xml, new_xml)

        # --- Step B: migrate from flat .eguiproject/resources/ (gen 2 -> 3) ---
        # Move image files from resources/ root into resources/images/
        os.makedirs(images_dir, exist_ok=True)
        for fname in os.listdir(eguiproject_res_dir):
            fpath = os.path.join(eguiproject_res_dir, fname)
            if os.path.isfile(fpath):
                ext = os.path.splitext(fname)[1].lower()
                if ext in _IMAGE_EXTS:
                    new_path = os.path.join(images_dir, fname)
                    if not os.path.exists(new_path):
                        shutil.move(fpath, new_path)
                    elif fpath != new_path:
                        os.remove(fpath)  # duplicate, remove from root

        # --- Step C: migrate values from .eguiproject/ (gen 2 -> 3) ---
        if os.path.isdir(config_dir):
            for dname in os.listdir(config_dir):
                if not _VALUES_DIR_RE.match(dname):
                    continue
                old_dir = os.path.join(config_dir, dname)
                if not os.path.isdir(old_dir):
                    continue
                # Skip if it's already inside resources/
                if os.path.commonpath([old_dir, eguiproject_res_dir]) == os.path.normpath(eguiproject_res_dir):
                    continue
                old_xml = os.path.join(old_dir, "strings.xml")
                if os.path.isfile(old_xml):
                    new_dir = os.path.join(eguiproject_res_dir, dname)
                    os.makedirs(new_dir, exist_ok=True)
                    new_xml = os.path.join(new_dir, "strings.xml")
                    if not os.path.exists(new_xml):
                        shutil.copy2(old_xml, new_xml)

        # --- Step D: migrate resources.xml from .eguiproject/ (gen 2 -> 3) ---
        old_catalog = os.path.join(config_dir, "resources.xml")
        new_catalog = os.path.join(eguiproject_res_dir, "resources.xml")
        if os.path.isfile(old_catalog) and not os.path.isfile(new_catalog):
            shutil.copy2(old_catalog, new_catalog)

        # Reload catalogs from migrated location if needed
        reloaded_strings = StringResourceCatalog.scan_and_load(eguiproject_res_dir)
        if reloaded_strings.has_strings:
            self.string_catalog = reloaded_strings

    def sync_resources_to_src(self, project_dir):
        """Sync .eguiproject/resources/ → resource/src/ for the generation pipeline.

        Copies source files to resource/src/ so app_resource_generate.py can
        find them.  Images live in resources/images/, fonts and text files
        live in the resources/ root.
        Only copies if source is newer or destination doesn't exist.
        """
        config_dir = os.path.join(project_dir, ".eguiproject")
        eguiproject_res_dir = os.path.join(config_dir, "resources")
        images_dir = os.path.join(eguiproject_res_dir, "images")
        target_src_dir = os.path.join(project_dir, "resource", "src")

        if not os.path.isdir(eguiproject_res_dir):
            return

        os.makedirs(target_src_dir, exist_ok=True)

        def _sync_file(src_path, dst_path):
            if not os.path.exists(dst_path):
                shutil.copy2(src_path, dst_path)
            elif os.path.getmtime(src_path) > os.path.getmtime(dst_path):
                shutil.copy2(src_path, dst_path)

        # Sync images from resources/images/
        if os.path.isdir(images_dir):
            for fname in os.listdir(images_dir):
                src_path = os.path.join(images_dir, fname)
                if os.path.isfile(src_path):
                    _sync_file(src_path, os.path.join(target_src_dir, fname))

        # Sync fonts and text files from resources/ root
        for fname in os.listdir(eguiproject_res_dir):
            src_path = os.path.join(eguiproject_res_dir, fname)
            if not os.path.isfile(src_path):
                continue
            # Skip resources.xml (not a source file)
            if fname == "resources.xml":
                continue
            _sync_file(src_path, os.path.join(target_src_dir, fname))

    @classmethod
    def get_config_dir(cls, project_dir):
        """Get the .eguiproject config directory for a project."""
        return os.path.join(project_dir, ".eguiproject")

