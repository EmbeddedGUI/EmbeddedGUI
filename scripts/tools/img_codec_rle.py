"""
RLE (Run-Length Encoding) encoder for EGUI image compression.

Control byte format (compatible with LVGL RLE):
  - bit7=1: literal mode, count = ctrl & 0x7F, followed by count * blk_size raw bytes
  - bit7=0: repeat mode, count = ctrl, followed by blk_size bytes repeated count times

Max run/literal per control byte: 127 (0x7F)
"""


def rle_encode(data: bytes, blk_size: int) -> bytes:
    """
    RLE encode a byte stream.

    Args:
        data: raw byte data (length must be multiple of blk_size)
        blk_size: block size in bytes (1 for alpha/gray8, 2 for rgb565, 4 for rgb32)

    Returns:
        RLE compressed bytes
    """
    if len(data) == 0:
        return b""

    num_blocks = len(data) // blk_size
    if num_blocks == 0:
        return b""

    result = bytearray()
    i = 0

    while i < num_blocks:
        # Look ahead to determine if we should do a repeat or literal run
        block = data[i * blk_size : (i + 1) * blk_size]

        # Count consecutive identical blocks
        run_count = 1
        while (i + run_count < num_blocks and run_count < 127 and
               data[(i + run_count) * blk_size : (i + run_count + 1) * blk_size] == block):
            run_count += 1

        if run_count >= 2:
            # Repeat mode: emit control byte (bit7=0) + one block
            result.append(run_count & 0x7F)  # bit7=0
            result.extend(block)
            i += run_count
        else:
            # Literal mode: collect non-repeating blocks
            lit_start = i
            lit_count = 1
            i += 1

            while i < num_blocks and lit_count < 127:
                next_block = data[i * blk_size : (i + 1) * blk_size]
                # Check if we're about to hit a repeat run of 2+
                if (i + 1 < num_blocks and
                        data[(i + 1) * blk_size : (i + 2) * blk_size] == next_block):
                    break  # Stop literal, let next iteration handle repeat
                lit_count += 1
                i += 1

            # Literal mode: emit control byte (bit7=1) + raw blocks
            result.append(0x80 | lit_count)
            result.extend(data[lit_start * blk_size : (lit_start + lit_count) * blk_size])

    return bytes(result)


def rle_decode(data: bytes, blk_size: int, output_size: int) -> bytes:
    """
    RLE decode a byte stream (for verification).

    Args:
        data: RLE compressed bytes
        blk_size: block size
        output_size: expected decompressed size in bytes

    Returns:
        Decompressed bytes
    """
    result = bytearray()
    rd = 0

    while rd < len(data) and len(result) < output_size:
        ctrl = data[rd]
        rd += 1

        if ctrl & 0x80:
            # Literal
            count = ctrl & 0x7F
            nbytes = count * blk_size
            result.extend(data[rd : rd + nbytes])
            rd += nbytes
        else:
            # Repeat
            count = ctrl
            block = data[rd : rd + blk_size]
            rd += blk_size
            for _ in range(count):
                result.extend(block)

    return bytes(result[:output_size])


def rle_encode_image(pixel_data: bytes, alpha_data: bytes,
                     width: int, height: int,
                     data_type: str, alpha_type: int):
    """
    RLE encode image pixel and alpha data.

    Args:
        pixel_data: raw pixel bytes
        alpha_data: raw alpha bytes (empty bytes if no alpha)
        width: image width
        height: image height
        data_type: "rgb565", "rgb32", or "gray8"
        alpha_type: alpha bits (0=no alpha, 1, 2, 4, 8)

    Returns:
        (compressed_pixels, compressed_alpha) tuple of bytes
    """
    # Determine block size for pixel data
    if data_type == "rgb32":
        pixel_blk = 4
    elif data_type == "rgb565":
        pixel_blk = 2
    elif data_type == "gray8":
        pixel_blk = 1
    else:
        pixel_blk = 2

    # Encode row-by-row so RLE runs never cross row boundaries.
    # The C decoder decompresses exactly `width` pixels per row call;
    # cross-row runs would cause stream desync.
    pixel_row_bytes = width * pixel_blk
    compressed_pixels = b""
    for y in range(height):
        row_data = pixel_data[y * pixel_row_bytes : (y + 1) * pixel_row_bytes]
        compressed_pixels += rle_encode(row_data, pixel_blk)

    # Compress alpha row-by-row if present
    compressed_alpha = b""
    if alpha_data and len(alpha_data) > 0 and alpha_type > 0:
        # Determine alpha row size in bytes (packed for sub-8 types)
        if alpha_type == 8:
            alpha_row_bytes = width
        elif alpha_type == 4:
            alpha_row_bytes = (width + 1) >> 1
        elif alpha_type == 2:
            alpha_row_bytes = (width + 3) >> 2
        elif alpha_type == 1:
            alpha_row_bytes = (width + 7) >> 3
        else:
            alpha_row_bytes = width

        for y in range(height):
            row_alpha = alpha_data[y * alpha_row_bytes : (y + 1) * alpha_row_bytes]
            compressed_alpha += rle_encode(row_alpha, 1)

    return compressed_pixels, compressed_alpha


# Self-test
if __name__ == "__main__":
    import os
    import sys

    print("RLE Encoder Self-Test")
    print("=" * 40)

    # Test 1: Simple repeat pattern
    test_data = b"\x01\x02" * 10 + b"\x03\x04" * 5
    encoded = rle_encode(test_data, 2)
    decoded = rle_decode(encoded, 2, len(test_data))
    assert decoded == test_data, f"Test 1 failed: decoded != original"
    ratio = len(encoded) / len(test_data) * 100
    print(f"Test 1 (repeat): {len(test_data)}B -> {len(encoded)}B ({ratio:.1f}%)")

    # Test 2: All different (literal)
    test_data2 = bytes(range(128))
    encoded2 = rle_encode(test_data2, 1)
    decoded2 = rle_decode(encoded2, 1, len(test_data2))
    assert decoded2 == test_data2, f"Test 2 failed: decoded != original"
    print(f"Test 2 (literal): {len(test_data2)}B -> {len(encoded2)}B")

    # Test 3: Mixed pattern
    test_data3 = b"\xAA" * 50 + bytes(range(20)) + b"\xBB" * 30
    encoded3 = rle_encode(test_data3, 1)
    decoded3 = rle_decode(encoded3, 1, len(test_data3))
    assert decoded3 == test_data3, f"Test 3 failed: decoded != original"
    print(f"Test 3 (mixed): {len(test_data3)}B -> {len(encoded3)}B")

    # Test 4: RGB565 image-like data
    test_data4 = b"\x00\x00" * 100 + b"\xFF\xFF" * 50 + bytes(range(60))
    encoded4 = rle_encode(test_data4, 2)
    decoded4 = rle_decode(encoded4, 2, len(test_data4))
    assert decoded4 == test_data4, f"Test 4 failed: decoded != original"
    print(f"Test 4 (rgb565): {len(test_data4)}B -> {len(encoded4)}B")

    # Test 5: Empty data
    encoded5 = rle_encode(b"", 1)
    assert encoded5 == b"", "Test 5 failed: empty data"
    print("Test 5 (empty): OK")

    print("=" * 40)
    print("All tests passed!")
