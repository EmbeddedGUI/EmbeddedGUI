#!/usr/bin/env python3
"""Synchronize EmbeddedGUI Visual Studio app configurations."""

from __future__ import annotations

import argparse
import html
import os
import re
import sys
import uuid
from dataclasses import dataclass
from pathlib import Path


PROJECT_GUID_RE = re.compile(
    r'^Project\("\{[^}]+\}"\)\s*=\s*"EmbeddedGUI_PC_Simulator",\s*"EmbeddedGUI_PC_Simulator\.vcxproj",\s*"(\{[^}]+\})"'
)
VALID_NAME_RE = re.compile(r"^[A-Za-z0-9_]+$")
EGUI_APP_RE = re.compile(r'^\s*<EguiApp Condition="([^"]+)">(.*)</EguiApp>\s*$')
EGUI_APP_SUB_RE = re.compile(r'^\s*<EguiAppSub Condition="([^"]+)">(.*)</EguiAppSub>\s*$')
EGUI_APP_CONDITION_RE = re.compile(r"^'\$\(Configuration\)'=='([^']+)'$")


@dataclass(frozen=True)
class Entry:
    app: str
    app_sub: str
    flavor: str
    platform: str

    @property
    def config(self) -> str:
        stem = self.app if not self.app_sub else f"{self.app}_{self.app_sub}"
        return f"{stem}_{self.flavor}"

    @property
    def solution_config(self) -> str:
        return f"{self.config}|{self.platform}"


@dataclass(frozen=True)
class ProjectItem:
    item_type: str
    include: str
    condition: str = ""
    filter_name: str = ""


GENERATED_ITEMS_BEGIN = "  <!-- BEGIN EmbeddedGUI generated project items -->"
GENERATED_ITEMS_END = "  <!-- END EmbeddedGUI generated project items -->"
FILTER_ITEMS_BEGIN = "  <!-- BEGIN EmbeddedGUI generated filter items -->"
FILTER_ITEMS_END = "  <!-- END EmbeddedGUI generated filter items -->"
VISUAL_STUDIO_PROJECT_DIR = Path("porting") / "pc"
VISUAL_STUDIO_PROPS_REPO_PATH = "porting\\pc\\EmbeddedGUI.VisualStudio.props"
CMAKE_PRESETS_REPO_PATH = "CMakePresets.json"


def repo_root_from_script() -> Path:
    return Path(__file__).resolve().parents[2]


def detect_newline(text: str) -> str:
    return "\r\n" if "\r\n" in text else "\n"


def read_lines(path: Path) -> tuple[list[str], str, bool]:
    text = path.read_text(encoding="utf-8")
    newline = detect_newline(text)
    return text.splitlines(), newline, text.endswith(("\n", "\r\n"))


def write_lines(path: Path, lines: list[str], newline: str, had_final_newline: bool) -> None:
    text = newline.join(lines)
    if had_final_newline:
        text += newline
    path.write_text(text, encoding="utf-8")


def validate_name(kind: str, value: str) -> None:
    if not value or not VALID_NAME_RE.match(value):
        raise ValueError(f"Invalid {kind}: {value!r}. Use letters, digits, and underscores only.")


def validate_entry(root: Path, entry: Entry, allow_missing: bool) -> None:
    validate_name("APP", entry.app)
    if entry.app_sub:
        validate_name("APP_SUB", entry.app_sub)

    app_path = root / "example" / entry.app
    app_sub_path = app_path / entry.app_sub if entry.app_sub else None
    if allow_missing:
        return
    if not app_path.is_dir():
        raise FileNotFoundError(f"APP directory does not exist: {app_path}")
    if app_sub_path is not None and not app_sub_path.is_dir():
        raise FileNotFoundError(f"APP_SUB directory does not exist: {app_sub_path}")


def parse_entry_token(token: str, flavor: str, platform: str) -> Entry:
    if "/" in token:
        app, app_sub = token.split("/", 1)
    elif ":" in token:
        app, app_sub = token.split(":", 1)
    else:
        app, app_sub = token, ""
    return Entry(app=app, app_sub=app_sub, flavor=flavor, platform=platform)


