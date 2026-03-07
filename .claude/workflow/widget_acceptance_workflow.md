# Widget Acceptance Workflow

Manual trigger workflow for advancing basic widget quality without repeated manual confirmation.

Phase-1 portfolio mode for the 1000-widget rollout:

- novelty-first: prioritize visually or interaction-wise distinctive widgets before incremental variants
- one widget at a time: do not start a second widget while another widget is still active
- recursive iteration floor: every widget must complete 30 quality cycles before closure

## Step -1: Portfolio tracker sync (required)

Before choosing a new widget, read and update `.claude/workflow/widget_progress_tracker.md`.

Rules:

- if `当前进行中` is not empty, continue that widget first; do not open a different widget in parallel
- only after the current widget is accepted or explicitly marked as shelved may a new widget be selected
- when a new widget is selected, register it in `当前进行中` immediately with date/category/novelty goal


## Step 0: Novelty-first gap discovery + design brief

```bash
# Generate gap report and recommend next widget
python scripts/widget/widget_design_bootstrap.py

# Generate design brief for a specific missing widget
python scripts/widget/widget_design_bootstrap.py --widget segmented_control --overwrite
```

The script includes duplication analysis by default:

- computes a duplication risk score (`low`/`medium`/`high`)
- lists overlapping existing widgets
- blocks high-duplication candidates by default
- override only when needed: `--allow-high-duplication`

Phase-1 candidate selection rule:

- sort by `innovation_priority` first, then duplication risk, then normal priority
- prefer widgets with new interaction models, fresh visual language, or new composition patterns
- defer minor variants and thin wrappers unless they unlock a clearly different scenario
- cross-check the recommendation against `.claude/workflow/widget_progress_tracker.md` before locking the target


Artifacts:

- `runtime_check_output/widget_gap_report.json`
- `runtime_check_output/widget_candidate_catalog.json`
- `runtime_check_output/HelloBasic_<widget>/widget_designs/<widget>.md`
- `runtime_check_output/HelloBasic_<widget>/widget_design_reviews/<widget>.json`
- `runtime_check_output/HelloBasic_<widget>/widget_design_gate_reports/<widget>.json`
- `runtime_check_output/HelloBasic_<widget>/iteration_records/<widget>.json`
- `.claude/workflow/widget_progress_tracker.md`

## Step 0.5: Design score gate

Fill the scorecard first (0-5 per criterion), then run:

```bash
python scripts/widget/widget_design_review_gate.py --widget segmented_control
```

Default gate:

- weighted overall score >= 80
- all critical criteria >= 3
- in phase-1 novelty mode, `design_novelty` should normally score >= 4 before implementation

If the gate fails, implementation and baseline flow must stop.

## Step 0.8: Duplication decision gate (required)

Before implementation, confirm the candidate is not functionally redundant:

- if duplication risk is `high`, do not continue by default
- continue only when there is explicit business value and clear differentiation notes
- required output must include a "differentiation" section and concrete non-overlap goals

## Step 1: Generate HelloBasic design doc (required)

Before implementation, create a detailed design doc in the widget demo directory:

- target path: `example/HelloBasic/<widget>/readme.md`
- this file is required, and must be updated together with `test.c`
- use `example/HelloBasic/gridlayout/readme.md` style as base, but keep the new doc more detailed
- readme language/encoding requirement: Chinese content, UTF-8 encoding

Required sections in `readme.md`:

1. Why this widget exists (problem/value/motivation).
2. Why existing widgets are not enough (must explicitly explain non-substitutability).
3. App overview and usage scenario.
4. Visual/layout spec (size, spacing, alignment, color intent).
5. Widget list with variable name, type, size, initial state, and purpose.
6. State coverage matrix (normal/selected/pressed/disabled/boundary).
7. Recording action table mapped to `egui_port_get_recording_action()`.
8. Runtime + baseline acceptance criteria and pass conditions.
9. Known limits and next iteration plan.
10. Functional overlap analysis with existing widgets and explicit differentiation plan.

## Step 2: Implement C widget

- Add `src/widget/egui_view_<widget>.h/.c`.
- Expose header in `src/egui.h`.

## Step 3: Register in UI Designer

- Add `scripts/ui_designer/custom_widgets/<widget>.py`.
- Keep `type_name`, `xml_tag`, params struct, properties/events aligned with C APIs.

## Step 4: Add HelloBasic demo

- Add `example/HelloBasic/<widget>/test.c`.
- Add `example/HelloBasic/<widget>/app_egui_config.h`.
- Update `example/HelloBasic/build.mk` comment list with the new `APP_SUB`.
- Implement `egui_port_get_recording_action()` so key states are captured.

## Step 4.5: Recursive quality iteration gate (required)

Each new widget must complete at least 30 recursive iteration cycles before the flow can be closed.

