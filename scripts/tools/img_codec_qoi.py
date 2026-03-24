"""
QOI (Quite OK Image) encoder for EGUI image compression.

Reference: https://qoiformat.org/qoi-specification.pdf

Opcodes:
  QOI_OP_RGB   (0xFE): 3 bytes r, g, b
  QOI_OP_RGBA  (0xFF): 4 bytes r, g, b, a
  QOI_OP_INDEX (0b00xxxxxx): 6-bit hash index
  QOI_OP_DIFF  (0b01xxxxxx): small diff dr,dg,db in [-2,1]
  QOI_OP_LUMA  (0b10xxxxxx + 1 byte): luma diff
  QOI_OP_RUN   (0b11xxxxxx): run-length 1..62

Note: Our encoder produces a "headerless" QOI stream (no 14-byte header,
no 8-byte end marker). Image metadata is stored externally in
egui_image_qoi_info_t.
"""


def _qoi_hash(r, g, b, a):
    return (r * 3 + g * 5 + b * 7 + a * 11) & 63


def _rgb565_to_rgb888_components(val):
    r5 = (val >> 11) & 0x1F
    g6 = (val >> 5) & 0x3F
    b5 = val & 0x1F
    return r5, g6, b5


def _rgb565_default_rgb888(r5, g6, b5):
    return (
        (r5 << 3) | (r5 >> 2),
        (g6 << 2) | (g6 >> 4),
        (b5 << 3) | (b5 >> 2),
    )


# Large opaque RGB565 images are decode-hot in HelloPerformance. Bias their
# bucket representatives for faster QOI decode, but keep smaller assets on the
# canonical RGB565->RGB888 expansion to avoid unnecessary size growth.
QOI_RGB565_SPEED_BIASED_PIXEL_THRESHOLD = 50000
# Only enabled for large opaque RGB565 assets; a deeper lookahead produces a
# more decode-friendly opcode stream for HelloPerformance's 240x240 hotspot.
QOI_RGB565_SPEED_BIASED_LOOKAHEAD = 24


def _qoi_opcode_rank(r, g, b, a, prev_r, prev_g, prev_b, prev_a, index):
    if r == prev_r and g == prev_g and b == prev_b and a == prev_a:
        return 0

    idx = _qoi_hash(r, g, b, a)
    if index[idx] == (r, g, b, a):
        return 1

    if a != prev_a:
        return 5

    dr = r - prev_r
    dg = g - prev_g
    db = b - prev_b
    dr_dg = dr - dg
    db_dg = db - dg

    if -2 <= dr <= 1 and -2 <= dg <= 1 and -2 <= db <= 1:
        return 2

    if -32 <= dg <= 31 and -8 <= dr_dg <= 7 and -8 <= db_dg <= 7:
        return 4

    return 3


def _qoi_rgb565_opaque_extra_candidates(default_rgb, low_rgb, high_rgb):
    return (
        default_rgb,
        (low_rgb[0], low_rgb[1], low_rgb[2]),
        (low_rgb[0], low_rgb[1], high_rgb[2]),
        (low_rgb[0], high_rgb[1], low_rgb[2]),
        (low_rgb[0], high_rgb[1], high_rgb[2]),
        (high_rgb[0], low_rgb[1], low_rgb[2]),
        (high_rgb[0], low_rgb[1], high_rgb[2]),
        (high_rgb[0], high_rgb[1], low_rgb[2]),
        (high_rgb[0], high_rgb[1], high_rgb[2]),
    )


def _qoi_predict_future_best_ranks(current_rgb, future_candidate_sets, prev_r, prev_g, prev_b, prev_a, index):
    if not future_candidate_sets:
        return ()

    current_r, current_g, current_b = current_rgb
    current_a = 255
    current_rgba = (current_r, current_g, current_b, current_a)
    current_index = index
    idx = _qoi_hash(current_r, current_g, current_b, current_a)

    if current_rgba != (prev_r, prev_g, prev_b, prev_a) and index[idx] != current_rgba:
        current_index = index.copy()
        current_index[idx] = current_rgba

    future_scores = []
    for candidate_set in future_candidate_sets:
        if not candidate_set:
            future_scores.append(255)
            continue

        best_candidate = None
        best_score = None
        seen = set()
        for candidate in candidate_set:
            if candidate in seen:
                continue
            seen.add(candidate)

            nr, ng, nb = candidate
            score = _qoi_opcode_rank(nr, ng, nb, 255, current_r, current_g, current_b, current_a, current_index)
            if best_score is None or score < best_score:
                best_score = score
                best_candidate = candidate

        future_scores.append(255 if best_score is None else best_score)

        if best_candidate is None:
            continue

        current_r, current_g, current_b = best_candidate
        current_rgba = (current_r, current_g, current_b, current_a)
        idx = _qoi_hash(current_r, current_g, current_b, current_a)
        if current_index[idx] != current_rgba:
            current_index = current_index.copy()
            current_index[idx] = current_rgba

    return tuple(future_scores)


