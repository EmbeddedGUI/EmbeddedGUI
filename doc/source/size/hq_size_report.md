# HQ Path QEMU Size Report

- Commit: `1fe189eb`
- Date: 2026-05-21T16:01:31.525244
- Build target: `APP=HelloSizeAnalysis APP_SUB=hq_path_probe PORT=qemu CPU_ARCH=cortex-m0plus`
- Measurement method: build `HelloSizeAnalysis/hq_path_probe` baseline, then rewrite the app-local probe config header to force-link a single HQ path into the final ELF.
- Report scope: isolated static link delta only. This report does not include runtime heap/stack because the HQ geometry paths themselves do not allocate heap.

## How To Read

- `Delta Text`: `.text` increment relative to baseline, used as the main code-size number.
- `Delta Rodata`: extra lookup tables / constants brought in by the HQ path.
- `Delta ROM`: `.text + .rodata` total flash increment relative to baseline.
- `ARC_HQ` here measures `egui_canvas_draw_arc_hq()` + `egui_canvas_draw_arc_fill_hq()` only.
- `ARC_ROUND_CAP_HQ` is listed separately because it is a real widget-facing helper and it pulls both arc and circle-fill dependencies.
- `Delta ROM` is the real ELF section delta. It is often larger than the listed public symbol sizes because static helpers and dependent sections are linked too.
- The `+8 BSS` seen in probe variants comes from the probe harness guard in `HelloSizeAnalysis/hq_path_probe`, not from persistent HQ runtime state.
- Turning a path off can only reclaim the same bytes when no other object still references that path.

## Baseline

| Variant | Text | Rodata | Data | Bss | Total ROM |
|---------|------|--------|------|-----|-----------|
| Baseline | 27032 | 7504 | 1392 | 6612 | 34536 |

## Increment Summary

| Path | Delta Text | Delta Rodata | Delta Data | Delta Bss | Delta ROM |
|------|-----------:|-------------:|-----------:|----------:|----------:|
| LINE_HQ | +12396 | +68 | +0 | +4 | +12464 |
| CIRCLE_HQ | +2828 | +36 | +0 | +4 | +2864 |
| ARC_HQ | +8372 | +764 | +0 | +4 | +9136 |
| ARC_ROUND_CAP_HQ | +10640 | +804 | +0 | +4 | +11444 |
| ALL_HQ | +20988 | +796 | +0 | +4 | +21784 |

## Detailed Variants

| Variant | Probe Config | Text | Rodata | Data | Bss | Total ROM |
|---------|-------------------|-----:|-------:|-----:|----:|----------:|
| Baseline | `(none)` | 27032 | 7504 | 1392 | 6612 | 34536 |
| LINE_HQ | `-DEGUI_SIZE_PROBE_LINK_LINE_HQ=1` | 39428 | 7572 | 1392 | 6616 | 47000 |
| CIRCLE_HQ | `-DEGUI_SIZE_PROBE_LINK_CIRCLE_HQ=1` | 29860 | 7540 | 1392 | 6616 | 37400 |
| ARC_HQ | `-DEGUI_SIZE_PROBE_LINK_ARC_HQ=1` | 35404 | 8268 | 1392 | 6616 | 43672 |
| ARC_ROUND_CAP_HQ | `-DEGUI_SIZE_PROBE_LINK_ARC_ROUND_CAP_HQ=1` | 37672 | 8308 | 1392 | 6616 | 45980 |
| ALL_HQ | `-DEGUI_SIZE_PROBE_LINK_LINE_HQ=1 -DEGUI_SIZE_PROBE_LINK_CIRCLE_HQ=1 -DEGUI_SIZE_PROBE_LINK_ARC_HQ=1` | 48020 | 8300 | 1392 | 6616 | 56320 |

## Linked Symbol Breakdown

### LINE_HQ

- Description: Force link the line/polyline HQ path only.
- Symbol text total: 3636 bytes

| Symbol | Size (bytes) | Type |
|--------|-------------:|------|
| `egui_canvas_draw_line_hq` | 2024 | `T` |
| `egui_canvas_draw_line_round_cap_hq` | 1552 | `T` |
| `egui_canvas_draw_polyline_hq` | 30 | `T` |
| `egui_canvas_draw_polyline_round_cap_hq` | 30 | `T` |

### CIRCLE_HQ

- Description: Force link the circle HQ path only.
- Symbol text total: 2708 bytes

| Symbol | Size (bytes) | Type |
|--------|-------------:|------|
| `egui_canvas_draw_circle_fill_hq` | 1744 | `T` |
| `egui_canvas_draw_circle_hq` | 964 | `T` |

### ARC_HQ

- Description: Force link the arc HQ path only. Round-cap arc is excluded to avoid double counting line/circle dependencies.
- Symbol text total: 1132 bytes

| Symbol | Size (bytes) | Type |
|--------|-------------:|------|
| `egui_canvas_draw_arc_fill_hq` | 624 | `T` |
| `egui_canvas_draw_arc_hq` | 508 | `T` |

### ARC_ROUND_CAP_HQ

- Description: Force link the round-cap arc helper used by widgets like activity_ring.
- Symbol text total: 2668 bytes

| Symbol | Size (bytes) | Type |
|--------|-------------:|------|
| `egui_canvas_draw_circle_fill_hq` | 1744 | `T` |
| `egui_canvas_draw_arc_hq` | 508 | `T` |
| `egui_canvas_draw_arc_round_cap_hq` | 416 | `T` |

### ALL_HQ

- Description: Force link the three core HQ paths together.
- Symbol text total: 7476 bytes

| Symbol | Size (bytes) | Type |
|--------|-------------:|------|
| `egui_canvas_draw_line_hq` | 2024 | `T` |
| `egui_canvas_draw_circle_fill_hq` | 1744 | `T` |
| `egui_canvas_draw_line_round_cap_hq` | 1552 | `T` |
| `egui_canvas_draw_circle_hq` | 964 | `T` |
| `egui_canvas_draw_arc_fill_hq` | 624 | `T` |
| `egui_canvas_draw_arc_hq` | 508 | `T` |
| `egui_canvas_draw_polyline_hq` | 30 | `T` |
| `egui_canvas_draw_polyline_round_cap_hq` | 30 | `T` |

## Reproduce

```bash
python scripts/size_analysis/main.py hq-size-to-doc
```

The raw JSON is written to `output/hq_size_results.json`.
