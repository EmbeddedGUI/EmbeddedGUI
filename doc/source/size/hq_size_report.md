# HQ Path QEMU Size Report

- Commit: `09190fa`
- Date: 2026-04-02T11:15:06.888554
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
| Baseline | 21988 | 7100 | 84 | 6148 | 29088 |

## Increment Summary

| Path | Delta Text | Delta Rodata | Delta Data | Delta Bss | Delta ROM |
|------|-----------:|-------------:|-----------:|----------:|----------:|
| LINE_HQ | +15128 | +72 | +0 | +8 | +15200 |
| CIRCLE_HQ | +15128 | +72 | +0 | +8 | +15200 |
| ARC_HQ | +15128 | +72 | +0 | +8 | +15200 |
| ARC_ROUND_CAP_HQ | +12580 | +808 | +0 | +8 | +13388 |
| ALL_HQ | +12580 | +808 | +0 | +8 | +13388 |

## Detailed Variants

| Variant | Probe Config | Text | Rodata | Data | Bss | Total ROM |
|---------|-------------------|-----:|-------:|-----:|----:|----------:|
| Baseline | `(none)` | 21988 | 7100 | 84 | 6148 | 29088 |
| LINE_HQ | `-DEGUI_SIZE_PROBE_LINK_LINE_HQ=1` | 37116 | 7172 | 84 | 6156 | 44288 |
| CIRCLE_HQ | `-DEGUI_SIZE_PROBE_LINK_CIRCLE_HQ=1` | 37116 | 7172 | 84 | 6156 | 44288 |
| ARC_HQ | `-DEGUI_SIZE_PROBE_LINK_ARC_HQ=1` | 37116 | 7172 | 84 | 6156 | 44288 |
| ARC_ROUND_CAP_HQ | `-DEGUI_SIZE_PROBE_LINK_ARC_ROUND_CAP_HQ=1` | 34568 | 7908 | 84 | 6156 | 42476 |
| ALL_HQ | `-DEGUI_SIZE_PROBE_LINK_LINE_HQ=1 -DEGUI_SIZE_PROBE_LINK_CIRCLE_HQ=1 -DEGUI_SIZE_PROBE_LINK_ARC_HQ=1` | 34568 | 7908 | 84 | 6156 | 42476 |

## Linked Symbol Breakdown

### LINE_HQ

- Description: Force link the line/polyline HQ path only.
- Symbol text total: 2700 bytes

| Symbol | Size (bytes) | Type |
|--------|-------------:|------|
| `egui_canvas_draw_line_hq` | 1372 | `T` |
| `egui_canvas_draw_line_round_cap_hq` | 1284 | `T` |
| `egui_canvas_draw_polyline_hq` | 22 | `T` |
| `egui_canvas_draw_polyline_round_cap_hq` | 22 | `T` |

### CIRCLE_HQ

- Description: Force link the circle HQ path only.
- Symbol text total: 3404 bytes

| Symbol | Size (bytes) | Type |
|--------|-------------:|------|
| `egui_canvas_draw_circle_fill_hq` | 3404 | `T` |

### ARC_ROUND_CAP_HQ

- Description: Force link the round-cap arc helper used by widgets like activity_ring.
- Symbol text total: 4256 bytes

| Symbol | Size (bytes) | Type |
|--------|-------------:|------|
| `egui_canvas_draw_circle_fill_hq` | 3404 | `T` |
| `egui_canvas_draw_arc_hq` | 480 | `T` |
| `egui_canvas_draw_arc_round_cap_hq` | 372 | `T` |

### ALL_HQ

- Description: Force link the three core HQ paths together.
- Symbol text total: 4480 bytes

| Symbol | Size (bytes) | Type |
|--------|-------------:|------|
| `egui_canvas_draw_circle_fill_hq` | 3404 | `T` |
| `egui_canvas_draw_arc_fill_hq` | 596 | `T` |
| `egui_canvas_draw_arc_hq` | 480 | `T` |

## Reproduce

```bash
python scripts/size_analysis/hq_size_to_doc.py
```

The raw JSON is written to `output/hq_size_results.json`.