def _qoi_choose_best_candidate(candidates, prev_r, prev_g, prev_b, prev_a, index, future_candidate_sets=None):
    best_rgb = None
    best_score = None
    best_future_scores = None
    seen = set()

    for candidate in candidates:
        if candidate in seen:
            continue
        seen.add(candidate)

        r, g, b = candidate
        score = _qoi_opcode_rank(r, g, b, 255, prev_r, prev_g, prev_b, prev_a, index)
        future_scores = None
        if future_candidate_sets is not None:
            future_scores = _qoi_predict_future_best_ranks(candidate, future_candidate_sets, prev_r, prev_g, prev_b, prev_a, index)

        if best_score is None or score < best_score:
            best_score = score
            best_rgb = candidate
            best_future_scores = future_scores
        elif score == best_score and future_candidate_sets is not None and future_scores < best_future_scores:
            best_rgb = candidate
            best_future_scores = future_scores

    return best_rgb, best_score


def _qoi_choose_rgb565_opaque_representatives(pixel_data: bytes, total_pixels: int) -> bytes:
    result = bytearray()
    index = [(0, 0, 0, 0)] * 64
    prev_r, prev_g, prev_b, prev_a = 0, 0, 0, 255
    run = 0

    for i in range(total_pixels):
        lo = pixel_data[i * 2]
        hi = pixel_data[i * 2 + 1]
        val = (hi << 8) | lo
        r5, g6, b5 = _rgb565_to_rgb888_components(val)

        default_rgb = _rgb565_default_rgb888(r5, g6, b5)
        low_rgb = (r5 << 3, g6 << 2, b5 << 3)
        high_rgb = (low_rgb[0] | 0x07, low_rgb[1] | 0x03, low_rgb[2] | 0x07)
        future_candidate_sets = []
        for lookahead in range(1, QOI_RGB565_SPEED_BIASED_LOOKAHEAD + 1):
            if i + lookahead >= total_pixels:
                break
            next_lo = pixel_data[(i + lookahead) * 2]
            next_hi = pixel_data[(i + lookahead) * 2 + 1]
            next_val = (next_hi << 8) | next_lo
            next_r5, next_g6, next_b5 = _rgb565_to_rgb888_components(next_val)
            next_default_rgb = _rgb565_default_rgb888(next_r5, next_g6, next_b5)
            next_low_rgb = (next_r5 << 3, next_g6 << 2, next_b5 << 3)
            next_high_rgb = (next_low_rgb[0] | 0x07, next_low_rgb[1] | 0x03, next_low_rgb[2] | 0x07)
            future_candidate_sets.append(_qoi_rgb565_opaque_extra_candidates(next_default_rgb, next_low_rgb, next_high_rgb))

        best_rgb, best_score = _qoi_choose_best_candidate(
            (high_rgb, default_rgb, low_rgb), prev_r, prev_g, prev_b, prev_a, index,
            future_candidate_sets=future_candidate_sets or None
        )

        extra_rgb, extra_score = _qoi_choose_best_candidate(
            _qoi_rgb565_opaque_extra_candidates(default_rgb, low_rgb, high_rgb), prev_r, prev_g, prev_b, prev_a, index
        )
        if extra_score < best_score and extra_score != 3:
            best_rgb = extra_rgb

        r, g, b = best_rgb
        result.append(r)
        result.append(g)
        result.append(b)

        if r == prev_r and g == prev_g and b == prev_b and prev_a == 255:
            run += 1
            if run == 62:
                run = 0
            continue

        if run > 0:
            run = 0

        idx = _qoi_hash(r, g, b, 255)
        rgba = (r, g, b, 255)
        if index[idx] != rgba:
            index[idx] = rgba

        prev_r, prev_g, prev_b = r, g, b

    return bytes(result)