```bash
# initialize template (auto-generated by Step 0, but can be created manually)
python scripts/widget/widget_iteration_gate.py --widget segmented_control --init

# record one cycle (run this repeatedly during refinement)
python scripts/widget/widget_iteration_gate.py --widget segmented_control --record \
  --goal "improve disabled contrast" \
  --changes "adjust disabled state color and spacing" \
  --verification "runtime_check + screenshot diff" \
  --visual-validation "checked latest screenshots and selected state contrast" \
  --interaction-validation "verified click/tap behavior and state transition are correct" \
  --changed-files "src/widget/egui_view_segmented_control.c,example/HelloBasic/segmented_control/test.c" \
  --visual-artifacts "runtime_check_output/HelloBasic_segmented_control/default/frame_0000.png,runtime_check_output/HelloBasic_segmented_control/visual_report.json" \
  --result PASS

# check gate status
python scripts/widget/widget_iteration_gate.py --widget segmented_control
```

Rule:

- `completed_cycles >= 30`
- latest cycle result must be `PASS`
- each cycle must include visual validation notes
- each cycle must include interaction validation notes
- each cycle must include changed file proof (`--changed-files`) and runtime proof (`visual_report` + `interaction_report` must be PASS at record time)
- gate requires enough real iteration diversity:
  - `unique_change_cycles >= max(3, min_cycles/2)` (default 30 cycles => >=15)
  - `visual_delta_cycles >= max(3, min_cycles/2)` (default 30 cycles => >=15)
- missing/invalid cycle entries are blocked

## Step 5: Runtime check + baseline acceptance (includes iteration gate)

```bash
# first run: runtime + first acceptance only (no baseline pass yet)
python scripts/widget/widget_acceptance_flow.py --widget segmented_control

# explicit baseline promotion
python scripts/widget/widget_acceptance_flow.py --widget segmented_control --promote-baseline

# regression mode
python scripts/widget/widget_acceptance_flow.py --widget segmented_control
```

Step 5 includes interaction validation by default:

- checks frame-to-frame transition changes from recording actions
- fails when changed transitions are below threshold (default `>=2`)
- for passive/non-interactive widgets, explicitly pass `--skip-interaction-check`
- runtime self-check marker is enforced: any app log containing `[RUNTIME_CHECK_FAIL]` fails runtime check immediately
- runtime clipping self-check rule is enforced by default:
  - `example/HelloBasic/<widget>/test.c` must include `[RUNTIME_CHECK_FAIL]` marker output
  - must include boundary clipping checks using `region_screen` vs `region` (width/height)
  - required for detecting hidden clipping issues (e.g., bottom button cut off but baseline still passes)

## Step 6: API + init_params usability gate (required final step)

```bash
python scripts/widget/widget_api_review_gate.py --widget segmented_control
```

Rules:

- header/source public API must expose `init/apply_params/init_with_params`
- `params` struct must include `egui_region_t region`
- `init_with_params` must call `init()` + `apply_params()`
- setters must map to real implementation and trigger invalidation on changes

`widget_acceptance_flow.py` runs this gate automatically as the final pass condition.

## Step 6.5: Readme rationale gate (required final step)

`widget_acceptance_flow.py` now runs readme rationale gate by default:

```bash
python scripts/widget/widget_readme_review_gate.py --widget segmented_control
```

Rules:

- readme must explicitly include:
  - why this widget exists
  - why existing widgets are not enough
  - overlap/differentiation boundary
- readme must be Chinese content with UTF-8 encoding
- if widget is wrapper/composition-based, readme must name wrapped base widget(s)

## Step 7: Portfolio tracker update (required before switching widgets)

Before closing the current widget and before starting the next one, update `.claude/workflow/widget_progress_tracker.md`:

- keep at most one active row in `当前进行中`
- when acceptance passes and baseline is promoted, move the widget into `已完成控件`
- record widget slug, category, completion date, accepted iteration count, innovation keywords, non-overlap notes, and key artifact paths
- if the widget is paused, move it to `已搁置/待恢复` with reason and next action; do not silently leave stale active records
- clear `当前进行中` only after the bookkeeping is finished

## Scope

For each `HelloBasic` widget/sub-app:

1. Widget is implemented in C layer.
2. Widget is registered in `scripts/ui_designer/custom_widgets/`.
3. Widget demo exists under `example/HelloBasic/<widget>/`.
4. Detailed design doc exists at `example/HelloBasic/<widget>/readme.md`.
5. Recursive iteration gate passes (`>=30` cycles and latest PASS).
6. Runtime render passes and visual diff gate passes.
7. API + init_params usability gate passes.
8. Portfolio tracker is updated in `.claude/workflow/widget_progress_tracker.md`.

## Result Artifacts

- Runtime frames: `runtime_check_output/HelloBasic_<widget>/default/frame_*.png`
- Visual report: `runtime_check_output/HelloBasic_<widget>/visual_report.json`
- Interaction report: `runtime_check_output/HelloBasic_<widget>/interaction_report.json`
- API review report: `runtime_check_output/HelloBasic_<widget>/api_review_report.json`
- Failed frame diffs: `runtime_check_output/HelloBasic_<widget>/visual_diff/`
- Baseline frames (local default): `runtime_check_output/baseline_local/HelloBasic_<widget>/default/frame_*.png`
- Iteration gate report: `runtime_check_output/HelloBasic_<widget>/iteration_reports/<widget>.json`
- Portfolio tracker: `.claude/workflow/widget_progress_tracker.md`