def build_mk_declares_app_sub(path: Path) -> bool:
    if not path.is_file():
        return False

    for line in path.read_text(encoding="utf-8").splitlines():
        stripped = line.strip()
        if not stripped or stripped.startswith("#"):
            continue
        if re.match(r"^APP_SUB\s*(?:\?|:|\+)?=", stripped):
            return True
    return False


def discover_app_names(root: Path) -> list[str]:
    example_root = root / "example"
    if not example_root.is_dir():
        return []

    apps = []
    for path in example_root.iterdir():
        if path.is_dir() and (path / "build.mk").is_file() and VALID_NAME_RE.match(path.name):
            apps.append(path.name)
    return sorted(apps)


def discover_app_sub_names(root: Path, app: str) -> list[str]:
    app_root = root / "example" / app
    if not build_mk_declares_app_sub(app_root / "build.mk"):
        return []

    app_subs = []
    for path in app_root.iterdir():
        if path.is_dir() and (path / "app_egui_config.h").is_file() and VALID_NAME_RE.match(path.name):
            app_subs.append(path.name)
    return sorted(app_subs)


def discover_entries(root: Path, flavor: str, platform: str) -> list[Entry]:
    entries: list[Entry] = []
    for app in discover_app_names(root):
        app_subs = discover_app_sub_names(root, app)
        if app_subs:
            entries.extend(Entry(app=app, app_sub=app_sub, flavor=flavor, platform=platform) for app_sub in app_subs)
        else:
            entries.append(Entry(app=app, app_sub="", flavor=flavor, platform=platform))
    return entries


def dedupe_entries(entries: list[Entry]) -> list[Entry]:
    deduped: list[Entry] = []
    seen: set[tuple[str, str, str, str]] = set()
    for entry in entries:
        key = (entry.app, entry.app_sub, entry.flavor, entry.platform)
        if key not in seen:
            seen.add(key)
            deduped.append(entry)
    return deduped


def repo_relative_path(root: Path, path: Path) -> str:
    return str(path.relative_to(root)).replace("/", "\\")


def project_relative_path(root: Path, project_dir: Path, include: str) -> str:
    return os.path.relpath(root / include, project_dir).replace("/", "\\")


def xml_attr(value: str) -> str:
    return value.replace("&", "&amp;").replace('"', "&quot;").replace("<", "&lt;").replace(">", "&gt;")


def app_condition(app: str, app_sub: str | None = None) -> str:
    condition = f"'$(EguiApp)'=='{app}'"
    if app_sub is not None:
        condition += f" And '$(EguiAppSub)'=='{app_sub}'"
    return condition


def add_project_item(items: list[ProjectItem], item_type: str, include: str, condition: str = "", filter_name: str = "") -> None:
    items.append(ProjectItem(item_type=item_type, include=include, condition=condition, filter_name=filter_name))


def list_files(root: Path, directory: str, pattern: str, recursive: bool = False) -> list[str]:
    base = root / directory
    if not base.is_dir():
        return []
    iterator = base.rglob(pattern) if recursive else base.glob(pattern)
    return sorted(repo_relative_path(root, path) for path in iterator if path.is_file())


def read_make_logical_lines(path: Path) -> list[str]:
    if not path.is_file():
        return []

    logical_lines: list[str] = []
    current = ""
    for raw_line in path.read_text(encoding="utf-8").splitlines():
        line = raw_line.split("#", 1)[0].rstrip()
        if not line and not current:
            continue
        if line.endswith("\\"):
            current += line[:-1] + " "
            continue
        current += line
        stripped = current.strip()
        if stripped:
            logical_lines.append(stripped)
        current = ""

    stripped = current.strip()
    if stripped:
        logical_lines.append(stripped)
    return logical_lines


def expand_make_path(value: str, variables: dict[str, str]) -> str:
    def replace_variable(match: re.Match[str]) -> str:
        name = match.group(1)
        return variables.get(name, match.group(0))

    expanded = re.sub(r"\$\(([^)]+)\)", replace_variable, value.strip().strip('"').strip("'"))
    if "$(" in expanded or "*" in expanded or ";" in expanded or "@(" in expanded:
        return ""
    return expanded.replace("/", "\\").strip("\\")


