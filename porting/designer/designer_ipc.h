#ifndef _DESIGNER_IPC_H_
#define _DESIGNER_IPC_H_

#include <stdint.h>
#include <stdbool.h>

/* Command codes (Python -> exe, via stdin) */
#define DESIGNER_CMD_RENDER 0x01
#define DESIGNER_CMD_TOUCH  0x02
#define DESIGNER_CMD_KEY    0x03
#define DESIGNER_CMD_QUIT   0xFF

/* Response codes (exe -> Python, via stdout) */
#define DESIGNER_RSP_READY 0x01
#define DESIGNER_RSP_FRAME 0x02
#define DESIGNER_RSP_ERROR 0xFF

/* Touch actions */
#define DESIGNER_TOUCH_DOWN 0x01
#define DESIGNER_TOUCH_UP   0x02
#define DESIGNER_TOUCH_MOVE 0x03

/* Initialize IPC (set stdin/stdout to binary mode on Windows) */
void designer_ipc_init(void);

/* Send responses to Python */
void designer_ipc_send_ready(void);
void designer_ipc_send_frame(const uint8_t *rgb888_data, uint32_t size);
void designer_ipc_send_error(const char *msg);

/**
 * Read one command from Python. Returns command code, fills payload.
 * Returns 0 on EOF (pipe closed).
 * payload buffer must be at least 16 bytes.
 */
uint8_t designer_ipc_read_cmd(uint8_t *payload, int *payload_len);

#endif /* _DESIGNER_IPC_H_ */
