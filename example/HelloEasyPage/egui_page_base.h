#ifndef _EGUI_PAGE_BASE_H_
#define _EGUI_PAGE_BASE_H_

#include "egui.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

typedef struct egui_page_base egui_page_base_t;

typedef struct egui_page_base_api egui_page_base_api_t;
struct egui_page_base_api
{
    void (*on_open)(egui_page_base_t *self);
    void (*on_close)(egui_page_base_t *self);
    
    void (*on_key_pressed)(egui_page_base_t *self, uint16_t keycode);
};

struct egui_page_base
{
    egui_view_group_t root_view; // view of the page

#if EGUI_CONFIG_DEBUG_CLASS_NAME
    const char *name; // name of the view
#endif

    const egui_page_base_api_t *api; // api of the view
};
void egui_page_base_add_view(egui_page_base_t *self, egui_view_t *view);
void egui_page_base_set_name(egui_page_base_t *self, const char *name);


void egui_page_base_open(egui_page_base_t *self);
void egui_page_base_close(egui_page_base_t *self);
void egui_page_base_key_pressed(egui_page_base_t *self, uint16_t keycode);


void egui_page_base_on_open(egui_page_base_t *self);
void egui_page_base_on_close(egui_page_base_t *self);
void egui_page_base_on_key_pressed(egui_page_base_t *self, uint16_t keycode);

void egui_page_base_init(egui_page_base_t *self);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_PAGE_BASE_H_ */