def read_make_paths(path: Path, variable_names: set[str], variables: dict[str, str]) -> list[str]:
    values: list[str] = []
    assignment_re = re.compile(r"^([A-Za-z0-9_]+)\s*\+=\s*(.*)$")
    for line in read_make_logical_lines(path):
        match = assignment_re.match(line)
        if not match or match.group(1) not in variable_names:
            continue
        for token in match.group(2).split():
            expanded = expand_make_path(token, variables)
            if expanded:
                values.append(expanded)
    return values


def extend_missing_paths(paths: list[str], extra_paths: list[str]) -> list[str]:
    result = list(paths)
    seen = {path.casefold() for path in result}
    for path in extra_paths:
        key = path.casefold()
        if key not in seen:
            seen.add(key)
            result.append(path)
    return result


def classify_example_source(root: Path, include: str) -> list[ProjectItem]:
    parts = Path(include).parts
    if len(parts) < 3 or parts[0] != "example":
        return []

    app = parts[1]
    rest = list(parts[2:])
    items: list[ProjectItem] = []

    if len(rest) == 1:
        add_project_item(items, "ClCompile", include, app_condition(app), "Source Files")
        return items

    if rest[0] == "resource":
        add_project_item(items, "ClCompile", include, app_condition(app, ""), "Source Files")
        return items

    app_sub = rest[0]
    if len(rest) == 2:
        add_project_item(items, "ClCompile", include, app_condition(app, app_sub), "Source Files")
        return items

    if len(rest) >= 3 and rest[1] == "resource":
        add_project_item(items, "ClCompile", include, app_condition(app, app_sub), "Source Files")
        return items

    return []


def build_compile_items(root: Path) -> list[ProjectItem]:
    items: list[ProjectItem] = []

    always_compile_files = [
        "driver\\lcd\\egui_lcd.c",
        "driver\\touch\\egui_touch.c",
        "porting\\pc\\egui_hal_sdl_sim.c",
        "porting\\pc\\egui_port_pc.c",
        "porting\\pc\\main.c",
        "porting\\pc\\sdl_port.c",
        "third_party\\plutosvg\\source\\plutosvg.c",
    ]
    always_compile_dirs = [
        "src\\anim",
        "src\\app",
        "src\\background",
        "src\\canvas",
        "src\\core",
        "src\\font",
        "src\\image",
        "src\\mask",
        "src\\resource",
        "src\\shadow",
        "src\\style",
        "src\\utils",
        "src\\utils\\simple_ringbuffer",
        "src\\widget",
        "third_party\\plutovg\\source",
    ]

    generated_compile_files = read_make_paths(root / "src" / "build.mk", {"EGUI_CODE_SRC_FILES"}, {"EGUI_PATH": "src"}) + read_make_paths(
        root / "porting" / "pc" / "build.mk",
        {"EGUI_CODE_SRC_FILES"},
        {"EGUI_PORT_PATH": "porting\\pc"},
    )
    always_compile_dirs = extend_missing_paths(
        always_compile_dirs,
        read_make_paths(root / "src" / "build.mk", {"EGUI_CODE_SRC"}, {"EGUI_PATH": "src"})
        + read_make_paths(root / "porting" / "pc" / "build.mk", {"SRC"}, {"EGUI_PORT_PATH": "porting\\pc"}),
    )

    for include in always_compile_files:
        if (root / include).is_file():
            add_project_item(items, "ClCompile", include, filter_name="Source Files")
    for directory in always_compile_dirs:
        for include in list_files(root, directory, "*.c"):
            add_project_item(items, "ClCompile", include, filter_name="Source Files")
    for include in generated_compile_files:
        if (root / include).is_file():
            add_project_item(items, "ClCompile", include, filter_name="Source Files")

    for include in list_files(root, "example", "*.c", recursive=True):
        items.extend(classify_example_source(root, include))

    for include in list_files(root, "example\\HelloUnitTest\\test", "*.c"):
        add_project_item(items, "ClCompile", include, app_condition("HelloUnitTest"), "Source Files")
    for include in list_files(root, "src\\test", "*.c"):
        add_project_item(items, "ClCompile", include, app_condition("HelloUnitTest"), "Source Files")

    add_project_item(
        items,
        "ClCompile",
        "example\\HelloBasic\\file_image\\file_io_stdio.c",
        app_condition("HelloBasic", "deferred_image"),
        "Source Files",
    )
    add_project_item(
        items,
        "ClCompile",
        "example\\HelloBasic\\file_image\\mount_router_template\\file_io_mount_router_template.c",
        app_condition("HelloBasic", "file_image"),
        "Source Files",
    )

    hello_basic_file_image = app_condition("HelloBasic", "file_image")
    hello_basic_deferred_image = app_condition("HelloBasic", "deferred_image")
    add_project_item(
        items,
        "ClCompile",
        "third_party\\file_image\\decoder_bmp_stream.c",
        f"({hello_basic_file_image}) Or ({hello_basic_deferred_image})",
        "Source Files",
    )
    for include in [
        "third_party\\file_image\\decoder_stb.c",
        "third_party\\file_image\\decoder_tjpgd_stream.c",
        "third_party\\file_image\\third_party_tjpgd.c",
    ]:
        add_project_item(items, "ClCompile", include, hello_basic_file_image, "Source Files")

    for include in [
        "example\\HelloBasic\\file_image\\decoder_registry.c",
        "example\\HelloBasic\\file_image\\file_image_stack.c",
        "example\\HelloBasic\\file_image\\mount_router_template\\file_io_mount_router_template.c",
        "third_party\\file_image\\decoder_bmp_stream.c",
        "third_party\\file_image\\decoder_stb.c",
        "third_party\\file_image\\decoder_tjpgd_stream.c",
        "third_party\\file_image\\third_party_tjpgd.c",
    ]:
        add_project_item(items, "ClCompile", include, app_condition("HelloPerformance"), "Source Files")

    seen: set[tuple[str, str, str]] = set()
    deduped: list[ProjectItem] = []
    for item in items:
        key = (item.item_type, item.include, item.condition)
        if key not in seen:
            seen.add(key)
            deduped.append(item)
    return deduped