def qoi_encode(pixel_data: bytes, width: int, height: int, channels: int = 3) -> bytes:
    """
    QOI encode pixel data (RGB or RGBA, 8-bit per channel).

    Args:
        pixel_data: raw pixel data in RGB or RGBA order, 8-bit per channel
        width: image width
        height: image height
        channels: 3 for RGB, 4 for RGBA

    Returns:
        QOI compressed byte stream (headerless, raw opcodes only)
    """
    total_pixels = width * height
    result = bytearray()

    # Index of previously seen pixels
    index = [(0, 0, 0, 0)] * 64

    prev_r, prev_g, prev_b, prev_a = 0, 0, 0, 255
    run = 0

    for px in range(total_pixels):
        offset = px * channels
        r = pixel_data[offset]
        g = pixel_data[offset + 1]
        b = pixel_data[offset + 2]
        a = pixel_data[offset + 3] if channels == 4 else 255

        if r == prev_r and g == prev_g and b == prev_b and a == prev_a:
            run += 1
            if run == 62 or px == total_pixels - 1:
                result.append(0xC0 | (run - 1))
                run = 0
        else:
            if run > 0:
                result.append(0xC0 | (run - 1))
                run = 0

            idx = _qoi_hash(r, g, b, a)

            if index[idx] == (r, g, b, a):
                # QOI_OP_INDEX
                result.append(idx)  # 0b00xxxxxx
            else:
                index[idx] = (r, g, b, a)

                if a != prev_a:
                    # QOI_OP_RGBA
                    result.append(0xFF)
                    result.append(r)
                    result.append(g)
                    result.append(b)
                    result.append(a)
                else:
                    dr = (r - prev_r) & 0xFF
                    dg = (g - prev_g) & 0xFF
                    db = (b - prev_b) & 0xFF

                    # Convert to signed [-128, 127]
                    if dr >= 128:
                        dr -= 256
                    if dg >= 128:
                        dg -= 256
                    if db >= 128:
                        db -= 256

                    dr_dg = dr - dg
                    db_dg = db - dg

                    if -2 <= dr <= 1 and -2 <= dg <= 1 and -2 <= db <= 1:
                        # QOI_OP_DIFF
                        result.append(0x40 | ((dr + 2) << 4) | ((dg + 2) << 2) | (db + 2))
                    elif -32 <= dg <= 31 and -8 <= dr_dg <= 7 and -8 <= db_dg <= 7:
                        # QOI_OP_LUMA
                        result.append(0x80 | (dg + 32))
                        result.append(((dr_dg + 8) << 4) | (db_dg + 8))
                    else:
                        # QOI_OP_RGB
                        result.append(0xFE)
                        result.append(r)
                        result.append(g)
                        result.append(b)

            prev_r, prev_g, prev_b, prev_a = r, g, b, a

    # Flush remaining run
    if run > 0:
        result.append(0xC0 | (run - 1))

    return bytes(result)


def qoi_decode(data: bytes, width: int, height: int, channels: int = 3) -> bytes:
    """
    QOI decode a byte stream (headerless, for verification).

    Returns:
        Decompressed pixel data in RGB or RGBA order
    """
    total_pixels = width * height
    result = bytearray()

    index = [(0, 0, 0, 0)] * 64
    prev_r, prev_g, prev_b, prev_a = 0, 0, 0, 255
    run = 0
    pos = 0

    for _ in range(total_pixels):
        if run > 0:
            run -= 1
        else:
            if pos >= len(data):
                break
            b1 = data[pos]
            pos += 1

            if b1 == 0xFE:  # QOI_OP_RGB
                prev_r = data[pos]; pos += 1
                prev_g = data[pos]; pos += 1
                prev_b = data[pos]; pos += 1
            elif b1 == 0xFF:  # QOI_OP_RGBA
                prev_r = data[pos]; pos += 1
                prev_g = data[pos]; pos += 1
                prev_b = data[pos]; pos += 1
                prev_a = data[pos]; pos += 1
            elif (b1 & 0xC0) == 0x00:  # QOI_OP_INDEX
                idx = b1 & 0x3F
                prev_r, prev_g, prev_b, prev_a = index[idx]
            elif (b1 & 0xC0) == 0x40:  # QOI_OP_DIFF
                prev_r = (prev_r + ((b1 >> 4) & 0x03) - 2) & 0xFF
                prev_g = (prev_g + ((b1 >> 2) & 0x03) - 2) & 0xFF
                prev_b = (prev_b + (b1 & 0x03) - 2) & 0xFF
            elif (b1 & 0xC0) == 0x80:  # QOI_OP_LUMA
                b2 = data[pos]; pos += 1
                vg = (b1 & 0x3F) - 32
                prev_r = (prev_r + vg - 8 + ((b2 >> 4) & 0x0F)) & 0xFF
                prev_g = (prev_g + vg) & 0xFF
                prev_b = (prev_b + vg - 8 + (b2 & 0x0F)) & 0xFF
            elif (b1 & 0xC0) == 0xC0:  # QOI_OP_RUN
                run = (b1 & 0x3F)  # remaining pixels after this one

            idx = _qoi_hash(prev_r, prev_g, prev_b, prev_a)
            index[idx] = (prev_r, prev_g, prev_b, prev_a)

        result.append(prev_r)
        result.append(prev_g)
        result.append(prev_b)
        if channels == 4:
            result.append(prev_a)

    return bytes(result)


