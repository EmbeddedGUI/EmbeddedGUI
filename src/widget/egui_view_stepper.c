#include "egui_view_stepper.h"

void egui_view_stepper_set_total_steps(egui_view_t *self, uint8_t total_steps)
{
    EGUI_LOCAL_INIT(egui_view_page_indicator_t);
    if (total_steps == 0)
    {
        total_steps = 1;
    }
    if (local->total_count == total_steps)
    {
        return;
    }
    egui_view_page_indicator_set_total_count(self, total_steps);
}

uint8_t egui_view_stepper_get_total_steps(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_page_indicator_t);
    return local->total_count;
}

void egui_view_stepper_set_current_step(egui_view_t *self, uint8_t current_step)
{
    EGUI_LOCAL_INIT(egui_view_page_indicator_t);
    if (local->total_count > 0 && current_step >= local->total_count)
    {
        current_step = (uint8_t)(local->total_count - 1);
    }
    if (local->current_index == current_step)
    {
        return;
    }
    egui_view_page_indicator_set_current_index(self, current_step);
}

uint8_t egui_view_stepper_get_current_step(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_page_indicator_t);
    return local->current_index;
}

void egui_view_stepper_init(egui_view_t *self)
{
    egui_view_page_indicator_init(self);
    egui_view_set_view_name(self, "egui_view_stepper");
}

void egui_view_stepper_apply_params(egui_view_t *self, const egui_view_stepper_params_t *params)
{
    uint8_t total_steps = params->total_steps;
    uint8_t current_step = params->current_step;
    if (total_steps == 0)
    {
        total_steps = 1;
    }
    if (current_step >= total_steps)
    {
        current_step = (uint8_t)(total_steps - 1U);
    }
    egui_view_page_indicator_params_t indicator_params = {
            .region = params->region,
            .total_count = total_steps,
            .current_index = current_step,
    };
    egui_view_page_indicator_apply_params(self, &indicator_params);
}

void egui_view_stepper_init_with_params(egui_view_t *self, const egui_view_stepper_params_t *params)
{
    egui_view_stepper_init(self);
    egui_view_stepper_apply_params(self, params);
}
