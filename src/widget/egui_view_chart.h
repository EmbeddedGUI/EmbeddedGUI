#ifndef _EGUI_VIEW_CHART_H_
#define _EGUI_VIEW_CHART_H_

/**
 * @brief Compatibility umbrella header for all built-in chart widgets.
 *
 * New code can include the type-specific chart headers directly, but this file

 * * remains useful for legacy code that expects one combined chart include.
 */

/* Backward-compatibility wrapper.
 * Include this header to pull in all chart widgets, or include only the
 * type-specific headers when code size matters. */

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

/** Legacy chart-type enum kept for compatibility. New code should use the type-specific widgets directly. */
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
