---
name: recording-simulation
description: Add input simulation (clicks, drags, swipes) to EmbeddedGUI examples for GIF recording
---

# Recording Simulation Skill

This skill explains how to add input simulation (clicks, drags, swipes) to EmbeddedGUI examples for GIF recording.

## Overview

When recording GIF demos with `EGUI_CONFIG_RECORDING_TEST=1`, you can simulate user interactions automatically. The system supports:

- **Click**: Single tap at a position
- **Drag**: Touch down, move, touch up (slow, smooth)
- **Swipe**: Fast drag gesture
- **Wait**: Pause without any action

## Quick Start

### 1. Enable Recording Test in `app_egui_config.h`

```c
#define EGUI_CONFIG_RECORDING_TEST 1
```

### 2. Include Header and Implement Actions in `uicode.c`

```c
#if EGUI_CONFIG_RECORDING_TEST
#include "core/egui_input_simulator.h"
#endif

// ... view definitions ...

#if EGUI_CONFIG_RECORDING_TEST
bool egui_port_get_recording_action(int action_index, egui_sim_action_t *p_action)
{
    if (action_index >= 3) return false;  // Stop after 3 actions

    // Get button center position
    int x, y;
    egui_sim_get_view_center(&my_button, &x, &y);

    p_action->type = EGUI_SIM_ACTION_CLICK;
    p_action->x1 = x;
    p_action->y1 = y;
    p_action->interval_ms = 1000;  // 1 second between clicks

    return true;
}
#endif
```

## Action Macros Reference

| Macro | Parameters | Description |
|-------|------------|-------------|
| `EGUI_SIM_CLICK(x, y, interval_ms)` | Position and delay | Single click |
| `EGUI_SIM_DRAG(x1, y1, x2, y2, steps, interval_ms)` | Start, end, steps, delay | Smooth drag |
| `EGUI_SIM_SWIPE(x1, y1, x2, y2, interval_ms)` | Start, end, delay | Fast swipe (5 steps) |
| `EGUI_SIM_WAIT(interval_ms)` | Delay | Wait without action |
| `EGUI_SIM_END()` | None | End marker |

## Action Set Macros (Recommended)

Use these macros to set action directly on p_action pointer:

| Macro | Description |
|-------|-------------|
| `EGUI_SIM_SET_CLICK_VIEW(p_action, &view, interval_ms)` | Click view center |
| `EGUI_SIM_SET_DRAG_VIEW(p_action, &from, &to, steps, interval_ms)` | Drag between views |
| `EGUI_SIM_SET_SWIPE_VIEW(p_action, &from, &to, interval_ms)` | Swipe between views |
| `EGUI_SIM_SET_WAIT(p_action, interval_ms)` | Wait |

### Example

```c
bool egui_port_get_recording_action(int action_index, egui_sim_action_t *p_action)
{
    if (action_index >= 3) return false;
    EGUI_SIM_SET_CLICK_VIEW(p_action, &button_1, 1000);
    return true;
}
```

## Precise Frame Capture

Use `recording_request_snapshot()` to capture a frame at the exact moment you need,
instead of relying on fixed wait times.

### API

```c
// Request a screenshot on the next rendered frame
recording_request_snapshot();
```

Call this in your `egui_port_get_recording_action()` callback whenever you want
a frame captured. The screenshot is taken after the next render completes,
ensuring the framebuffer reflects the latest state.

**IMPORTANT:** Always guard `recording_request_snapshot()` with `first_call` check.
The callback is called every frame cycle during the wait interval, so without the
guard, snapshots are requested every frame, causing excessive frame captures.

### Example: Multi-page app with animations

```c
bool egui_port_get_recording_action(int action_index, egui_sim_action_t *p_action)
{
    static int last_action = -1;
    int first_call = (action_index != last_action);
    last_action = action_index;

    switch (action_index)
    {
    case 0: // Capture initial page
        if (first_call)
            recording_request_snapshot();
        EGUI_SIM_SET_WAIT(p_action, 200);
        return true;
    case 1: // Switch page and capture
        if (first_call)
        {
            uicode_switch_page(1);
            recording_request_snapshot();
        }
        EGUI_SIM_SET_WAIT(p_action, 200);
        return true;
    default:
        return false;
    }
}
```

