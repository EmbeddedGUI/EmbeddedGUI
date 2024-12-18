#include <string.h>
#include <stdio.h>
#include <time.h>
#include <assert.h>
#include <stdbool.h>

#include "uicode.h"
#include "sdl_port.h"


void egui_api_log(const char *format, ...)
{
    va_list argptr;
    va_start(argptr, format);
    vprintf(format, argptr);
    va_end(argptr);
}

void egui_api_assert(const char *file, int line)
{
#if EGUI_CONFIG_DEBUG_LOG_LEVEL >= EGUI_LOG_IMPL_LEVEL_DBG
    char s_buf[0x200];
    memset(s_buf, 0, sizeof(s_buf));
    sprintf(s_buf, "vvvvvvvvvvvvvvvvvvvvvvvvvvvv\n\nAssert@ file = %s, line = %d\n\n^^^^^^^^^^^^^^^^^^^^^^^^^^^^\n", file, line);
    printf("%s", s_buf);
#endif

    while(1);
}

void egui_api_free(void *ptr)
{
    free(ptr);
}

void *egui_api_malloc(int size)
{
    return malloc(size);
}

void egui_api_sprintf(char *str, const char *format, ...)
{
    va_list argptr;
    va_start(argptr, format);
    vsprintf(str, format, argptr);
    va_end(argptr);
}

void egui_api_draw_data(int16_t x, int16_t y, int16_t width, int16_t height, const egui_color_int_t *data)
{
    // printf("api_draw_data, x: %d, y: %d, width: %d, height: %d\n", x, y, width, height);

    VT_Fill_Multiple_Colors(x, y, x + width - 1, y + height - 1, (egui_color_int_t *)data);
}

void egui_api_refresh_display(void)
{
    // printf("api_refresh_display\n");

    VT_sdl_flush(1);
}

void egui_api_timer_start(uint32_t ms)
{
}

void egui_api_timer_stop(void)
{
}

uint32_t egui_api_timer_get_current(void)
{

    return sdl_get_system_timestamp_ms();
}

void egui_api_delay(uint32_t ms)
{
    sdl_port_sleep(ms);
}


void egui_api_pfb_clear(void *s, int n)
{
    memset(s, 0, n);
}

#if EGUI_CONFIG_FUNCTION_RESOURCE_MANAGER
// long getFileSize(const char *filename) {
//     FILE *file = fopen(filename, "rb");  // 以二进制模式打开文件
//     if (file == NULL) {
//         perror("Error opening file");
//         return -1;  // 返回-1表示打开文件失败
//     }

//     // 移动文件指针到文件末尾
//     if (fseek(file, 0, SEEK_END) != 0) {
//         perror("Error seeking to end of file");
//         fclose(file);
//         return -1;  // 返回-1表示移动失败
//     }

//     // 获取文件大小
//     long fileSize = ftell(file);
//     if (fileSize == -1) {
//         perror("Error telling file position");
//         fclose(file);
//         return -1;  // 返回-1表示获取大小失败
//     }

//     // 关闭文件
//     fclose(file);
//     return fileSize;  // 返回文件大小
// }

void egui_api_load_external_resource(void *dest, const uint32_t res_id, uint32_t start_offset, uint32_t size)
{
    FILE *file;
    uint32_t res_offset = egui_ext_res_id_map[res_id];
    uint32_t res_real_offset = res_offset + start_offset;
    // EGUI_LOG_DBG("api_load_external_resource, res_id: %d, res_offset: %d(0x%08x), start_offset: %d(0x%08x), size: %d, res_real_offset: %d(0x%08x)\n"
    //    , res_id, res_offset, res_offset, start_offset, start_offset, size, res_real_offset, res_real_offset);
 
    // Open file for reading
    extern char* pc_get_input_file_path(void);
    // long file_size = getFileSize(file_path);
    // printf("api_load_external_resource, name: %s, size: %ld\n", file_path, file_size);
    file = fopen(pc_get_input_file_path(), "rb");
    if (file == NULL) {
        EGUI_LOG_ERR("Error opening file\r\n");
        return;
    }

    if (fseek(file, res_real_offset, SEEK_SET) != 0) {
        EGUI_LOG_ERR("Error seeking in file");
        fclose(file);
        return;
    }

    // read data from file
    int read_size = fread(dest, 1, size, file);
    while (read_size != size)
    {
        EGUI_LOG_ERR("Error reading file, read_size: %d, size: %d\r\n", read_size, size);
        
        if (feof(file)) {
            EGUI_LOG_ERR("Reached end of file.\n");
            while(1);
        }
        else if (ferror(file)) {
            EGUI_LOG_ERR("Error reading file.\n");
            while(1);
        }
        fclose(file);
        return;
    }
 
    // close file
    fclose(file);
}
#endif