def build_include_items(root: Path) -> list[ProjectItem]:
    items: list[ProjectItem] = []
    include_files = [
        "driver\\lcd\\egui_lcd.h",
        "driver\\touch\\egui_touch.h",
    ]
    include_dirs = [
        ("example", True),
        ("porting\\pc", False),
        ("src", True),
        ("third_party\\file_image", False),
        ("third_party\\file_image\\third_party", False),
        ("third_party\\plutosvg\\source", False),
        ("third_party\\plutovg\\include", False),
        ("third_party\\plutovg\\source", False),
    ]
    generated_include_dirs = read_make_paths(root / "src" / "build.mk", {"EGUI_CODE_INCLUDE"}, {"EGUI_PATH": "src"}) + read_make_paths(
        root / "driver" / "build.mk",
        {"EGUI_CODE_INCLUDE"},
        {},
    ) + read_make_paths(
        root / "porting" / "pc" / "build.mk",
        {"INCLUDE"},
        {"EGUI_PORT_PATH": "porting\\pc"},
    )
    include_dirs = extend_missing_paths([f"{directory}|{int(recursive)}" for directory, recursive in include_dirs], [f"{directory}|0" for directory in generated_include_dirs])
    normalized_include_dirs = [(value.rsplit("|", 1)[0], value.rsplit("|", 1)[1] == "1") for value in include_dirs]

    for include in include_files:
        add_project_item(items, "ClInclude", include, filter_name="Header Files")
    for directory, recursive in normalized_include_dirs:
        for include in list_files(root, directory, "*.h", recursive=recursive):
            add_project_item(items, "ClInclude", include, filter_name="Header Files")

    seen: set[str] = set()
    deduped: list[ProjectItem] = []
    for item in items:
        if item.include not in seen:
            seen.add(item.include)
            deduped.append(item)
    return deduped


def format_project_item(root: Path, project_dir: Path, item: ProjectItem) -> str:
    condition = f' Condition="{xml_attr(item.condition)}"' if item.condition else ""
    include = project_relative_path(root, project_dir, item.include)
    return f'    <{item.item_type} Include="{xml_attr(include)}"{condition} />'


def format_filter_item(root: Path, project_dir: Path, item: ProjectItem) -> str:
    include = project_relative_path(root, project_dir, item.include)
    return f'    <{item.item_type} Include="{xml_attr(include)}"><Filter>{xml_attr(filter_name_for_item(item))}</Filter></{item.item_type}>'


