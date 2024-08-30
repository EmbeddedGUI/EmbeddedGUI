#ifndef _EGUI_RESOURCE_H_
#define _EGUI_RESOURCE_H_

#include "font/egui_font.h"
#include "font/egui_font_std.h"


/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif




// Used id.
enum {
    EGUI_VIEW_ID_ROOT_VIEW_GROUP = 0x00,

    EGUI_VIEW_ID_USER_ROOT_VIEW_GROUP = 0x10,

    EGUI_VIEW_ID_DEBUG_INFO_TEXTVIEW = 0x80,

    EGUI_VIEW_ID_USER_ID_BEGIN = 0x100,
};

extern const egui_font_std_t egui_res_font_montserrat_8_4; 
extern const egui_font_std_t egui_res_font_montserrat_10_4;
extern const egui_font_std_t egui_res_font_montserrat_12_4;
extern const egui_font_std_t egui_res_font_montserrat_14_4;
extern const egui_font_std_t egui_res_font_montserrat_16_4;
extern const egui_font_std_t egui_res_font_montserrat_18_4;
extern const egui_font_std_t egui_res_font_montserrat_20_4;
extern const egui_font_std_t egui_res_font_montserrat_22_4;
extern const egui_font_std_t egui_res_font_montserrat_24_4;
extern const egui_font_std_t egui_res_font_montserrat_26_4;
extern const egui_font_std_t egui_res_font_montserrat_28_4;
extern const egui_font_std_t egui_res_font_montserrat_30_4;
extern const egui_font_std_t egui_res_font_montserrat_32_4;
extern const egui_font_std_t egui_res_font_montserrat_34_4;
extern const egui_font_std_t egui_res_font_montserrat_36_4;
extern const egui_font_std_t egui_res_font_montserrat_38_4;
extern const egui_font_std_t egui_res_font_montserrat_30_4;
extern const egui_font_std_t egui_res_font_montserrat_42_4;
extern const egui_font_std_t egui_res_font_montserrat_44_4;
extern const egui_font_std_t egui_res_font_montserrat_46_4;
extern const egui_font_std_t egui_res_font_montserrat_48_4;

// BITMAP
typedef struct struct_bitmap_info
{
    unsigned short width;
    unsigned short height;
    unsigned short color_bits; // support 16 bits only
    const unsigned short *pixel_color_array;
} BITMAP_INFO;




/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif


#endif /* _EGUI_RESOURCE_H_ */
