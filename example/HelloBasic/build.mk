EGUI_CODE_SRC		+= $(EGUI_APP_PATH)

EGUI_CODE_INCLUDE	+= $(EGUI_APP_PATH)


# select the sub app
# APP_SUB ?= anim
APP_SUB ?= button
# APP_SUB ?= button_img
# APP_SUB ?= image
# APP_SUB ?= label
# APP_SUB ?= linearlayout
# APP_SUB ?= mask
# APP_SUB ?= progress_bar
# APP_SUB ?= scroll
# APP_SUB ?= switch
# APP_SUB ?= viewpage
# APP_SUB ?= viewpage_cache
# APP_SUB ?= virtual_page
# APP_SUB ?= virtual_strip
# APP_SUB ?= virtual_grid
# APP_SUB ?= virtual_section_list
# APP_SUB ?= virtual_tree
# APP_SUB ?= virtual_viewport
# APP_SUB ?= mp4
# APP_SUB ?= checkbox
# APP_SUB ?= radio_button
# APP_SUB ?= slider
# APP_SUB ?= circular_progress_bar
# APP_SUB ?= image_button
# APP_SUB ?= divider
# APP_SUB ?= page_indicator
# APP_SUB ?= gauge
# APP_SUB ?= number_picker
# APP_SUB ?= tab_bar
# APP_SUB ?= segmented_control
# APP_SUB ?= pattern_lock
# APP_SUB ?= gridlayout
# APP_SUB ?= led
# APP_SUB ?= toggle_button
# APP_SUB ?= spinner
# APP_SUB ?= card
# APP_SUB ?= arc_slider
# APP_SUB ?= roller
# APP_SUB ?= textinput
# APP_SUB ?= textblock
# APP_SUB ?= combobox
# APP_SUB ?= autocomplete
# APP_SUB ?= notification_badge
# APP_SUB ?= activity_ring
# APP_SUB ?= analog_clock
# APP_SUB ?= heart_rate
# APP_SUB ?= compass
# APP_SUB ?= stopwatch
# APP_SUB ?= digital_clock
# APP_SUB ?= mini_calendar
# APP_SUB ?= line
# APP_SUB ?= scale
# APP_SUB ?= button_matrix
# APP_SUB ?= chips
# APP_SUB ?= stepper
# APP_SUB ?= table
# APP_SUB ?= animated_image
# APP_SUB ?= list
# APP_SUB ?= spangroup
# APP_SUB ?= tileview
# APP_SUB ?= window
# APP_SUB ?= menu
# APP_SUB ?= enhanced_widgets

EGUI_APP_SUB_PATH := $(EGUI_APP_PATH)/$(APP_SUB)

# Each sub-app has its own app_egui_config.h, so they need separate OBJDIRs
# to ensure framework sources are compiled with the correct config flags.
APP_OBJ_SUFFIX := HelloBasic_$(APP_SUB)

EGUI_CODE_SRC		+= $(EGUI_APP_SUB_PATH)
EGUI_CODE_SRC		+= $(EGUI_APP_SUB_PATH)/resource
EGUI_CODE_SRC		+= $(EGUI_APP_SUB_PATH)/resource/img
EGUI_CODE_SRC		+= $(EGUI_APP_SUB_PATH)/resource/font

EGUI_CODE_INCLUDE	+= $(EGUI_APP_SUB_PATH)
EGUI_CODE_INCLUDE	+= $(EGUI_APP_SUB_PATH)/resource


EGUI_APP_RESOURCE_PATH ?= $(EGUI_APP_SUB_PATH)/resource
