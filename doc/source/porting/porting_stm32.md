# STM32 绉绘

鏈枃妗ｄ粙缁?EmbeddedGUI 鍦?STM32G0 绯诲垪 MCU 涓婄殑绉绘瀹炵幇锛屼娇鐢?SPI 鎺ュ彛椹卞姩 ST7789 LCD 灞忓箷锛孎T6336 瑙︽懜鎺у埗鍣ㄣ€?

## 纭欢閰嶇疆

鍙傝€冨钩鍙帮細STM32G0B0RE

| 缁勪欢 | 鍨嬪彿/鎺ュ彛 | 璇存槑 |
|------|----------|------|
| MCU | STM32G0B0RE | Cortex-M0+, 64MHz, 128KB Flash, 144KB RAM |
| LCD | ST7789 | 240x320, RGB565, SPI 鎺ュ彛 |
| 瑙︽懜 | FT6336 | 鐢靛瑙︽懜, I2C 鎺ュ彛 |

## 鏂囦欢缁撴瀯

```
porting/stm32g0/
鈹溾攢鈹€ Porting/
鈹?  鈹溾攢鈹€ egui_port_mcu.c    # Display/Platform Driver 娉ㄥ唽
鈹?  鈹溾攢鈹€ port_main.c        # 涓诲惊鐜叆鍙?
鈹?  鈹溾攢鈹€ port_main.h        # 涓诲惊鐜ご鏂囦欢
鈹?  鈹溾攢鈹€ app_lcd.c          # LCD 搴旂敤灞傦紙瑙︽懜杞銆丏MA 鍥炶皟锛?
鈹?  鈹溾攢鈹€ app_lcd.h          # LCD 搴旂敤灞傚ご鏂囦欢
鈹?  鈹溾攢鈹€ lcd_st7789.c       # ST7789 SPI 椹卞姩
鈹?  鈹溾攢鈹€ lcd_st7789.h       # ST7789 椹卞姩澶存枃浠?
鈹?  鈹溾攢鈹€ tc_ft6336.c        # FT6336 瑙︽懜椹卞姩
鈹?  鈹斺攢鈹€ tc_ft6336.h        # FT6336 椹卞姩澶存枃浠?
鈹溾攢鈹€ GCC/
鈹?  鈹斺攢鈹€ Makefile.base      # GCC 鏋勫缓瑙勫垯
鈹溾攢鈹€ STM32G0B0RETX_FLASH.ld # 閾炬帴鑴氭湰
鈹溾攢鈹€ build.mk               # 鏋勫缓妯″潡瀹氫箟
鈹斺攢鈹€ app_egui_config.h      # 閰嶇疆瑕嗙洊锛堝湪 example 鐩綍涓嬶級
```

## SPI 灞忓箷椹卞姩

### Display Driver 娉ㄥ唽

`egui_port_mcu.c` 涓敞鍐?Display Driver锛?

```c
static void port_display_set_brightness(egui_core_t *core, uint8_t level)
{
    EGUI_UNUSED(core);
    egui_hal_stm32g0_set_backlight_level(level);
}

static void port_display_set_rotation(egui_core_t *core, egui_display_rotation_t rotation)
{
    egui_hal_lcd_driver_t *lcd = egui_hal_lcd_get(core);
    if (lcd == NULL)
    {
        return;
    }

    switch (rotation)
    {
    case EGUI_DISPLAY_ROTATION_0:
        if (lcd->mirror) lcd->mirror(lcd, 0, 0);
        if (lcd->swap_xy) lcd->swap_xy(lcd, 0);
        break;
    case EGUI_DISPLAY_ROTATION_90:
        if (lcd->mirror) lcd->mirror(lcd, 1, 0);
        if (lcd->swap_xy) lcd->swap_xy(lcd, 1);
        break;
    case EGUI_DISPLAY_ROTATION_180:
        if (lcd->mirror) lcd->mirror(lcd, 1, 1);
        if (lcd->swap_xy) lcd->swap_xy(lcd, 0);
        break;
    case EGUI_DISPLAY_ROTATION_270:
        if (lcd->mirror) lcd->mirror(lcd, 0, 1);
        if (lcd->swap_xy) lcd->swap_xy(lcd, 1);
        break;
    }
}
```