def root_filter_for_item(item: ProjectItem) -> str:
    if item.item_type == "ClCompile":
        return "Source Files"
    if item.item_type == "ClInclude":
        return "Header Files"
    if item.item_type == "None":
        return "Configuration"
    return item.filter_name


def filter_name_for_item(item: ProjectItem) -> str:
    root_filter = root_filter_for_item(item)
    if item.item_type not in ("ClCompile", "ClInclude"):
        return root_filter

    parts = item.include.replace("/", "\\").split("\\")
    parent_parts = [part for part in parts[:-1] if part and part != "."]
    if not parent_parts:
        return root_filter
    return root_filter + "\\" + "\\".join(parent_parts)


def filter_parent_chain(filter_name: str) -> list[str]:
    parts = filter_name.split("\\")
    return ["\\".join(parts[:index]) for index in range(1, len(parts) + 1)]


def generated_filter_definitions(project_items: list[ProjectItem]) -> list[str]:
    generated_roots = {"Source Files", "Header Files", "Configuration"}
    filters: set[str] = set()
    for item in project_items:
        filter_name = filter_name_for_item(item)
        for parent in filter_parent_chain(filter_name):
            if parent not in generated_roots:
                filters.add(parent)

    return [
        f'    <Filter Include="{xml_attr(filter_name)}"><UniqueIdentifier>{stable_filter_guid(filter_name)}</UniqueIdentifier></Filter>'
        for filter_name in sorted(filters, key=str.casefold)
    ]


def stable_filter_guid(filter_name: str) -> str:
    value = uuid.uuid5(uuid.NAMESPACE_URL, f"EmbeddedGUI.VisualStudio.Filter:{filter_name}")
    return "{" + str(value).upper() + "}"


def generated_vcxproj_item_block(root: Path, project_dir: Path) -> list[str]:
    compile_items = build_compile_items(root)
    include_items = build_include_items(root)
    config_items = [
        ProjectItem("None", VISUAL_STUDIO_PROPS_REPO_PATH, filter_name="Configuration"),
        ProjectItem("None", CMAKE_PRESETS_REPO_PATH, filter_name="Configuration"),
    ]

    return (
        [GENERATED_ITEMS_BEGIN, '  <ItemGroup Label="EmbeddedGUI Generated C Sources">']
        + [format_project_item(root, project_dir, item) for item in compile_items]
        + ['  </ItemGroup>', '  <ItemGroup Label="EmbeddedGUI Generated Headers">']
        + [format_project_item(root, project_dir, item) for item in include_items]
        + [
            "  </ItemGroup>",
            '  <ItemGroup Label="EmbeddedGUI Generated Configuration Files">',
        ]
        + [format_project_item(root, project_dir, item) for item in config_items]
        + ["  </ItemGroup>", GENERATED_ITEMS_END]
    )


def generated_filter_item_block(root: Path, project_dir: Path) -> list[str]:
    project_items = build_compile_items(root) + build_include_items(root)
    unique_items: list[ProjectItem] = []
    seen: set[tuple[str, str]] = set()
    for item in project_items:
        key = (item.item_type, item.include)
        if key not in seen:
            seen.add(key)
            unique_items.append(item)

    source_items = [item for item in unique_items if item.item_type == "ClCompile"]
    header_items = [item for item in unique_items if item.item_type == "ClInclude"]
    config_items = [
        ProjectItem("None", VISUAL_STUDIO_PROPS_REPO_PATH, filter_name="Configuration"),
        ProjectItem("None", CMAKE_PRESETS_REPO_PATH, filter_name="Configuration"),
    ]

    return (
        [FILTER_ITEMS_BEGIN, '  <ItemGroup Label="EmbeddedGUI Generated Filter Definitions">']
        + generated_filter_definitions(unique_items)
        + ['  </ItemGroup>', '  <ItemGroup Label="EmbeddedGUI Generated Source Filters">']
        + [format_filter_item(root, project_dir, item) for item in source_items]
        + ['  </ItemGroup>', '  <ItemGroup Label="EmbeddedGUI Generated Header Filters">']
        + [format_filter_item(root, project_dir, item) for item in header_items]
        + [
            "  </ItemGroup>",
            '  <ItemGroup Label="EmbeddedGUI Generated Configuration Filters">',
        ]
        + [format_filter_item(root, project_dir, item) for item in config_items]
        + ["  </ItemGroup>", FILTER_ITEMS_END]
    )


