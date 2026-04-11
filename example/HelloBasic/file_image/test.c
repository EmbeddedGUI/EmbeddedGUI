#include "egui.h"
#include "uicode.h"

#include "decoder_bmp_stream.h"
#include "decoder_stb.h"
#include "decoder_tjpgd_stream.h"
#include "file_io_stdio.h"
#include "file_image_stack.h"

#define CARD_W        108
#define CARD_H        92
#define CARD_IMAGE_W  88
#define CARD_IMAGE_H  56
#define CARD_STATUS_H 12
#define CARD_GAP_X    8
#define CARD_GAP_Y    10
#define CARD_LEFT_X   8
#define CARD_RIGHT_X  (CARD_LEFT_X + CARD_W + CARD_GAP_X)
#define CARD_ROW0_Y   8
#define CARD_ROW1_Y   (CARD_ROW0_Y + CARD_H + CARD_GAP_Y)
#define CARD_ROW2_Y   (CARD_ROW1_Y + CARD_H + CARD_GAP_Y)
#define CARD_IMAGE_Y  20
#define CARD_STATUS_Y 78

#define FILE_IMAGE_TITLE_FONT  ((const egui_font_t *)&egui_res_font_montserrat_12_4)
#define FILE_IMAGE_STATUS_FONT ((const egui_font_t *)&egui_res_font_montserrat_10_4)

#define FILE_IMAGE_JPG     "sd:sample_landscape.jpg"
#define FILE_IMAGE_PNG     "lfs:sample_overlay.png"
#define FILE_IMAGE_BMP     "flash:sample_badge.bmp"
#define FILE_IMAGE_MISSING "flash:missing_file.png"

typedef struct file_image_demo_card
{
    egui_view_t panel;
    egui_view_label_t title;
    egui_view_image_t image;
    egui_view_label_t status;
    char status_text[32];
} file_image_demo_card_t;

static egui_view_group_t root_group;

static file_image_demo_card_t jpg_card;
static file_image_demo_card_t png_card;
static file_image_demo_card_t bmp_card;
static file_image_demo_card_t resize_card;
static file_image_demo_card_t missing_card;

static file_image_stdio_context_t file_image_sd_ctx = {
        .root_prefix = "example/HelloBasic/file_image/files/",
};
static file_image_stdio_context_t file_image_lfs_ctx = {
        .root_prefix = "example/HelloBasic/file_image/files/",
};
static file_image_stdio_context_t file_image_flash_ctx = {
        .root_prefix = "example/HelloBasic/file_image/files/",
};
static egui_image_file_io_t file_image_sd_io;
static egui_image_file_io_t file_image_lfs_io;
static egui_image_file_io_t file_image_flash_io;
static file_image_stack_state_t file_image_stack_state;
static const file_image_mount_router_entry_t file_image_mount_entries[] = {
        { "sd:", &file_image_sd_io, 1 },
        { "lfs:", &file_image_lfs_io, 1 },
        { "flash:", &file_image_flash_io, 1 },
};
static const file_image_decoder_registry_config_t file_image_decoder_config = {
        .bmp_stream = &g_file_image_bmp_stream_decoder,
        .jpeg_vendor = NULL,
        .jpeg_stream = &g_file_image_tjpgd_stream_decoder,
        .png_vendor = NULL,
        .generic_fallback = &g_file_image_stb_decoder,
        .clear_first = 1,
};
static const file_image_stack_config_t file_image_stack_config = {
        .default_io = NULL,
        .mount_entries = file_image_mount_entries,
        .mount_entry_count = sizeof(file_image_mount_entries) / sizeof(file_image_mount_entries[0]),
        .fallback_io = &file_image_lfs_io,
        .decoder_config = &file_image_decoder_config,
};

static egui_image_file_t jpg_image;
static egui_image_file_t png_image;
static egui_image_file_t bmp_image;
static egui_image_file_t resize_image;
static egui_image_file_t missing_image;

EGUI_VIEW_GROUP_PARAMS_INIT(root_group_p, 0, 0, EGUI_CONFIG_SCEEN_WIDTH, EGUI_CONFIG_SCEEN_HEIGHT);

EGUI_BACKGROUND_COLOR_PARAM_INIT_SOLID(bg_root_param, EGUI_COLOR_HEX(0x0F172A), EGUI_ALPHA_100);
EGUI_BACKGROUND_PARAM_INIT(bg_root_params, &bg_root_param, &bg_root_param, &bg_root_param);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(bg_root, &bg_root_params);

