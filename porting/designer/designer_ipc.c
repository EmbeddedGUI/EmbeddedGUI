#include "designer_ipc.h"
#include <stdio.h>
#include <string.h>

#ifdef _WIN32
#include <io.h>
#include <fcntl.h>
#endif

void designer_ipc_init(void)
{
#ifdef _WIN32
    /* Set stdin/stdout to binary mode to prevent CR/LF translation */
    _setmode(_fileno(stdin), _O_BINARY);
    _setmode(_fileno(stdout), _O_BINARY);
#endif
    /* Disable stdout buffering for immediate response */
    setvbuf(stdout, NULL, _IONBF, 0);
}

static void write_bytes(const void *data, size_t len)
{
    fwrite(data, 1, len, stdout);
}

static bool read_bytes(void *buf, size_t len)
{
    size_t n = fread(buf, 1, len, stdin);
    return n == len;
}

void designer_ipc_send_ready(void)
{
    uint8_t rsp = DESIGNER_RSP_READY;
    write_bytes(&rsp, 1);
}

void designer_ipc_send_frame(const uint8_t *rgb888_data, uint32_t size)
{
    uint8_t rsp = DESIGNER_RSP_FRAME;
    write_bytes(&rsp, 1);
    write_bytes(&size, 4);
    write_bytes(rgb888_data, size);
}

void designer_ipc_send_error(const char *msg)
{
    uint8_t rsp = DESIGNER_RSP_ERROR;
    uint16_t len = (uint16_t)strlen(msg);
    write_bytes(&rsp, 1);
    write_bytes(&len, 2);
    write_bytes(msg, len);
}

uint8_t designer_ipc_read_cmd(uint8_t *payload, int *payload_len)
{
    uint8_t cmd;
    *payload_len = 0;
    if (!read_bytes(&cmd, 1))
    {
        return 0; /* EOF */
    }

    switch (cmd)
    {
    case DESIGNER_CMD_RENDER:
        *payload_len = 0;
        break;
    case DESIGNER_CMD_TOUCH:
        /* 1B action + 2B x + 2B y */
        if (!read_bytes(payload, 5))
        {
            return 0;
        }
        *payload_len = 5;
        break;
    case DESIGNER_CMD_KEY:
        /* 1B type + 2B code */
        if (!read_bytes(payload, 3))
        {
            return 0;
        }
        *payload_len = 3;
        break;
    case DESIGNER_CMD_QUIT:
        *payload_len = 0;
        break;
    default:
        break;
    }
    return cmd;
}
