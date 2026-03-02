#ifndef _EGUI_VIEW_CHART_H_
#define _EGUI_VIEW_CHART_H_

// Backward compatibility wrapper.
// The monolithic egui_view_chart has been split into 4 independent chart widgets.
// Include this header to get all chart types, or include individual headers for
// smaller code size (linker can strip unused chart types).

#include "egui_view_chart_axis.h"
#include "egui_view_chart_common.h"
#include "egui_view_chart_line.h"
#include "egui_view_chart_scatter.h"
#include "egui_view_chart_bar.h"
#include "egui_view_chart_pie.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

// Preserved for reference - use type-specific widgets directly
typedef enum egui_chart_type
{
    EGUI_CHART_TYPE_LINE = 0,
    EGUI_CHART_TYPE_SCATTER,
    EGUI_CHART_TYPE_BAR,
    EGUI_CHART_TYPE_PIE,
} egui_chart_type_t;

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_VIEW_CHART_H_ */
