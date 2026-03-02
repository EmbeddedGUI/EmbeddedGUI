"""Tests for DesignerBridge protocol encoding/decoding."""

import struct
import io
import pytest
from unittest.mock import MagicMock, patch, PropertyMock

from ui_designer.engine.designer_bridge import (
    DesignerBridge,
    CMD_RENDER, CMD_TOUCH, CMD_KEY, CMD_QUIT,
    RSP_READY, RSP_FRAME, RSP_ERROR,
    TOUCH_DOWN, TOUCH_UP, TOUCH_MOVE,
)


class FakePipe:
    """Simulates a subprocess pipe with read/write buffers."""

    def __init__(self, read_data=b""):
        self._read_buf = io.BytesIO(read_data)
        self._write_buf = io.BytesIO()

    def read(self, n=-1):
        if n == -1:
            return self._read_buf.read()
        return self._read_buf.read(n)

    def write(self, data):
        self._write_buf.write(data)

    def flush(self):
        pass

    @property
    def written(self):
        return self._write_buf.getvalue()


def _make_bridge_with_pipes(stdout_data=b""):
    """Create a DesignerBridge with fake pipes (skip start())."""
    bridge = DesignerBridge()
    bridge.width = 240
    bridge.height = 320
    bridge.proc = MagicMock()
    bridge.proc.poll.return_value = None  # process is running
    bridge.proc.stdin = FakePipe()
    bridge.proc.stdout = FakePipe(stdout_data)
    return bridge


class TestSendCommands:
    def test_send_render_cmd(self):
        bridge = _make_bridge_with_pipes(
            stdout_data=bytes([RSP_FRAME]) + struct.pack("<I", 3) + b"\xff\x00\x00"
        )
        bridge.render()
        assert bridge.proc.stdin.written[0:1] == bytes([CMD_RENDER])

    def test_send_touch_down_cmd(self):
        bridge = _make_bridge_with_pipes()
        bridge.inject_touch(TOUCH_DOWN, 100, 200)
        written = bridge.proc.stdin.written
        assert written[0] == CMD_TOUCH
        action, x, y = struct.unpack("<BHH", written[1:6])
        assert action == TOUCH_DOWN
        assert x == 100
        assert y == 200

    def test_send_touch_move_cmd(self):
        bridge = _make_bridge_with_pipes()
        bridge.inject_touch(TOUCH_MOVE, 50, 75)
        written = bridge.proc.stdin.written
        assert written[0] == CMD_TOUCH
        action, x, y = struct.unpack("<BHH", written[1:6])
        assert action == TOUCH_MOVE
        assert x == 50
        assert y == 75

    def test_send_key_cmd(self):
        bridge = _make_bridge_with_pipes()
        bridge.inject_key(1, 0x0041)
        written = bridge.proc.stdin.written
        assert written[0] == CMD_KEY
        key_type, code = struct.unpack("<BH", written[1:4])
        assert key_type == 1
        assert code == 0x0041

    def test_stop_sends_quit(self):
        bridge = _make_bridge_with_pipes()
        bridge.proc.wait.return_value = 0
        stdin_pipe = bridge.proc.stdin  # capture before stop() clears proc
        bridge.stop()
        assert stdin_pipe.written[0:1] == bytes([CMD_QUIT])
        assert bridge.proc is None


class TestParseResponses:
    def test_parse_ready_response(self):
        """Test that start() correctly parses RSP_READY."""
        bridge = DesignerBridge()
        mock_proc = MagicMock()
        mock_proc.stdout = FakePipe(bytes([RSP_READY]))
        mock_proc.poll.return_value = None
        with patch("subprocess.Popen", return_value=mock_proc):
            bridge.start("fake.exe", "fake.bin", 240, 320)
        assert bridge.width == 240
        assert bridge.height == 320

    def test_parse_ready_wrong_response(self):
        """Test that start() raises on unexpected response."""
        bridge = DesignerBridge()
        mock_proc = MagicMock()
        mock_proc.stdout = FakePipe(bytes([0x99]))
        with patch("subprocess.Popen", return_value=mock_proc):
            with pytest.raises(RuntimeError, match="Expected READY"):
                bridge.start("fake.exe", "fake.bin", 240, 320)

    def test_parse_frame_response(self):
        """Test render() returns correct frame data."""
        frame_data = b"\x01\x02\x03" * 100
        stdout_data = (
            bytes([RSP_FRAME])
            + struct.pack("<I", len(frame_data))
            + frame_data
        )
        bridge = _make_bridge_with_pipes(stdout_data)
        result = bridge.render()
        assert result == frame_data
        assert len(result) == 300

    def test_parse_error_response(self):
        """Test render() raises on RSP_ERROR."""
        err_msg = b"something went wrong"
        stdout_data = (
            bytes([RSP_ERROR])
            + struct.pack("<H", len(err_msg))
            + err_msg
        )
        bridge = _make_bridge_with_pipes(stdout_data)
        with pytest.raises(RuntimeError, match="Designer error"):
            bridge.render()

    def test_pipe_closed_raises(self):
        """Test that EOF on pipe raises ConnectionError."""
        bridge = _make_bridge_with_pipes(b"")  # empty pipe
        with pytest.raises(ConnectionError, match="pipe closed"):
            bridge.render()


class TestIsRunning:
    def test_running_when_process_alive(self):
        bridge = _make_bridge_with_pipes()
        assert bridge.is_running is True

    def test_not_running_when_no_process(self):
        bridge = DesignerBridge()
        assert bridge.is_running is False

    def test_not_running_when_process_exited(self):
        bridge = _make_bridge_with_pipes()
        bridge.proc.poll.return_value = 0
        assert bridge.is_running is False


class TestErrorHandling:
    def test_send_cmd_oserror_raises_connection_error(self):
        """Test that OSError on stdin.write is wrapped as ConnectionError."""
        bridge = _make_bridge_with_pipes()
        bridge.proc.stdin.write = MagicMock(side_effect=OSError("Invalid argument"))
        with pytest.raises(ConnectionError, match="pipe closed"):
            bridge.render()

    def test_send_cmd_broken_pipe_raises_connection_error(self):
        """Test that BrokenPipeError on stdin.flush is wrapped as ConnectionError."""
        bridge = _make_bridge_with_pipes()
        bridge.proc.stdin.flush = MagicMock(side_effect=BrokenPipeError())
        with pytest.raises(ConnectionError, match="pipe closed"):
            bridge.render()

    def test_get_stderr_returns_output_after_exit(self):
        """Test get_stderr reads stderr when process has exited."""
        bridge = _make_bridge_with_pipes()
        bridge.proc.poll.return_value = 1  # process exited
        bridge.proc.stderr = FakePipe(b"Assert@ file = test.c, line = 42\n")
        stderr = bridge.get_stderr()
        assert "Assert@ file = test.c" in stderr

    def test_get_stderr_empty_when_running(self):
        """Test get_stderr returns empty string when process is still running."""
        bridge = _make_bridge_with_pipes()
        bridge.proc.poll.return_value = None  # still running
        assert bridge.get_stderr() == ""