def qoi_encode_image(pixel_data: bytes, alpha_data: bytes,
                     width: int, height: int,
                     data_type: str, alpha_type: int):
    """
    QOI encode image for EGUI.
    
    QOI works natively in RGB888/RGBA8888 space. For RGB565 input,
    convert to RGB888 before encoding (C decoder does the reverse).

    Args:
        pixel_data: raw pixel bytes in target format
        alpha_data: raw alpha bytes (empty if no alpha)
        width: image width
        height: image height
        data_type: "rgb565" or "rgb32"
        alpha_type: 0 or 8

    Returns:
        (compressed_data, channels) where channels = 3 or 4
    """
    total_pixels = width * height
    has_alpha = (alpha_data and len(alpha_data) > 0 and alpha_type == 8)
    channels = 4 if has_alpha else 3

    # Convert pixel data to RGB888/RGBA8888 for QOI encoding
    rgb_data = bytearray()

    if data_type == "rgb565":
        if has_alpha or total_pixels < QOI_RGB565_SPEED_BIASED_PIXEL_THRESHOLD:
            for i in range(total_pixels):
                # Little-endian uint16
                lo = pixel_data[i * 2]
                hi = pixel_data[i * 2 + 1]
                val = (hi << 8) | lo
                r5, g6, b5 = _rgb565_to_rgb888_components(val)
                r8, g8, b8 = _rgb565_default_rgb888(r5, g6, b5)
                rgb_data.append(r8)
                rgb_data.append(g8)
                rgb_data.append(b8)
                if has_alpha:
                    rgb_data.append(alpha_data[i])
        else:
            rgb_data.extend(_qoi_choose_rgb565_opaque_representatives(pixel_data, total_pixels))
    elif data_type == "rgb32":
        for i in range(total_pixels):
            # RGB32: stored as R,G,B,A (4 bytes per pixel)
            r = pixel_data[i * 4 + 0]
            g = pixel_data[i * 4 + 1]
            b = pixel_data[i * 4 + 2]
            a = pixel_data[i * 4 + 3] if has_alpha else 255
            rgb_data.append(r)
            rgb_data.append(g)
            rgb_data.append(b)
            if has_alpha:
                rgb_data.append(a)
    else:
        raise ValueError(f"QOI does not support data_type={data_type}. Use RLE for gray8.")

    compressed = qoi_encode(bytes(rgb_data), width, height, channels)
    return compressed, channels


# Self-test
if __name__ == "__main__":
    print("QOI Encoder Self-Test")
    print("=" * 40)

    # Test 1: Simple solid color image (should compress well)
    w, h = 16, 16
    solid_rgb = bytes([128, 64, 32] * (w * h))
    enc1 = qoi_encode(solid_rgb, w, h, 3)
    dec1 = qoi_decode(enc1, w, h, 3)
    assert dec1 == solid_rgb, "Test 1 failed: decode mismatch"
    print(f"Test 1 (solid RGB): {len(solid_rgb)}B -> {len(enc1)}B")

    # Test 2: Gradient image
    gradient = bytearray()
    for y in range(16):
        for x in range(16):
            gradient.extend([x * 16, y * 16, 128])
    enc2 = qoi_encode(bytes(gradient), 16, 16, 3)
    dec2 = qoi_decode(enc2, 16, 16, 3)
    assert dec2 == bytes(gradient), "Test 2 failed: decode mismatch"
    print(f"Test 2 (gradient): {len(gradient)}B -> {len(enc2)}B")

    # Test 3: RGBA with alpha
    rgba = bytearray()
    for i in range(64):
        rgba.extend([255, 0, 0, i * 4])
    enc3 = qoi_encode(bytes(rgba), 8, 8, 4)
    dec3 = qoi_decode(enc3, 8, 8, 4)
    assert dec3 == bytes(rgba), "Test 3 failed: decode mismatch"
    print(f"Test 3 (RGBA): {len(rgba)}B -> {len(enc3)}B")

    # Test 4: RGB565 image encoding path
    rgb565_data = bytearray()
    alpha_data = bytearray()
    for i in range(64):
        rgb565_data.extend([0x00, 0xF8])  # Pure red in RGB565 (0xF800)
        alpha_data.append(255)
    comp, ch = qoi_encode_image(bytes(rgb565_data), bytes(alpha_data), 8, 8, "rgb565", 8)
    print(f"Test 4 (rgb565): {len(rgb565_data)}B -> {len(comp)}B, channels={ch}")

    # Test 5: Empty edge case
    enc5 = qoi_encode(b"", 0, 0, 3)
    assert enc5 == b"", "Test 5 failed"
    print("Test 5 (empty): OK")

    print("=" * 40)
    print("All tests passed!")