褰撳墠 `stm32g0` 绀轰緥鍏ュ彛鐨勫疄闄呴『搴忔槸锛歚egui_init(&core, egui_pfb)` 鈫?`egui_port_init(&core)` 鈫?`egui_display_driver_register()` 鈫?鍙€?`egui_port_register_touch_driver()` 鈫?`uicode_disp0_init()` 鈫?`egui_screen_on(&core)`銆傝嫢瑕佹柊寤哄灞忔垨寮傛瀯鍒嗚鲸鐜?port锛屽缓璁敼鐢?`egui_display_setup_t + egui_setup_display()` 鐨勭粺涓€鍏ュ彛銆?
褰撳墠 `stm32g0` port 涓嶅啀鑷繁瀹炵幇涓€灞?`draw_area(core, ...)` 杞彂锛涘儚绱犲啓鍏ユˉ鎺ョ敱 `egui_hal_lcd_register()` 鑷姩瀹屾垚锛屾澘绾т唬鐮佷富瑕佽ˉ鍏呬寒搴︺€佹棆杞瓑鑳藉姏銆?
鍚屾椂鏀寔鍚屾鍜屽紓姝ワ紙DMA锛変袱绉嶄紶杈撴ā寮忋€?

### ST7789 SPI 閫氫俊

鍏稿瀷鐨?SPI LCD 鍐欏叆娴佺▼锛?

1. 璁剧疆鍒楀湴鍧€鑼冨洿锛圕ASET 鍛戒护锛?
2. 璁剧疆琛屽湴鍧€鑼冨洿锛圧ASET 鍛戒护锛?
3. 鍙戦€佸啓鍐呭瓨鍛戒护锛圧AMWR锛?
4. 閫氳繃 SPI 鍙戦€佸儚绱犳暟鎹?

## SysTick 瀹氭椂鍣?

Platform Driver 浣跨敤 HAL 搴撶殑 SysTick 鎻愪緵姣鏃堕棿鎴筹細

```c
static uint32_t mcu_get_tick_ms(void)
{
    return HAL_GetTick();
}

static void mcu_delay(uint32_t ms)
{
    HAL_Delay(ms);
}
```

`HAL_GetTick()` 鍩轰簬 SysTick 涓柇锛屾瘡 1ms 閫掑涓€娆★紝涓哄姩鐢汇€佸畾鏃跺櫒鍜岃緭鍏ヨ秴鏃舵彁渚涙椂闂村熀鍑嗐€?

## DMA 寮傛浼犺緭

### 鍩烘湰鍘熺悊

SPI DMA 浼犺緭鍏佽 CPU 鍦ㄦ暟鎹紶杈撴湡闂寸户缁墽琛屽叾浠栦换鍔★紙濡傛覆鏌撲笅涓€涓?PFB 鍧楋級銆?

### PFB 缂撳啿鍖烘竻闆跺姞閫?

閫氳繃 `EGUI_CONFIG_PLATFORM_CUSTOM_MEMORY_OP=1` 娉ㄥ唽 `memset_fast` 鍚庯紝
鍙互鎶婂唴閮?`egui_api_memset()` / `egui_api_pfb_clear()` 鐨勯浂濉厖鍒嗘敮鎺ュ埌 DMA锛?

```c
#if APP_EGUI_CONFIG_USE_DMA_TO_RESET_PFB_BUFFER
extern DMA_HandleTypeDef hdma_memtomem_dma2_channel5;
const egui_color_int_t fixed_0_buffer[EGUI_CONFIG_PFB_WIDTH * EGUI_CONFIG_PFB_HEIGHT] = {0};

static void mcu_memset_fast(void *s, int c, int n)
{
    if (c == 0)
    {
        HAL_DMA_Start(&hdma_memtomem_dma2_channel5, (uint32_t)fixed_0_buffer, (uint32_t)s, n >> 2);
        HAL_DMA_PollForTransfer(&hdma_memtomem_dma2_channel5, HAL_DMA_FULL_TRANSFER, 1000);
        return;
    }

    memset(s, c, n);
}
#endif
```

杩欐牱鍙互淇濊瘉锛?

- `c == 0` 鏃惰蛋 DMA 娓呴浂锛屾敹鐩婃渶澶?
- 闈為浂濉厖鍊兼椂鍥為€€鍒版爣鍑?`memset`锛岃涔変繚鎸佸畬鏁?

### 鍙岀紦鍐?+ DMA

`app_lcd.c` 瀹炵幇浜嗗弻缂撳啿鏈哄埗锛屽湪 DMA 浼犺緭褰撳墠 PFB 鏃跺垏鎹㈠埌澶囩敤缂撳啿鍖猴細

```c
EGUI_CONFIG_PFB_BUFFER_DECLARE(egui_pfb);
static egui_core_t core;