def find_line(lines: list[str], predicate, description: str) -> int:
    for index, line in enumerate(lines):
        if predicate(line):
            return index
    raise ValueError(f"Could not find insertion point: {description}")


def find_project_guid(sln_lines: list[str]) -> str:
    for line in sln_lines:
        match = PROJECT_GUID_RE.match(line)
        if match:
            return match.group(1)
    raise ValueError("Could not find EmbeddedGUI_PC_Simulator project GUID in solution.")


def insert_before_first_release_project_config(lines: list[str], block: list[str]) -> bool:
    insert_index = find_line(
        lines,
        lambda line: '<ProjectConfiguration Include="Release|' in line,
        "first Release ProjectConfiguration in vcxproj",
    )
    lines[insert_index:insert_index] = block
    return True


def ensure_vcxproj_project_config(lines: list[str], entry: Entry) -> bool:
    include = f'<ProjectConfiguration Include="{entry.solution_config}">'
    if any(include in line for line in lines):
        return False

    block = [
        f'    <ProjectConfiguration Include="{entry.solution_config}">',
        f"      <Configuration>{entry.config}</Configuration>",
        f"      <Platform>{entry.platform}</Platform>",
        "    </ProjectConfiguration>",
    ]
    return insert_before_first_release_project_config(lines, block)


def named_app_group_bounds(lines: list[str]) -> tuple[int, int]:
    group_start = find_line(
        lines,
        lambda line: '<PropertyGroup Label="EmbeddedGUI Named App Configurations">' in line,
        "EmbeddedGUI Named App Configurations property group",
    )
    group_end = find_line(
        lines[group_start + 1 :],
        lambda line: line.strip() == "</PropertyGroup>",
        "end of EmbeddedGUI Named App Configurations property group",
    )
    return group_start, group_start + 1 + group_end


def config_from_named_app_condition(condition: str) -> str:
    match = EGUI_APP_CONDITION_RE.match(html.unescape(condition))
    return match.group(1) if match else ""


def read_vcxproj_named_apps(lines: list[str]) -> list[tuple[str, str, str]]:
    group_start, group_end = named_app_group_bounds(lines)
    app_by_config: dict[str, str] = {}
    sub_by_config: dict[str, str] = {}
    order: list[str] = []

    for line in lines[group_start + 1 : group_end]:
        app_match = EGUI_APP_RE.match(line)
        sub_match = EGUI_APP_SUB_RE.match(line)
        if app_match:
            config = config_from_named_app_condition(app_match.group(1))
            if config and config not in app_by_config:
                app_by_config[config] = html.unescape(app_match.group(2))
                order.append(config)
            continue
        if sub_match:
            config = config_from_named_app_condition(sub_match.group(1))
            if config and config not in sub_by_config:
                sub_by_config[config] = html.unescape(sub_match.group(2))

    mappings = []
    for config in order:
        if config in app_by_config and config in sub_by_config:
            mappings.append((config, app_by_config[config], sub_by_config[config]))
    return mappings


def format_named_app_mapping(config: str, app: str, app_sub: str) -> list[str]:
    condition = f"'$(Configuration)'=='{config}'"
    return [
        f'    <EguiApp Condition="{xml_attr(condition)}">{xml_attr(app)}</EguiApp>',
        f'    <EguiAppSub Condition="{xml_attr(condition)}">{xml_attr(app_sub)}</EguiAppSub>',
    ]


def ensure_vcxproj_named_apps(lines: list[str], entries: list[Entry]) -> bool:
    group_start, group_end = named_app_group_bounds(lines)
    mappings = read_vcxproj_named_apps(lines)
    mapping_by_config = {config: (app, app_sub) for config, app, app_sub in mappings}
    ordered_configs = [config for config, _, _ in mappings]

    for entry in entries:
        if entry.config not in mapping_by_config:
            mapping_by_config[entry.config] = (entry.app, entry.app_sub)
            ordered_configs.append(entry.config)

    replacement = [lines[group_start]]
    for config in ordered_configs:
        app, app_sub = mapping_by_config[config]
        replacement.extend(format_named_app_mapping(config, app, app_sub))
    replacement.append(lines[group_end])

    current = lines[group_start : group_end + 1]
    if current == replacement:
        return False
    lines[group_start : group_end + 1] = replacement
    return True


