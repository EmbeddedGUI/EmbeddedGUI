# Rotation

Display software rotation test.

Cycles through 0/90/180/270 degree screen rotations using `egui_display_set_rotation()`.

Labels at different screen positions show the current rotation angle, making the rotation effect clearly visible.

Requires `EGUI_CONFIG_FUNCTION_SOFTWARE_ROTATION_ENABLE = 1`.
For multi-display setups, override it per core with `egui_display_setup_t.render_config->software_rotation = 1`.