**Note:** If you don't call `recording_request_snapshot()`, the framework
automatically captures a frame after each action completes (fallback behavior).
This ensures existing code works without modification.

## Helper Functions

### Get View Center Position

```c
int x, y;
egui_sim_get_view_center(&my_button, &x, &y);
```

### Get Relative Position in View

```c
// Get position at 25% from left, 50% from top
int x, y;
egui_sim_get_view_pos(&my_view, 0.25f, 0.5f, &x, &y);
```

### Center Coordinate Macros

```c
EGUI_SIM_VIEW_CENTER_X(&my_button)
EGUI_SIM_VIEW_CENTER_Y(&my_button)
```

## Static Action Array Example

For simple cases with predefined actions:

```c
#if EGUI_CONFIG_RECORDING_TEST
#include "core/egui_input_simulator.h"
#endif

#if EGUI_CONFIG_RECORDING_TEST
bool egui_port_get_recording_action(int action_index, egui_sim_action_t *p_action)
{
    static const egui_sim_action_t actions[] = {
        EGUI_SIM_WAIT(500),
        EGUI_SIM_CLICK(120, 160, 1000),
        EGUI_SIM_DRAG(50, 160, 190, 160, 10, 500),
        EGUI_SIM_SWIPE(190, 160, 50, 160, 500),
        EGUI_SIM_END()
    };

    if (actions[action_index].type == EGUI_SIM_ACTION_NONE) {
        return false;
    }
    *p_action = actions[action_index];
    return true;
}
#endif
```

## Dynamic Actions Example

For actions that depend on view positions:

```c
#if EGUI_CONFIG_RECORDING_TEST
bool egui_port_get_recording_action(int action_index, egui_sim_action_t *p_action)
{
    int btn_x, btn_y;
    egui_sim_get_view_center(&button_1, &btn_x, &btn_y);

    switch (action_index)
    {
    case 0:  // Click button
        p_action->type = EGUI_SIM_ACTION_CLICK;
        p_action->x1 = btn_x;
        p_action->y1 = btn_y;
        p_action->interval_ms = 1000;
        return true;

    case 1:  // Drag viewpage left
        p_action->type = EGUI_SIM_ACTION_DRAG;
        p_action->x1 = EGUI_CONFIG_SCEEN_WIDTH * 3 / 4;
        p_action->y1 = EGUI_CONFIG_SCEEN_HEIGHT / 2;
        p_action->x2 = EGUI_CONFIG_SCEEN_WIDTH / 4;
        p_action->y2 = EGUI_CONFIG_SCEEN_HEIGHT / 2;
        p_action->steps = 15;
        p_action->interval_ms = 500;
        return true;

    default:
        return false;
    }
}
#endif
```

## ViewPage/Scroll Examples

```c
// Swipe left (show next page)
EGUI_SIM_SWIPE(180, 160, 60, 160, 1000)

// Swipe right (show previous page)
EGUI_SIM_SWIPE(60, 160, 180, 160, 1000)

// Scroll down (in scroll view)
EGUI_SIM_DRAG(120, 250, 120, 50, 20, 500)
```

## Run GIF Recording

```bash
# Record single app
python scripts/gif_recorder.py --app HelloSimple

# Custom duration and FPS
python scripts/gif_recorder.py --app HelloSimple --duration 8 --fps 15
```

## Files Reference

| File | Description |
|------|-------------|
| `src/core/egui_input_simulator.h` | Action types, helper macros, and `recording_request_snapshot()` declaration |
| `porting/pc/sdl_port.c` | Simulation execution logic and `recording_request_snapshot()` implementation |
| `src/core/egui_config_default.h` | `EGUI_CONFIG_RECORDING_TEST` macro |