def ensure_sln_solution_config(lines: list[str], entry: Entry) -> bool:
    config_line = f"\t\t{entry.solution_config} = {entry.solution_config}"
    if config_line in lines:
        return False

    insert_index = find_line(
        lines,
        lambda line: line == "\t\tRelease|x64 = Release|x64",
        "Release|x64 solution configuration",
    )
    lines[insert_index:insert_index] = [config_line]
    return True


def ensure_sln_project_config(lines: list[str], entry: Entry, project_guid: str) -> bool:
    active_line = f"\t\t{project_guid}.{entry.solution_config}.ActiveCfg = {entry.solution_config}"
    build_line = f"\t\t{project_guid}.{entry.solution_config}.Build.0 = {entry.solution_config}"
    if active_line in lines and build_line in lines:
        return False

    insert_index = find_line(
        lines,
        lambda line: line.startswith(f"\t\t{project_guid}.Release|x64.ActiveCfg"),
        "Release|x64 project configuration",
    )
    block = []
    if active_line not in lines:
        block.append(active_line)
    if build_line not in lines:
        block.append(build_line)
    lines[insert_index:insert_index] = block
    return bool(block)


def replace_between_markers(lines: list[str], begin_marker: str, end_marker: str, replacement: list[str], insert_before: str) -> bool:
    try:
        begin_index = lines.index(begin_marker)
        end_index = lines.index(end_marker, begin_index + 1)
        current = lines[begin_index : end_index + 1]
        if current == replacement:
            return False
        lines[begin_index : end_index + 1] = replacement
        return True
    except ValueError:
        insert_index = find_line(lines, lambda line: line.strip() == insert_before, insert_before)
        lines[insert_index:insert_index] = replacement
        return True


def remove_legacy_item_groups(lines: list[str]) -> bool:
    changed = False
    index = 0
    legacy_tokens = (
        '    <ClCompile Include=',
        '    <ClInclude Include=',
        '    <None Include="EmbeddedGUI.VisualStudio.props"',
        '    <None Include="CMakePresets.json"',
    )
    while index < len(lines):
        if lines[index].strip() == "<ItemGroup>":
            end_index = index + 1
            while end_index < len(lines) and lines[end_index].strip() != "</ItemGroup>":
                end_index += 1
            if end_index < len(lines) and any(any(token in line for token in legacy_tokens) for line in lines[index + 1 : end_index]):
                del lines[index : end_index + 1]
                changed = True
                continue
        index += 1
    return changed


def ensure_vcxproj_generated_items(root: Path, project_dir: Path, lines: list[str]) -> bool:
    changed = remove_legacy_item_groups(lines)
    replacement = generated_vcxproj_item_block(root, project_dir)
    if replace_between_markers(lines, GENERATED_ITEMS_BEGIN, GENERATED_ITEMS_END, replacement, '<Import Project="$(VCTargetsPath)\\Microsoft.Cpp.targets" />'):
        changed = True
    return changed


def remove_legacy_filter_item_groups(lines: list[str]) -> bool:
    changed = False
    index = 0
    legacy_tokens = (
        "    <ClCompile Include=",
        "    <ClInclude Include=",
        '    <None Include="EmbeddedGUI.VisualStudio.props"',
        '    <None Include="CMakePresets.json"',
    )
    while index < len(lines):
        if lines[index].strip() == "<ItemGroup>":
            end_index = index + 1
            while end_index < len(lines) and lines[end_index].strip() != "</ItemGroup>":
                end_index += 1
            if end_index < len(lines) and any(any(token in line for token in legacy_tokens) for line in lines[index + 1 : end_index]):
                del lines[index : end_index + 1]
                changed = True
                continue
        index += 1
    return changed


def ensure_filters_generated_items(root: Path, project_dir: Path, lines: list[str]) -> bool:
    changed = remove_legacy_filter_item_groups(lines)
    replacement = generated_filter_item_block(root, project_dir)
    if replace_between_markers(lines, FILTER_ITEMS_BEGIN, FILTER_ITEMS_END, replacement, "</Project>"):
        changed = True
    return changed