void port_main(void)
{
    egui_init(&core, egui_pfb);
    // egui_init() 浼氭妸缂栬瘧鏈熷０鏄庣殑鎵€鏈?PFB buffer 涓€娆℃€т氦缁?core
}
```

褰撳墠澶氱紦鍐插凡缁忕敱 core 鍐呴儴鐨?`egui_pfb_manager_t` 缁熶竴绠＄悊锛屼笉鍐嶉€氳繃搴旂敤灞傛墜鍔ㄥ垏鎹⑩€滀富 / 澶囦唤 PFB 鎸囬拡鈥濄€?
SPI DMA 浼犺緭瀹屾垚涓柇鍥炶皟锛?

```c
void HAL_SPI_TxCpltCallback(SPI_HandleTypeDef *hspi)
{
    if (hspi->Instance == ST7789_SPI_PORT.Instance)
    {
        egui_pfb_notify_flush_complete(&core);
    }
}
```

### 澶氱紦鍐茬幆褰㈤槦鍒?

`port_main.c` 鏀寔鏈€澶?4 涓?PFB 缂撳啿鍖虹殑鐜舰闃熷垪锛?

```c
EGUI_CONFIG_PFB_BUFFER_DECLARE(egui_pfb);
static egui_core_t core;

void port_main(void)
{
    egui_init(&core, egui_pfb);
    // egui_init() 浼氭妸缂栬瘧鏈熷０鏄庣殑鎵€鏈?PFB buffer 涓€娆℃€т氦缁?core
}
```

## 瑙︽懜杈撳叆

### FT6336 瑙︽懜椹卞姩

閫氳繃 I2C 璇诲彇瑙︽懜鍧愭爣锛屽湪 `app_lcd.c` 涓疆璇㈠鐞嗭細

```c
static egui_hal_touch_driver_t s_touch_driver;
void egui_port_register_touch_driver(egui_core_t *core)
{
    egui_hal_touch_config_t touch_config = {
        .width = EGUI_CONFIG_SCEEN_WIDTH,
        .height = EGUI_CONFIG_SCEEN_HEIGHT,
        .swap_xy = 0,
        .mirror_x = 0,
        .mirror_y = 0,
    };

    egui_hal_touch_register(core, &s_touch_driver, &touch_config);
}
```

当前实现说明：

FT6336 鐨?INT / RST 绠¤剼浠嶇敱搴曞眰 port 鎻愪緵锛屼絾瑙︽懜涓婃姤宸茬粡鐢?HAL touch driver 缁熶竴澶勭悊锛屼笉鍐嶉渶瑕佸簲鐢ㄥ眰鑷繁杞骞惰浆鎴?motion event銆?
## 涓诲惊鐜?

```c
void port_main(void)
{
    egui_init(&core, egui_pfb);
    egui_port_init(&core);
    egui_display_driver_register(&core, egui_port_get_display_driver());
#if EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH
    egui_port_register_touch_driver(&core);
#endif
    uicode_disp0_init(&core);
    egui_screen_on(&core);

    while (1)
    {
        egui_polling_work(&core);
    }
}
```

## 鍏稿瀷璧勬簮鍗犵敤

鍩轰簬 STM32G0B0RE锛孯GB565锛孭FB 30x40 鐨勫吀鍨嬪崰鐢細

| 椤圭洰 | 澶у皬 |
|------|------|
| 鏍稿績妗嗘灦浠ｇ爜 | ~5-8KB Flash |
| PFB 缂撳啿鍖?| 2,400B RAM锛堝崟缂撳啿锛?|
| 姣忎釜鎺т欢瀹炰緥 | ~50-200B RAM |
| 瀛椾綋璧勬簮锛?6px, 4-bit锛?| ~2-10KB Flash锛堝彇鍐充簬瀛楃鏁帮級 |
| 鍥剧墖璧勬簮 | 鍙栧喅浜庡昂瀵稿拰鏍煎紡 |

## 鏋勫缓鍛戒护

```bash
# 浣跨敤 GCC 宸ュ叿閾炬瀯寤?
make all APP=HelloSimple PORT=stm32g0

# 浣跨敤绌哄钩鍙帮紙浠呭垎鏋愬ぇ灏忥紝涓嶅惈纭欢椹卞姩锛?
# 鎵撳紑 Keil 宸ョ▼
# porting/stm32g0/MDK-ARM/proj_stm32g0.uvprojx
```

## 涓柇浼樺厛绾?

寤鸿鐨勪腑鏂紭鍏堢骇閰嶇疆锛?

| 涓柇 | 浼樺厛绾?| 璇存槑 |
|------|--------|------|
| SysTick | 鏈€楂?| 淇濊瘉鏃堕棿鎴冲噯纭?|
| SPI DMA | 涓?| DMA 浼犺緭瀹屾垚鍥炶皟 |
| GPIO EXTI锛堣Е鎽革級 | 浣?| 瑙︽懜涓柇涓嶉渶瑕佸疄鏃跺搷搴?|
