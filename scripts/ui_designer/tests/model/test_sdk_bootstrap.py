"""Tests for ui_designer.model.sdk_bootstrap."""

from pathlib import Path
from types import SimpleNamespace

import pytest

from ui_designer.model.sdk_bootstrap import default_sdk_install_dir, download_and_extract_sdk, ensure_sdk_downloaded
from ui_designer.model.workspace import normalize_path


def _create_sdk_root(root: Path):
    (root / "src").mkdir(parents=True)
    (root / "porting" / "designer").mkdir(parents=True)
    (root / "Makefile").write_text("all:\n", encoding="utf-8")


class TestSdkBootstrap:
    def test_default_sdk_install_dir_uses_config_dir(self, tmp_path, monkeypatch):
        monkeypatch.setattr("ui_designer.model.sdk_bootstrap._get_config_dir", lambda: str(tmp_path / "config"))

        assert default_sdk_install_dir() == normalize_path(str(tmp_path / "config" / "sdk" / "EmbeddedGUI"))

    def test_ensure_sdk_downloaded_reuses_existing_sdk(self, tmp_path):
        sdk_root = tmp_path / "sdk"
        _create_sdk_root(sdk_root)

        assert ensure_sdk_downloaded(str(sdk_root)) == normalize_path(str(sdk_root))

    def test_ensure_sdk_downloaded_falls_back_to_gitee_clone(self, tmp_path, monkeypatch):
        target_dir = tmp_path / "sdk" / "EmbeddedGUI"
        calls = []

        def fake_download(url, destination_dir, progress_callback=None):
            calls.append(("download", url))
            raise RuntimeError("archive unavailable")

        def fake_clone(url, destination_dir, progress_callback=None):
            calls.append(("clone", url))
            _create_sdk_root(Path(destination_dir))
            return str(destination_dir)

        monkeypatch.setattr("ui_designer.model.sdk_bootstrap.download_and_extract_sdk", fake_download)
        monkeypatch.setattr("ui_designer.model.sdk_bootstrap.clone_sdk_from_git", fake_clone)

        result = ensure_sdk_downloaded(str(target_dir))

        assert result == normalize_path(str(target_dir))
        assert calls[0][0] == "download"
        assert calls[1][0] == "download"
        assert calls[2][0] == "clone"

    def test_ensure_sdk_downloaded_reports_all_attempts(self, tmp_path, monkeypatch):
        target_dir = tmp_path / "sdk" / "EmbeddedGUI"

        monkeypatch.setattr(
            "ui_designer.model.sdk_bootstrap.download_and_extract_sdk",
            lambda url, destination_dir, progress_callback=None: (_ for _ in ()).throw(RuntimeError(f"download failed: {url}")),
        )
        monkeypatch.setattr(
            "ui_designer.model.sdk_bootstrap.clone_sdk_from_git",
            lambda url, destination_dir, progress_callback=None: (_ for _ in ()).throw(RuntimeError("clone failed")),
        )

        with pytest.raises(RuntimeError, match="GitHub archive"):
            ensure_sdk_downloaded(str(target_dir))

    def test_download_and_extract_sdk_rejects_html_response(self, tmp_path, monkeypatch):
        target_dir = tmp_path / "sdk" / "EmbeddedGUI"

        def fake_download(url, destination_dir, progress_callback=None):
            Path(destination_dir).write_text("<!DOCTYPE html><html>blocked</html>", encoding="utf-8")
            return SimpleNamespace(content_type="text/html; charset=utf-8", final_url=url)

        monkeypatch.setattr("ui_designer.model.sdk_bootstrap._download_file", fake_download)

        with pytest.raises(RuntimeError, match="server returned HTML instead of a zip archive"):
            download_and_extract_sdk("https://gitee.example/archive.zip", str(target_dir))

    def test_download_file_times_out_and_allows_fallback(self, tmp_path, monkeypatch):
        from ui_designer.model import sdk_bootstrap as module

        class FakeResponse:
            headers = {"Content-Length": "1024", "Content-Type": "application/zip"}

            def __enter__(self):
                return self

            def __exit__(self, exc_type, exc, tb):
                return False

            def geturl(self):
                return "https://example.invalid/sdk.zip"

            def read(self, _size):
                return b"x" * 16

        tick_values = iter([0, module._get_download_timeout_seconds() + 1])

        monkeypatch.setattr("ui_designer.model.sdk_bootstrap.urllib.request.urlopen", lambda *args, **kwargs: FakeResponse())
        monkeypatch.setattr("ui_designer.model.sdk_bootstrap.time.monotonic", lambda: next(tick_values))

        with pytest.raises(RuntimeError, match="download timed out"):
            module._download_file("https://example.invalid/sdk.zip", tmp_path / "sdk.zip")