EGUI_BACKGROUND_COLOR_PARAM_INIT_ROUND_RECTANGLE_STROKE(bg_card_param, EGUI_COLOR_HEX(0xE2E8F0), EGUI_ALPHA_100, 8, 1, EGUI_COLOR_HEX(0x94A3B8),
                                                        EGUI_ALPHA_100);
EGUI_BACKGROUND_PARAM_INIT(bg_card_params, &bg_card_param, &bg_card_param, &bg_card_param);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(bg_card, &bg_card_params);

static void file_image_demo_init_label(egui_view_label_t *label, egui_dim_t x, egui_dim_t y, egui_dim_t width, egui_dim_t height, const char *text,
                                       const egui_font_t *font, egui_color_t color)
{
    egui_view_label_init(EGUI_VIEW_OF(label));
    egui_view_set_position(EGUI_VIEW_OF(label), x, y);
    egui_view_set_size(EGUI_VIEW_OF(label), width, height);
    egui_view_label_set_font(EGUI_VIEW_OF(label), font);
    egui_view_label_set_text(EGUI_VIEW_OF(label), text);
    egui_view_label_set_align_type(EGUI_VIEW_OF(label), EGUI_ALIGN_CENTER);
    egui_view_label_set_font_color(EGUI_VIEW_OF(label), color, EGUI_ALPHA_100);
}

static void file_image_demo_copy_text(char *dst, uint32_t dst_size, const char *src)
{
    uint32_t i;

    if (dst == NULL || dst_size == 0)
    {
        return;
    }
    if (src == NULL)
    {
        dst[0] = '\0';
        return;
    }

    for (i = 0; i + 1 < dst_size && src[i] != '\0'; i++)
    {
        dst[i] = src[i];
    }
    dst[i] = '\0';
}

static void file_image_demo_build_status_text(char *dst, uint32_t dst_size, const egui_image_file_t *image, const egui_image_file_t *placeholder_image)
{
    egui_image_file_status_t status;
    const char *decoder_name;
    const char *placeholder_text = NULL;

    if (dst == NULL || dst_size == 0)
    {
        return;
    }

    status = egui_image_file_get_status(image);
    decoder_name = egui_image_file_get_decoder_name(image);
    if (status == EGUI_IMAGE_FILE_STATUS_READY && decoder_name != NULL)
    {
        file_image_demo_copy_text(dst, dst_size, decoder_name);
        return;
    }

    if (placeholder_image != NULL)
    {
        placeholder_text = egui_image_file_get_decoder_name(placeholder_image);
        if (placeholder_text == NULL)
        {
            placeholder_text = egui_image_file_status_to_string(egui_image_file_get_status(placeholder_image));
        }
    }

    if (placeholder_text != NULL)
    {
        file_image_demo_copy_text(dst, dst_size, "ph:");
        file_image_demo_copy_text(dst + 3, dst_size > 3 ? dst_size - 3 : 0, placeholder_text);
        return;
    }

    file_image_demo_copy_text(dst, dst_size, egui_image_file_status_to_string(status));
}

static void file_image_demo_init_card(file_image_demo_card_t *card, egui_dim_t x, egui_dim_t y, const char *title, egui_image_file_t *image, int resize_mode,
                                      egui_dim_t image_width, egui_dim_t image_height, const egui_image_file_t *placeholder_image)
{
    egui_dim_t image_x = x + (CARD_W - image_width) / 2;

    file_image_demo_build_status_text(card->status_text, sizeof(card->status_text), image, placeholder_image);

    egui_view_init(EGUI_VIEW_OF(&card->panel));
    egui_view_set_position(EGUI_VIEW_OF(&card->panel), x, y);
    egui_view_set_size(EGUI_VIEW_OF(&card->panel), CARD_W, CARD_H);
    egui_view_set_background(EGUI_VIEW_OF(&card->panel), EGUI_BG_OF(&bg_card));

    file_image_demo_init_label(&card->title, x, y + 4, CARD_W, 14, title, FILE_IMAGE_TITLE_FONT, EGUI_COLOR_HEX(0x0F172A));
    file_image_demo_init_label(&card->status, x, y + CARD_STATUS_Y, CARD_W, CARD_STATUS_H, card->status_text, FILE_IMAGE_STATUS_FONT, EGUI_COLOR_HEX(0x475569));

    egui_view_image_init(EGUI_VIEW_OF(&card->image));
    egui_view_set_position(EGUI_VIEW_OF(&card->image), image_x, y + CARD_IMAGE_Y);
    egui_view_set_size(EGUI_VIEW_OF(&card->image), image_width, image_height);
    egui_view_image_set_image(EGUI_VIEW_OF(&card->image), (egui_image_t *)image);
    egui_view_image_set_image_type(EGUI_VIEW_OF(&card->image), resize_mode);

    egui_view_group_add_child(EGUI_VIEW_OF(&root_group), EGUI_VIEW_OF(&card->panel));
    egui_view_group_add_child(EGUI_VIEW_OF(&root_group), EGUI_VIEW_OF(&card->image));
    egui_view_group_add_child(EGUI_VIEW_OF(&root_group), EGUI_VIEW_OF(&card->title));
    egui_view_group_add_child(EGUI_VIEW_OF(&root_group), EGUI_VIEW_OF(&card->status));
}

