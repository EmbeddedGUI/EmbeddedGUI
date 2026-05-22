#include "egui_core.h"
#include "egui_encoder_driver.h"

#if EGUI_CONFIG_FUNCTION_ENCODER

#include "egui_input.h"
#include "egui_key_event.h"
#include "egui_timer.h"

/* ------------------------------------------------------------------ */
/* Registration                                                        */
/* ------------------------------------------------------------------ */

void egui_encoder_driver_register(egui_core_t *core, egui_encoder_driver_t *driver)
{
    if (core == NULL)
    {
        return;
    }
    core->system.encoder_driver = driver;
}

egui_encoder_driver_t *egui_encoder_driver_get(egui_core_t *core)
{
    if (core == NULL)
    {
        return NULL;
    }
    return core->system.encoder_driver;
}

/* ------------------------------------------------------------------ */
/* Polling                                                             */
/* ------------------------------------------------------------------ */

void egui_encoder_polling_work(egui_core_t *core)
{
    int16_t delta;
    uint8_t btn;
    int16_t i;
    uint32_t held_ms;
    egui_encoder_driver_t *drv;

    if (core == NULL)
    {
        return;
    }
    drv = core->system.encoder_driver;
    if (drv == NULL || drv->ops == NULL || drv->ops->read == NULL)
    {
        return;
    }

    delta = 0;
    btn = 0;
    if (drv->ops->read(drv->user_data, &delta, &btn) != 0)
    {
        return; /* hardware read error – skip this poll */
    }

    /* --- Rotation: one key-event pair per tick -------------------- */
    if (delta > 0)
    {
        for (i = 0; i < delta; i++)
        {
            egui_input_add_key(core, EGUI_KEY_EVENT_ACTION_DOWN, EGUI_KEY_CODE_RIGHT, 0, 0);
            egui_input_add_key(core, EGUI_KEY_EVENT_ACTION_UP, EGUI_KEY_CODE_RIGHT, 0, 0);
        }
    }
    else if (delta < 0)
    {
        for (i = 0; i > delta; i--)
        {
            egui_input_add_key(core, EGUI_KEY_EVENT_ACTION_DOWN, EGUI_KEY_CODE_LEFT, 0, 0);
            egui_input_add_key(core, EGUI_KEY_EVENT_ACTION_UP, EGUI_KEY_CODE_LEFT, 0, 0);
        }
    }

    /* --- Button edge detection ------------------------------------ */
    if (btn && !drv->_last_button)
    {
        /* press edge */
        egui_input_add_key(core, EGUI_KEY_EVENT_ACTION_DOWN, EGUI_KEY_CODE_ENTER, 0, 0);
        drv->_press_tick = egui_timer_get_current_time();
        drv->_long_press_sent = 0;
    }
    else if (!btn && drv->_last_button)
    {
        /* release edge */
        egui_input_add_key(core, EGUI_KEY_EVENT_ACTION_UP, EGUI_KEY_CODE_ENTER, 0, 0);
        drv->_long_press_sent = 0;
    }
    else if (btn && drv->_last_button && !drv->_long_press_sent)
    {
        /* held – emit one long-press event after the threshold */
        held_ms = egui_timer_get_current_time() - drv->_press_tick;
        if (held_ms >= EGUI_CONFIG_ENCODER_PRESS_LONG_MS)
        {
            egui_input_add_key(core, EGUI_KEY_EVENT_ACTION_LONG_PRESS, EGUI_KEY_CODE_ENTER, 0, 0);
            drv->_long_press_sent = 1;
        }
    }

    drv->_last_button = btn;
}

#endif /* EGUI_CONFIG_FUNCTION_ENCODER */
