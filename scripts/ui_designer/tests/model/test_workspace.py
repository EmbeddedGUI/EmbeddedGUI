"""Tests for ui_designer.model.workspace helpers."""

import os
from pathlib import Path

import pytest

import ui_designer.model.workspace as workspace_module
from ui_designer.model.workspace import (
    compute_make_app_root_arg,
    describe_sdk_root,
    find_sdk_root,
    infer_sdk_root_from_project_dir,
    is_valid_sdk_root,
    normalize_path,
    resolve_sdk_root_candidate,
    resolve_project_sdk_root,
    serialize_sdk_root,
)


def _create_sdk_root(root: Path):
    (root / "src").mkdir(parents=True)
    (root / "porting" / "designer").mkdir(parents=True)
    (root / "Makefile").write_text("all:\n")


class TestWorkspaceHelpers:
    def test_normalize_path_empty(self):
        assert normalize_path("") == ""
        assert normalize_path(None) == ""

    def test_is_valid_sdk_root(self, tmp_path):
        sdk_root = tmp_path / "EmbeddedGUI"
        _create_sdk_root(sdk_root)
        assert is_valid_sdk_root(str(sdk_root))
        assert describe_sdk_root(str(sdk_root)) == "ready"
        assert describe_sdk_root(str(tmp_path / "missing")) == "invalid"

    def test_serialize_and_resolve_sdk_root(self, tmp_path):
        project_dir = tmp_path / "workspace" / "App"
        sdk_root = tmp_path / "sdk"
        project_dir.mkdir(parents=True)
        sdk_root.mkdir()

        stored = serialize_sdk_root(str(project_dir), str(sdk_root))
        assert stored
        assert resolve_project_sdk_root(str(project_dir), stored) == normalize_path(str(sdk_root))

    def test_infer_sdk_root_from_project_dir(self, tmp_path):
        sdk_root = tmp_path / "sdk"
        _create_sdk_root(sdk_root)
        app_dir = sdk_root / "example" / "HelloApp"
        app_dir.mkdir(parents=True)
        assert infer_sdk_root_from_project_dir(str(app_dir)) == normalize_path(str(sdk_root))

    def test_compute_make_app_root_arg_for_sdk_example(self, tmp_path):
        sdk_root = tmp_path / "sdk"
        app_dir = sdk_root / "example" / "HelloApp"
        app_dir.mkdir(parents=True)
        assert compute_make_app_root_arg(str(sdk_root), str(app_dir), "HelloApp") == "example"

    def test_compute_make_app_root_arg_for_external_app(self, tmp_path):
        sdk_root = tmp_path / "sdk"
        app_root = tmp_path / "workspace"
        app_dir = app_root / "HelloApp"
        app_dir.mkdir(parents=True)
        assert compute_make_app_root_arg(str(sdk_root), str(app_dir), "HelloApp") == os.path.relpath(str(app_root), str(sdk_root)).replace("\\", "/")

    def test_find_sdk_root_prefers_cli_and_project(self, tmp_path):
        sdk_root = tmp_path / "sdk"
        _create_sdk_root(sdk_root)
        project_dir = tmp_path / "workspace" / "HelloApp"
        project_dir.mkdir(parents=True)

        found = find_sdk_root(cli_sdk_root=str(sdk_root), project_path=str(project_dir))
        assert found == normalize_path(str(sdk_root))

    def test_find_sdk_root_uses_environment(self, tmp_path, monkeypatch):
        sdk_root = tmp_path / "sdk"
        _create_sdk_root(sdk_root)
        monkeypatch.setenv("EMBEDDEDGUI_SDK_ROOT", str(sdk_root))
        assert find_sdk_root() == normalize_path(str(sdk_root))

    def test_resolve_sdk_root_candidate_accepts_parent_with_embeddedgui_child(self, tmp_path):
        parent_dir = tmp_path / "tools"
        sdk_root = parent_dir / "EmbeddedGUI-main"
        parent_dir.mkdir(parents=True)
        _create_sdk_root(sdk_root)

        assert resolve_sdk_root_candidate(str(parent_dir)) == normalize_path(str(sdk_root))

    def test_resolve_sdk_root_candidate_accepts_sdk_container_directory(self, tmp_path):
        sdk_container = tmp_path / "sdk"
        sdk_root = sdk_container / "EmbeddedGUI-main"
        sdk_container.mkdir(parents=True)
        _create_sdk_root(sdk_root)

        assert resolve_sdk_root_candidate(str(sdk_container)) == normalize_path(str(sdk_root))

    def test_find_sdk_root_discovers_plain_sdk_sibling_from_project_path(self, tmp_path, monkeypatch):
        workspace_root = tmp_path / "workspace"
        project_dir = workspace_root / "apps" / "HelloApp"
        sdk_root = workspace_root / "sdk"
        project_dir.mkdir(parents=True)
        _create_sdk_root(sdk_root)

        isolated_cwd = tmp_path / "isolated_cwd"
        isolated_cwd.mkdir()
        monkeypatch.chdir(isolated_cwd)

        assert find_sdk_root(project_path=str(project_dir), env={}) == normalize_path(str(sdk_root))

    def test_find_sdk_root_discovers_plain_sdk_container_near_frozen_designer(self, tmp_path, monkeypatch):
        tools_root = tmp_path / "tools"
        designer_dir = tools_root / "EmbeddedGUI-Designer"
        sdk_root = tools_root / "sdk"
        designer_dir.mkdir(parents=True)
        _create_sdk_root(sdk_root)

        isolated_cwd = tmp_path / "isolated_cwd"
        isolated_cwd.mkdir()
        monkeypatch.chdir(isolated_cwd)
        monkeypatch.setattr(workspace_module.sys, "frozen", True, raising=False)
        monkeypatch.setattr(workspace_module.sys, "executable", str(designer_dir / "EmbeddedGUI-Designer.exe"))

        assert find_sdk_root(env={}) == normalize_path(str(sdk_root))

    def test_find_sdk_root_discovers_embeddedgui_child_under_sdk_container(self, tmp_path, monkeypatch):
        tools_root = tmp_path / "tools"
        designer_dir = tools_root / "EmbeddedGUI-Designer"
        sdk_root = tools_root / "sdk" / "EmbeddedGUI-main"
        designer_dir.mkdir(parents=True)
        _create_sdk_root(sdk_root)

        isolated_cwd = tmp_path / "isolated_cwd"
        isolated_cwd.mkdir()
        monkeypatch.chdir(isolated_cwd)
        monkeypatch.setattr(workspace_module.sys, "frozen", True, raising=False)
        monkeypatch.setattr(workspace_module.sys, "executable", str(designer_dir / "EmbeddedGUI-Designer.exe"))

        assert find_sdk_root(env={}) == normalize_path(str(sdk_root))