static void file_image_demo_prepare_image(egui_image_file_t *image, const char *path, const egui_image_t *placeholder)
{
    egui_image_file_init(image);
    egui_image_file_set_placeholder(image, placeholder);
    egui_image_file_set_path(image, path);
    egui_image_file_reload(image);
}

void test_init_ui(void)
{
    egui_view_group_init_with_params(EGUI_VIEW_OF(&root_group), &root_group_p);
    egui_view_set_background(EGUI_VIEW_OF(&root_group), EGUI_BG_OF(&bg_root));

    /*
     * The PC demo uses three stdio-backed logical mounts to simulate sd:/ lfs:/ flash: routing.
     * MCU targets can keep the same paths and replace only the IO adapters behind each mount.
     */
    file_image_stdio_io_init(&file_image_sd_io, &file_image_sd_ctx);
    file_image_stdio_io_init(&file_image_lfs_io, &file_image_lfs_ctx);
    file_image_stdio_io_init(&file_image_flash_io, &file_image_flash_ctx);
    EGUI_ASSERT(file_image_stack_apply(&file_image_stack_state, &file_image_stack_config));

    file_image_demo_prepare_image(&jpg_image, FILE_IMAGE_JPG, NULL);
    file_image_demo_prepare_image(&png_image, FILE_IMAGE_PNG, NULL);
    file_image_demo_prepare_image(&bmp_image, FILE_IMAGE_BMP, NULL);
    file_image_demo_prepare_image(&resize_image, FILE_IMAGE_JPG, NULL);
    file_image_demo_prepare_image(&missing_image, FILE_IMAGE_MISSING, (const egui_image_t *)&png_image);

    file_image_demo_init_card(&jpg_card, CARD_LEFT_X, CARD_ROW0_Y, "SD JPG", &jpg_image, EGUI_VIEW_IMAGE_TYPE_NORMAL, CARD_IMAGE_W, CARD_IMAGE_H, NULL);
    file_image_demo_init_card(&png_card, CARD_RIGHT_X, CARD_ROW0_Y, "LFS PNG", &png_image, EGUI_VIEW_IMAGE_TYPE_NORMAL, CARD_IMAGE_W, CARD_IMAGE_H, NULL);
    file_image_demo_init_card(&bmp_card, CARD_LEFT_X, CARD_ROW1_Y, "Flash BMP", &bmp_image, EGUI_VIEW_IMAGE_TYPE_NORMAL, CARD_IMAGE_W, CARD_IMAGE_H, NULL);
    file_image_demo_init_card(&resize_card, CARD_RIGHT_X, CARD_ROW1_Y, "SD JPG Resize", &resize_image, EGUI_VIEW_IMAGE_TYPE_RESIZE, 96, 40, NULL);
    file_image_demo_init_card(&missing_card, CARD_LEFT_X, CARD_ROW2_Y, "Missing -> LFS", &missing_image, EGUI_VIEW_IMAGE_TYPE_RESIZE, CARD_IMAGE_W,
                              CARD_IMAGE_H, &png_image);

    egui_core_add_user_root_view(EGUI_VIEW_OF(&root_group));
}

#if EGUI_CONFIG_RECORDING_TEST
bool egui_port_get_recording_action(int action_index, egui_sim_action_t *p_action)
{
    switch (action_index)
    {
    case 0:
        EGUI_SIM_SET_WAIT(p_action, 2000);
        return true;
    default:
        return false;
    }
}
#endif
