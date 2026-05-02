#include "game_app.h"

static const hello_game_descriptor_t descriptor = {
        .kind = HELLO_GAME_KIND_BRICK_BREAKER,
        .title = "Brick Breaker",
        .level = 1,
        .tick_ms = 45,
};

const hello_game_descriptor_t *hello_game_get_descriptor(void)
{
    return &descriptor;
}
