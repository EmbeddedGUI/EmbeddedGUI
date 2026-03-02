"""Bridge for communicating with the headless designer exe via pipes."""

import struct
import subprocess
import sys

# Command codes (Python -> exe, via stdin)
CMD_RENDER = 0x01
CMD_TOUCH = 0x02
CMD_KEY = 0x03
CMD_QUIT = 0xFF

# Response codes (exe -> Python, via stdout)
RSP_READY = 0x01
RSP_FRAME = 0x02
RSP_ERROR = 0xFF

# Touch actions
TOUCH_DOWN = 0x01
TOUCH_UP = 0x02
TOUCH_MOVE = 0x03


class DesignerBridge:
    """Manages the headless designer exe subprocess and pipe communication."""

    def __init__(self):
        self.proc = None
        self.width = 0
        self.height = 0

    def start(self, exe_path, resource_path, width, height):
        """Start the designer exe and wait for READY signal."""
        self.width = width
        self.height = height
        self.proc = subprocess.Popen(
            [exe_path, resource_path],
            stdin=subprocess.PIPE,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
        )
        # Wait for RSP_READY
        rsp = self._read_exact(1)
        if rsp[0] != RSP_READY:
            raise RuntimeError(f"Expected READY, got 0x{rsp[0]:02x}")

    def render(self):
        """Request a frame render and return RGB888 bytes."""
        self._send_cmd(CMD_RENDER)
        return self._read_frame()

    def inject_touch(self, action, x, y):
        """Send a touch event to the exe."""
        payload = struct.pack("<BHH", action, x, y)
        self._send_cmd(CMD_TOUCH, payload)

    def inject_key(self, key_type, code):
        """Send a key event to the exe."""
        payload = struct.pack("<BH", key_type, code)
        self._send_cmd(CMD_KEY, payload)

    def stop(self):
        """Stop the exe process."""
        if self.proc and self.proc.poll() is None:
            try:
                self._send_cmd(CMD_QUIT)
                self.proc.wait(timeout=3)
            except Exception:
                try:
                    self.proc.kill()
                except OSError:
                    pass
        self.proc = None

    def get_stderr(self):
        """Read any available stderr output from the exe (non-blocking)."""
        if self.proc and self.proc.stderr:
            try:
                # Only read if process has exited (otherwise would block)
                if self.proc.poll() is not None:
                    return self.proc.stderr.read().decode("utf-8", errors="replace")
            except Exception:
                pass
        return ""

    @property
    def is_running(self):
        return self.proc is not None and self.proc.poll() is None

    # --- Internal ---

    def _send_cmd(self, cmd, payload=b""):
        try:
            self.proc.stdin.write(bytes([cmd]) + payload)
            self.proc.stdin.flush()
        except (OSError, BrokenPipeError):
            raise ConnectionError("Designer exe pipe closed")

    def _read_exact(self, n):
        data = b""
        while len(data) < n:
            chunk = self.proc.stdout.read(n - len(data))
            if not chunk:
                raise ConnectionError("Designer exe pipe closed")
            data += chunk
        return data

    def _read_frame(self):
        rsp = self._read_exact(1)
        if rsp[0] == RSP_ERROR:
            err_len = struct.unpack("<H", self._read_exact(2))[0]
            err_msg = self._read_exact(err_len).decode("utf-8", errors="replace")
            raise RuntimeError(f"Designer error: {err_msg}")
        if rsp[0] != RSP_FRAME:
            raise RuntimeError(f"Expected FRAME, got 0x{rsp[0]:02x}")
        size = struct.unpack("<I", self._read_exact(4))[0]
        return self._read_exact(size)