def update_files(root: Path, entries: list[Entry], dry_run: bool) -> list[str]:
    project_dir = root / VISUAL_STUDIO_PROJECT_DIR
    sln_path = project_dir / "EmbeddedGUI_PC_Simulator.sln"
    vcxproj_path = project_dir / "EmbeddedGUI_PC_Simulator.vcxproj"
    filters_path = project_dir / "EmbeddedGUI_PC_Simulator.vcxproj.filters"

    sln_lines, sln_newline, sln_final_newline = read_lines(sln_path)
    vcxproj_lines, vcxproj_newline, vcxproj_final_newline = read_lines(vcxproj_path)
    filters_lines, filters_newline, filters_final_newline = read_lines(filters_path)
    project_guid = find_project_guid(sln_lines)

    changes: list[str] = []
    for entry in entries:
        if ensure_vcxproj_project_config(vcxproj_lines, entry):
            changes.append(f"vcxproj ProjectConfiguration: {entry.solution_config}")
        if ensure_sln_solution_config(sln_lines, entry):
            changes.append(f"sln SolutionConfigurationPlatforms: {entry.solution_config}")
        if ensure_sln_project_config(sln_lines, entry, project_guid):
            changes.append(f"sln ProjectConfigurationPlatforms: {entry.solution_config}")

    if ensure_vcxproj_named_apps(vcxproj_lines, entries):
        changes.append("vcxproj EguiApp mappings")
    if ensure_vcxproj_generated_items(root, project_dir, vcxproj_lines):
        changes.append("vcxproj generated project items")
    if ensure_filters_generated_items(root, project_dir, filters_lines):
        changes.append("vcxproj.filters generated project items")

    if changes and not dry_run:
        write_lines(sln_path, sln_lines, sln_newline, sln_final_newline)
        write_lines(vcxproj_path, vcxproj_lines, vcxproj_newline, vcxproj_final_newline)
        write_lines(filters_path, filters_lines, filters_newline, filters_final_newline)
    return changes


def build_arg_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description="Synchronize EmbeddedGUI_PC_Simulator.sln and .vcxproj.")
    parser.add_argument(
        "--entry",
        action="append",
        default=[],
        metavar="APP[/APP_SUB]",
        help="specific configuration to sync, for example HelloDirty or HelloBasic/checkbox; can be repeated",
    )
    parser.add_argument("--app", help="APP name to sync as a single entry")
    parser.add_argument("--app-sub", default="", help="APP_SUB name used with --app")
    parser.add_argument("--flavor", default="Debug", choices=("Debug", "Release"), help="configuration flavor")
    parser.add_argument("--platform", default="x64", choices=("x64",), help="Visual Studio platform; this solution is x64-only")
    parser.add_argument("--allow-missing", action="store_true", help="do not require example directories to exist")
    parser.add_argument(
        "--all",
        action="store_true",
        help="discover all example apps and sub-apps; this is the default when no --entry/--app is provided",
    )
    parser.add_argument("--dry-run", action="store_true", help="print changes without writing files")
    parser.add_argument("--check", action="store_true", help="exit with code 1 if files need updates")
    return parser


def main(argv: list[str] | None = None) -> int:
    parser = build_arg_parser()
    args = parser.parse_args(argv)

    tokens = list(args.entry)
    if args.app:
        tokens.append(f"{args.app}/{args.app_sub}" if args.app_sub else args.app)
    root = repo_root_from_script()
    entries: list[Entry] = []
    if args.all or not tokens:
        entries.extend(discover_entries(root, args.flavor, args.platform))
    entries.extend(parse_entry_token(token, args.flavor, args.platform) for token in tokens)
    entries = dedupe_entries(entries)
    try:
        for entry in entries:
            validate_entry(root, entry, args.allow_missing)
        changes = update_files(root, entries, dry_run=args.dry_run or args.check)
    except Exception as exc:  # noqa: BLE001 - command-line tool should print concise failures.
        print(f"error: {exc}", file=sys.stderr)
        return 2

    if changes:
        verb = "would update" if (args.dry_run or args.check) else "updated"
        print(f"{verb}:")
        for change in changes:
            print(f"  - {change}")
        return 1 if args.check else 0

    print("Visual Studio solution is already up to date.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