## Gate Rules

- Runtime check must pass (`code_runtime_check.py` returns success).
- Runtime self-check marker must be clean (no `[RUNTIME_CHECK_FAIL]` in app output).
- Runtime clipping self-check rule must be present in `test.c`.
- Interaction validation must pass (changed transitions from recording actions).
- Frames must not be blank (`min stddev` guard).
- Recursive iteration gate must pass (`min-iterations >= 30`, latest cycle PASS).
- Recursive iteration gate must include strict proof:
  - changed files tracked per cycle
  - visual/interaction runtime proof tracked per cycle
  - minimum unique change cycles and visual delta cycles reached
- API + init_params usability gate must pass.
- Readme rationale gate must pass.
- Portfolio tracker must be updated before moving to the next widget.
- **If baseline is missing**: only run first acceptance checks; regression is `NOT_EVALUATED` (not a regression PASS).
- **After explicit baseline promotion**: frame names/count must match baseline and each frame SSIM must be >= threshold (default `0.92`).

For animated widgets, recorder uses frame-stability capture:

- waits for unchanged framebuffer cycles (`snapshot-stable-cycles`)
- has timeout fallback (`snapshot-max-wait-ms`) to avoid hangs
- supports accelerated recording clock (`clock-scale`) for faster iteration

Important: keep `clock-scale` consistent between baseline capture and regression comparison.

Recommended fast profile (validated on `HelloStyleDemo` + `HelloBasic_heart_rate`):

- `clock-scale=6`
- `snapshot-settle-ms=0`
- `snapshot-stable-cycles=1`
- `snapshot-max-wait-ms=1500`

Fallback for unstable animated pages:

- keep `clock-scale=6`
- raise `snapshot-max-wait-ms` to `2500`

Runtime step can be skipped when fresh frames already exist:

```bash
python scripts/widget/widget_acceptance_flow.py --widget checkbox --skip-runtime-check
```

## Exit Code

- `0`: Passed (regression pass, or baseline explicitly promoted)
- `1`: Failed (runtime/render/compare issue)
- `2`: Hold (baseline not promoted yet, or recursive iteration gate not satisfied)

## Useful Options

```bash
--threshold <float>          # default 0.92
--min-stddev <float>         # default 2.0
--timeout <seconds>          # default 20
--speed <int>                # default 1
--clock-scale <int>          # default 6 (accelerate recording timeline)
--skip-runtime-check         # only compare existing frames
--skip-runtime-self-check-rule # emergency baseline maintenance only
--promote-baseline           # explicit baseline promotion/refresh
--baseline-dir <path>        # custom local baseline directory
--allow-frame-mismatch       # compare only common frames (for timing jitter scenarios)
--snapshot-settle-ms <ms>    # default 0
--snapshot-stable-cycles <n> # default 1
--snapshot-max-wait-ms <ms>  # default 1500
--min-interaction-transitions <n>  # default 2
--interaction-diff-threshold <f>   # default 0.1
--skip-interaction-check      # only for passive/non-interactive widgets
--skip-api-review-gate        # only for emergency baseline maintenance
--api-review-report <path>    # custom API review report output path
--skip-readme-gate            # only for emergency baseline maintenance
--readme-review-report <path> # custom readme gate report output path
--min-iterations <n>         # default 30
--skip-iteration-gate        # for baseline maintenance only

# widget_iteration_gate proof options
--changed-files <csv>             # required for strict cycle proof
--visual-artifacts <csv>          # optional extra artifact links
--min-unique-change-cycles <n>    # default max(3, min_cycles/2)
--min-visual-delta-cycles <n>     # default max(3, min_cycles/2)
--skip-proof-validation           # emergency fallback only
```

Runtime recorder options:

```bash
python scripts/code_runtime_check.py --app HelloBasic --app-sub spinner \
  --clock-scale 6 --snapshot-settle-ms 0 --snapshot-stable-cycles 1 --snapshot-max-wait-ms 1500
```

Environment variable:

```bash
EGUI_BASELINE_DIR=<path>     # default baseline directory for both scripts
```

## Bootstrap Existing Baselines

```bash
# registered widgets only (default)
python scripts/widget/widget_baseline_bootstrap.py

# refresh existing baseline too
python scripts/widget/widget_baseline_bootstrap.py --refresh

# all HelloBasic sub-apps (including non-widget demos)
python scripts/widget/widget_baseline_bootstrap.py --scope hellobasic

# capture current node before starting a new widget
python scripts/widget/widget_baseline_bootstrap.py --scope registered --refresh

# reuse existing fresh runtime frames
python scripts/widget/widget_baseline_bootstrap.py --scope registered --refresh --skip-runtime-check
```

Output report:

- `runtime_check_output/baseline_local/baseline_bootstrap_report.json`
