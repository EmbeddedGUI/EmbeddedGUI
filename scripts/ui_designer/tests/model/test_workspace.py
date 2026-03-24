"""Tests for ui_designer.model.workspace helpers."""

import os
from pathlib import Path

import pytest

from ui_designer.model.workspace import (
    compute_make_app_root_arg,
    describe_sdk_root,
    find_sdk_root,
    infer_sdk_root_from_project_dir,
    is_valid_sdk_root,
    normalize_path,
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
