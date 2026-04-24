# Rotation

Display software rotation test.

Cycles through 0/90/180/270 degree screen rotations using `egui_display_set_rotation()`.

Labels at different screen positions show the current rotation angle, making the rotation effect clearly visible.

Requires software rotation to be enabled, for example via `EGUI_CONFIG_SOFTWARE_ROTATION = 1` or `egui_display_setup_t.render_config->software_rotation = 1`.
