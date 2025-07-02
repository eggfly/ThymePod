#include <Arduino_GFX_Library.h>


// #define AD35_S3
// #define BLOCKCODELAB_ARCADE_LITE
// #define DLC35010R // or called "Elecrow ESP Terminal with 3.5inch Parallel RGB Capacitive Touch Display (ILI9488)"
// #define DRAGON_RADAR
// #define ESP32_1732S019
// #define ESP32_2424012
// #define ESP32_2432S028
// #define ESP32_3248S035
// #define ESP32_4827A043 /* requires manual set bus speed to 8 mHz: gfx->begin(8000000) */
// #define ESP32_4827A043_QSPI
// #define ESP32_4827S043
// #define ESP32_4848S040_86BOX_GUITION
// #define ESP32_8048S043
// #define ESP32_8048S070
// #define ESP32_LCDKIT_SPI
// #define ESP32_LCDKIT_PAR8A
// #define ESP32_LCDKIT_PAR8B
// #define ESP32_LCDKIT_PAR16
// #define ESP32_S3_313LCD
// #define ESP32_S3_BOX_3
// #define ESP32_S3_EYE
// #define ESP32_S3_RGB
// #define ESP32_S3_RPI_DPI
// #define ESP32S3_2_1_TP
// #define GS_T3E // Golden Security GS-T3E
// #define IBUBLY
// #define JC1060P470
// #define JC3248W535
// #define JC3636W518
// #define JC8012P4A1
// #define LILYGO_T_DECK
// #define LILYGO_T_DECK_PLUS
// #define LILYGO_T_DISPLAY
// #define LILYGO_T_DISPLAY_S3
// #define LILYGO_T_Display_S3_AMOLED
// #define LILYGO_T_Display_S3_AMOLED_1_64
// #define LILYGO_T_Display_S3_LONG
// #define LILYGO_T_DISPLAY_S3_PRO
// #define LILYGO_T_QT
// #define LILYGO_T_RGB
// #define LILYGO_T_TRACK
// #define LILYGO_T_WATCH_2021
// #define LILYGO_T4_S3
// #define MAKERFABS_TFT_TOUCH_3_5
// #define MAKERFABS_ESP32_S3_TFT_4_0
// #define MAKERFABS_ESP32_S3_TFT_4_3_v1_3
// #define XIAO_SAMD21_ROUND_DISPLAY
// #define XIAO_ESP32C3_ROUND_DISPLAY
// #define XIAO_ESP32S3_ROUND_DISPLAY
// #define WAVESHARE_ESP32_C6_LCD_1_47
// #define WAVESHARE_ESP32_C6_LCD_1_9
// #define WAVESHARE_ESP32_S3_LCD_1_3
// #define WAVESHARE_ESP32_S3_LCD_1_3_PRISM
// #define WAVESHARE_ESP32_S3_LCD_1_46
// #define WAVESHARE_ESP32_S3_LCD_2_8
// #define WAVESHARE_ESP32_S3_TOUCH_AMOLED_1_64
// #define WAVESHARE_ESP32_S3_TOUCH_AMOLED_2_41
// #define WAVESHARE_RP2040_LCD_0_96
// #define WAVESHARE_RP2040_LCD_1_28 // Waveshare RP2040-LCD-1.28 or RP2350-LCD-1.28
// #define WT32_SC01
// #define WZ8048C050 // or called "Elecrow Wizee-ESP32"
// #define ZX2D10GE10R_V4848
// #define ZX3D50CE02S // or called "WT32-SC01 PLUS"
// #define ZX3D95CE01S_AR
// #define ZX3D95CE01S_TR
// #define ZX7D00CE01S // or called "QM Smart Panlee 7.0 inch serial screen"
#define AM200Q460460LK

#define GFX_DEV_DEVICE AM200Q460460LK
#define SCREEN_WIDTH 460
#define SCREEN_HEIGHT 460


#define AMOLED_CS 3
#define AMOLED_SCLK 15
#define AMOLED_D0 16
#define AMOLED_D1 17
#define AMOLED_D2 18
#define AMOLED_D3 8
#define AMOLED_RST 46


// Arduino_DataBus *bus = new Arduino_ESP32QSPI(
//     8 /* CS */, 7 /* SCK */, 15 /* D0 */, 16 /* D1 */, 17 /* D2 */, 18 /* D3 */);

Arduino_DataBus *bus = new Arduino_ESP32QSPI(
    AMOLED_CS /* CS */, AMOLED_SCLK /* SCK */, AMOLED_D0 /* D0 */, AMOLED_D1 /* D1 */, AMOLED_D2 /* D2 */, AMOLED_D3 /* D3 */);

Arduino_CO5300 *gfx = new Arduino_CO5300(bus, AMOLED_RST /* RST */, 0 /* rotation */, false /* IPS */, SCREEN_WIDTH, SCREEN_HEIGHT,
                                    0 /* col offset 1 */, 0 /* row offset 1 */, 0 /* col_offset2 */, 0 /* row_offset2 */);
// #define CANVAS
// Arduino_Canvas *gfx = new Arduino_Canvas(SCREEN_WIDTH /* width */, SCREEN_HEIGHT /* height */, g, 0 /* output_x */, 0 /* output_y */, 0 /* rotation */);

