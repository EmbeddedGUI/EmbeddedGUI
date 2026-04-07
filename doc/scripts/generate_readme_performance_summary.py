import json
import textwrap
from pathlib import Path

from PIL import Image, ImageDraw, ImageFont, ImageOps


REPO_ROOT = Path(__file__).resolve().parents[2]
DOC_DIR = Path(__file__).resolve().parents[1]
PERF_INDEX_PATH = REPO_ROOT / "perf_output" / "perf_scenes_index.json"
OUTPUT_PATH = DOC_DIR / "source" / "images" / "readme_performance_summary.png"

BACKGROUND_COLOR = "#f4f6f8"
CARD_COLOR = "#ffffff"
CARD_BORDER_COLOR = "#d7dde2"
TITLE_COLOR = "#15202b"
SUBTITLE_COLOR = "#5b6670"
TIME_COLOR = "#0f766e"
LABEL_COLOR = "#1f2933"

OUTER_MARGIN = 24
COLUMN_GAP = 16
ROW_GAP = 16
CARD_PADDING = 8
TITLE_HEIGHT = 78
LABEL_SPACING = 4
LABEL_HEIGHT = 58
TILE_WIDTH = 180

SELECTED_SCENES = [
    "TEXT_RECT",
    "IMAGE_565",
    "GRADIENT_CIRCLE",
    "SHADOW_ROUND",
]


def load_font(size, bold=False):
    candidates = []
    if bold:
        candidates.extend(
            [
                "C:/Windows/Fonts/segoeuib.ttf",
                "C:/Windows/Fonts/arialbd.ttf",
                "C:/Windows/Fonts/calibrib.ttf",
                "DejaVuSans-Bold.ttf",
            ]
        )
    else:
        candidates.extend(
            [
                "C:/Windows/Fonts/segoeui.ttf",
                "C:/Windows/Fonts/arial.ttf",
                "C:/Windows/Fonts/calibri.ttf",
                "DejaVuSans.ttf",
            ]
        )

    for candidate in candidates:
        try:
            return ImageFont.truetype(candidate, size)
        except OSError:
            continue
    return ImageFont.load_default()


def rounded_rectangle(draw, box, radius, fill, outline=None, width=1):
    draw.rounded_rectangle(box, radius=radius, fill=fill, outline=outline, width=width)


def measure_multiline_height(draw, lines, font, spacing):
    if not lines:
        return 0
    bbox = draw.multiline_textbbox((0, 0), "\n".join(lines), font=font, spacing=spacing)
    return bbox[3] - bbox[1]


def wrap_case_name(name, width):
    words = name.split("_")
    lines = []
    current = ""
    max_chars = max(10, width // 9)

    for word in words:
        token = word if not current else "_" + word
        if len(current) + len(token) <= max_chars:
            current += token
            continue
        if current:
            lines.append(current)
        current = word

    if current:
        lines.append(current)

    wrapped = []
    for line in lines:
        wrapped.extend(textwrap.wrap(line, width=max_chars, break_long_words=True, break_on_hyphens=False) or [""])
    return wrapped[:3]


def load_scene_records():
    if not PERF_INDEX_PATH.exists():
        raise FileNotFoundError(
            "missing perf scene index: {}\n"
            "run `python scripts/perf_analysis/main.py scene-capture` first".format(PERF_INDEX_PATH)
        )

    index = json.loads(PERF_INDEX_PATH.read_text(encoding="utf-8"))
    record_map = {}
    for record in index.get("scenes", []):
        record_map[record["name"]] = record

    records = []
    for scene_name in SELECTED_SCENES:
        if scene_name not in record_map:
            raise KeyError("scene {} not found in {}".format(scene_name, PERF_INDEX_PATH))
        record = record_map[scene_name]
        frame_path = REPO_ROOT / record["frame"]
        if not frame_path.exists():
            raise FileNotFoundError("missing scene frame: {}".format(frame_path))
        records.append(
            {
                "name": scene_name,
                "time_ms": record["time_ms"],
                "frame_path": frame_path,
            }
        )
    return records


def main():
    records = load_scene_records()

    OUTPUT_PATH.parent.mkdir(parents=True, exist_ok=True)

    with Image.open(records[0]["frame_path"]) as sample:
        tile_height = max(1, int(round(TILE_WIDTH * sample.height / sample.width)))

    card_width = TILE_WIDTH + CARD_PADDING * 2
    card_height = tile_height + LABEL_HEIGHT + CARD_PADDING * 2
    canvas_width = OUTER_MARGIN * 2 + len(records) * card_width + (len(records) - 1) * COLUMN_GAP
    canvas_height = OUTER_MARGIN * 2 + TITLE_HEIGHT + card_height

    canvas = Image.new("RGB", (canvas_width, canvas_height), BACKGROUND_COLOR)
    draw = ImageDraw.Draw(canvas)

    title_font = load_font(30, bold=True)
    subtitle_font = load_font(16)
    label_font = load_font(14, bold=True)
    time_font = load_font(14)

    draw.text((OUTER_MARGIN, OUTER_MARGIN), "HelloPerformance Representative Scenes", font=title_font, fill=TITLE_COLOR)
    draw.text(
        (OUTER_MARGIN, OUTER_MARGIN + 40),
        "Subset captured from perf_scenes.png using the same contact-sheet style",
        font=subtitle_font,
        fill=SUBTITLE_COLOR,
    )

    y0 = OUTER_MARGIN + TITLE_HEIGHT
    for index, record in enumerate(records):
        x0 = OUTER_MARGIN + index * (card_width + COLUMN_GAP)
        x1 = x0 + card_width
        y1 = y0 + card_height

        rounded_rectangle(draw, (x0, y0, x1, y1), radius=12, fill=CARD_COLOR, outline=CARD_BORDER_COLOR, width=1)

        with Image.open(record["frame_path"]) as frame:
            preview = ImageOps.contain(frame.convert("RGB"), (TILE_WIDTH, tile_height), Image.Resampling.LANCZOS)
        preview_x = x0 + CARD_PADDING + (TILE_WIDTH - preview.width) // 2
        preview_y = y0 + CARD_PADDING + (tile_height - preview.height) // 2
        canvas.paste(preview, (preview_x, preview_y))

        label_x = x0 + CARD_PADDING + 6
        label_y = y0 + CARD_PADDING + tile_height + 6
        name_lines = wrap_case_name(record["name"], TILE_WIDTH - 12)
        draw.multiline_text((label_x, label_y), "\n".join(name_lines), fill=LABEL_COLOR, font=label_font, spacing=LABEL_SPACING)
        name_height = measure_multiline_height(draw, name_lines, label_font, LABEL_SPACING)
        draw.text((label_x, label_y + name_height + 6), "{:.3f} ms".format(record["time_ms"]), fill=TIME_COLOR, font=time_font)

    canvas.save(OUTPUT_PATH, optimize=True)
    print("generated:", OUTPUT_PATH)
    for record in records:
        print("{} {:.3f} ms {}".format(record["name"], record["time_ms"], record["frame_path"].relative_to(REPO_ROOT)))


if __name__ == "__main__":
    main()
