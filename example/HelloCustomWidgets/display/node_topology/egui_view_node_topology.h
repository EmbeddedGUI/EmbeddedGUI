#ifndef _EGUI_VIEW_NODE_TOPOLOGY_H_
#define _EGUI_VIEW_NODE_TOPOLOGY_H_

#include "egui.h"

#ifdef __cplusplus
extern "C" {
#endif

#define EGUI_VIEW_NODE_TOPOLOGY_MAX_NODES 5
#define EGUI_VIEW_NODE_TOPOLOGY_MAX_LINKS 6
#define EGUI_VIEW_NODE_TOPOLOGY_MAX_SNAPSHOTS 3

typedef struct egui_view_node_topology_node egui_view_node_topology_node_t;
struct egui_view_node_topology_node
{
    uint8_t x;
    uint8_t y;
    uint8_t radius;
    const char *label;
    uint8_t status;
};

typedef struct egui_view_node_topology_link egui_view_node_topology_link_t;
struct egui_view_node_topology_link
{
    uint8_t from_index;
    uint8_t to_index;
    uint8_t active;
};

typedef struct egui_view_node_topology_snapshot egui_view_node_topology_snapshot_t;
struct egui_view_node_topology_snapshot
{
    const char *title;
    const egui_view_node_topology_node_t *nodes;
    const egui_view_node_topology_link_t *links;
    uint8_t node_count;
    uint8_t link_count;
    uint8_t focus_node;
};

typedef struct egui_view_node_topology egui_view_node_topology_t;
struct egui_view_node_topology
{
    egui_view_t base;
    const egui_view_node_topology_snapshot_t *snapshots;
    const egui_font_t *font;
    egui_color_t surface_color;
    egui_color_t border_color;
    egui_color_t text_color;
    egui_color_t muted_text_color;
    egui_color_t active_color;
    uint8_t snapshot_count;
    uint8_t current_snapshot;
    uint8_t focus_node;
    uint8_t show_header;
    uint8_t compact_mode;
};

void egui_view_node_topology_init(egui_view_t *self);
void egui_view_node_topology_set_snapshots(egui_view_t *self, const egui_view_node_topology_snapshot_t *snapshots, uint8_t snapshot_count);
void egui_view_node_topology_set_current_snapshot(egui_view_t *self, uint8_t snapshot_index);
uint8_t egui_view_node_topology_get_current_snapshot(egui_view_t *self);
void egui_view_node_topology_set_focus_node(egui_view_t *self, uint8_t node_index);
void egui_view_node_topology_set_font(egui_view_t *self, const egui_font_t *font);
void egui_view_node_topology_set_show_header(egui_view_t *self, uint8_t show_header);
void egui_view_node_topology_set_compact_mode(egui_view_t *self, uint8_t compact_mode);
void egui_view_node_topology_set_palette(
        egui_view_t *self,
        egui_color_t surface_color,
        egui_color_t border_color,
        egui_color_t text_color,
        egui_color_t muted_text_color,
        egui_color_t active_color);

#ifdef __cplusplus
}
#endif

#endif /* _EGUI_VIEW_NODE_TOPOLOGY_H_ */
