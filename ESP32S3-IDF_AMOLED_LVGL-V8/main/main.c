#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/i2c.h"
#include "driver/spi_master.h"
#include "esp_timer.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_panel_ops.h"
#include "esp_err.h"
#include "esp_log.h"

#include "lvgl.h"
#include "lv_demos.h"

#include "esp_lcd_qspi_amoled.h"

static const char *TAG = "example";
static SemaphoreHandle_t lvgl_mux = NULL;

#define LCD_HOST    SPI2_HOST
#define TOUCH_HOST  I2C_NUM_0

#if CONFIG_LV_COLOR_DEPTH == 32
#define LCD_BIT_PER_PIXEL       (24)
#elif CONFIG_LV_COLOR_DEPTH == 16
#define LCD_BIT_PER_PIXEL       (16)
#endif
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////// Please update the following configuration according to your LCD spec //////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#define EXAMPLE_LCD_BK_LIGHT_ON_LEVEL  1
#define EXAMPLE_LCD_BK_LIGHT_OFF_LEVEL !EXAMPLE_LCD_BK_LIGHT_ON_LEVEL
// 鱼鹰光电屏幕验证底板
#define EXAMPLE_PIN_NUM_LCD_CS            (GPIO_NUM_40)
#define EXAMPLE_PIN_NUM_LCD_PCLK          (GPIO_NUM_21) 
#define EXAMPLE_PIN_NUM_LCD_DATA0         (GPIO_NUM_47)
#define EXAMPLE_PIN_NUM_LCD_DATA1         (GPIO_NUM_45)
#define EXAMPLE_PIN_NUM_LCD_DATA2         (GPIO_NUM_38)
#define EXAMPLE_PIN_NUM_LCD_DATA3         (GPIO_NUM_39)
#define EXAMPLE_PIN_NUM_LCD_RST           (GPIO_NUM_41)

#define EXAMPLE_USE_TOUCH               1

#if EXAMPLE_USE_TOUCH
// ESP32S3_AMOLED_触摸
#define EXAMPLE_PIN_NUM_TOUCH_SCL         (GPIO_NUM_2 )
#define EXAMPLE_PIN_NUM_TOUCH_SDA         (GPIO_NUM_1 )
#define EXAMPLE_PIN_NUM_TOUCH_RST         (GPIO_NUM_13)
#define EXAMPLE_PIN_NUM_TOUCH_INT         (GPIO_NUM_14)
#endif

#define CST816_ID   1
#define CST820_ID   2
#define CHSC6417_ID 3

#define EXAMPLE_LVGL_BUF_HEIGHT        (EXAMPLE_LCD_V_RES / 10)
#define EXAMPLE_LVGL_TICK_PERIOD_MS    2
#define EXAMPLE_LVGL_TASK_MAX_DELAY_MS 500
#define EXAMPLE_LVGL_TASK_MIN_DELAY_MS 1
#define EXAMPLE_LVGL_TASK_STACK_SIZE   (4 * 1024)
#define EXAMPLE_LVGL_TASK_PRIORITY     2

// 鱼鹰光电
#define AM196Q410502LK_196_410x502   1
#define AM178Q368448LK_178_368x448   2
#define AM151Q466466LK_151_466x466_C 3
#define AM160Q480480LK_160_480x480_C 4
#define AM201Q240296LK_201_240x296   5
#define AM200Q460460LK_200_460x460   6

// 设置当前屏幕尺寸
#define CURRENT_SCREEN_SIZE AM178Q368448LK_178_368x448 // 在这里选择屏幕尺寸

#if CURRENT_SCREEN_SIZE == AM196Q410502LK_196_410x502
  #define EXAMPLE_LCD_H_RES              410
  #define EXAMPLE_LCD_V_RES              502
  #define EXAMPLE_LCD_X_GAP              22
  #define EXAMPLE_LCD_Y_GAP              0
  #define TOUCH_IC_CONFIG                CST820_ID
  #define AMOLED_QSPI_MAX_PCLK           40 * 1000 * 1000

  static const qspi_amoled_lcd_init_cmd_t lcd_init_cmds[] = {
    {0xFE, (uint8_t []){0x00}, 1, 0},
    {0x11, (uint8_t []){0x00}, 0, 120},// 退出睡眠模式
    {0x35, (uint8_t []){0x00}, 1, 0},// 开启撕裂效果
    {0xFE, (uint8_t []){0x00}, 1, 0},
    {0xC4, (uint8_t []){0x80}, 1, 0},// SPI 模式控制
    // {0x36, (uint8_t []){0x00}, 1, 0},// 设置内存数据访问控制
    {0x3A, (uint8_t []){0x55}, 1, 0},//// 设置像素格式 16位
    {0x53, (uint8_t []){0x20}, 1, 0},// 设置 CTRL 显示1
    {0x63, (uint8_t []){0xFF}, 1, 0},// 设置 HBM 模式下的亮度值
    {0x2A, (uint8_t []){0x00, 0x00, 0x01, 0xBF}, 4, 0},
    {0x2B, (uint8_t []){0x00, 0x00, 0x01, 0x6F}, 4, 0},
    {0x29, (uint8_t []){0x00}, 0, 60},// 打开显示器
    {0x51, (uint8_t []){0xFF}, 1, 0},// 设置正常模式下的亮度值
    {0x58, (uint8_t []){0x07}, 1, 10},// 设置正常模式下的亮度值
  };
#elif CURRENT_SCREEN_SIZE == AM178Q368448LK_178_368x448
  #define EXAMPLE_LCD_H_RES              368
  #define EXAMPLE_LCD_V_RES              448
  #define EXAMPLE_LCD_X_GAP              16
  #define EXAMPLE_LCD_Y_GAP              0
  #define TOUCH_IC_CONFIG                CHSC6417_ID
  #define AMOLED_QSPI_MAX_PCLK           40 * 1000 * 1000
  static const qspi_amoled_lcd_init_cmd_t lcd_init_cmds[] = {
    {0xFE, (uint8_t []){0x00}, 1, 0},
    {0x11, (uint8_t []){0x00}, 0, 120},// 退出睡眠模式
    {0x35, (uint8_t []){0x00}, 1, 0},// 开启撕裂效果
    {0xFE, (uint8_t []){0x00}, 1, 0},
    {0xC4, (uint8_t []){0x80}, 1, 0},// SPI 模式控制
    // {0x36, (uint8_t []){0x00}, 1, 0},// 设置内存数据访问控制
    {0x3A, (uint8_t []){0x55}, 1, 0},//// 设置像素格式 16位
    {0x53, (uint8_t []){0x20}, 1, 0},// 设置 CTRL 显示1
    {0x63, (uint8_t []){0xFF}, 1, 0},// 设置 HBM 模式下的亮度值
    {0x2A, (uint8_t []){0x00, 0x00, 0x01, 0xBF}, 4, 0},
    {0x2B, (uint8_t []){0x00, 0x00, 0x01, 0x6F}, 4, 0},
    {0x29, (uint8_t []){0x00}, 0, 60},// 打开显示器
    {0x51, (uint8_t []){0xA0}, 1, 0},// 设置正常模式下的亮度值
    {0x58, (uint8_t []){0x07}, 1, 10},// 设置正常模式下的亮度值
  };
#elif CURRENT_SCREEN_SIZE == AM151Q466466LK_151_466x466_C
  #define EXAMPLE_LCD_H_RES              466
  #define EXAMPLE_LCD_V_RES              466
  #define EXAMPLE_LCD_X_GAP              6
  #define EXAMPLE_LCD_Y_GAP              0
  #define TOUCH_IC_CONFIG                CST820_ID
  #define AMOLED_QSPI_MAX_PCLK           40 * 1000 * 1000
  static const qspi_amoled_lcd_init_cmd_t lcd_init_cmds[] = {
    {0xFE, (uint8_t []){0x00}, 1, 0},
    {0x11, (uint8_t []){0x00}, 0, 120},// 退出睡眠模式
    {0x35, (uint8_t []){0x00}, 1, 0},// 开启撕裂效果
    {0xFE, (uint8_t []){0x00}, 1, 0},
    {0xC4, (uint8_t []){0x80}, 1, 0},// SPI 模式控制
    // {0x36, (uint8_t []){0x00}, 1, 0},// 设置内存数据访问控制
    {0x3A, (uint8_t []){0x55}, 1, 0},//// 设置像素格式 16位
    {0x53, (uint8_t []){0x20}, 1, 0},// 设置 CTRL 显示1
    {0x63, (uint8_t []){0xFF}, 1, 0},// 设置 HBM 模式下的亮度值
    {0x2A, (uint8_t []){0x00, 0x00, 0x01, 0xBF}, 4, 0},
    {0x2B, (uint8_t []){0x00, 0x00, 0x01, 0x6F}, 4, 0},
    {0x51, (uint8_t []){0xA0}, 1, 0},// 设置正常模式下的亮度值
    {0x58, (uint8_t []){0x07}, 1, 10},// 设置正常模式下的亮度值
  };
#elif CURRENT_SCREEN_SIZE == AM160Q480480LK_160_480x480_C
  #define EXAMPLE_LCD_H_RES              480
  #define EXAMPLE_LCD_V_RES              480
  #define EXAMPLE_LCD_X_GAP              0
  #define EXAMPLE_LCD_Y_GAP              0
  #define TOUCH_IC_CONFIG                CHSC6417_ID
  #define AMOLED_QSPI_MAX_PCLK           40 * 1000 * 1000
  static const qspi_amoled_lcd_init_cmd_t lcd_init_cmds[] = {
    {0xFE, (uint8_t []){0x00}, 1, 0},
    {0xF0, (uint8_t []){0x50}, 1, 0},
    {0xB1, (uint8_t []){0x78,0x70}, 2, 0},
    {0xC4, (uint8_t []){0x80}, 1, 0},
    {0x35, (uint8_t []){0x00}, 1, 0},
    {0x36, (uint8_t []){0x00}, 1, 0},
    {0x3A, (uint8_t []){0x55}, 1, 0},
    {0x53, (uint8_t []){0x20}, 1, 0},
    {0x51, (uint8_t []){0xFF}, 1, 0},
    {0x63, (uint8_t []){0xFF}, 1, 0},
    {0x64, (uint8_t []){0x10}, 1, 0},
    {0x67, (uint8_t []){0x01}, 1, 0},
    {0x68, (uint8_t []){0x31}, 1, 0},
    {0x2A, (uint8_t []){0x00,0x00,0x01,0xdf}, 4, 0},
    {0x2B, (uint8_t []){0x00,0x00,0x01,0xdf}, 4, 0},
    {0x11, (uint8_t []){0x00}, 0, 120},
    {0x29, (uint8_t []){0x00}, 0, 120},
  };
#elif CURRENT_SCREEN_SIZE == AM201Q240296LK_201_240x296
  #define EXAMPLE_LCD_H_RES              240
  #define EXAMPLE_LCD_V_RES              296
  #define EXAMPLE_LCD_X_GAP              0
  #define EXAMPLE_LCD_Y_GAP              0
  #define TOUCH_IC_CONFIG                CST816_ID
  #define AMOLED_QSPI_MAX_PCLK           40 * 1000 * 1000
  static const qspi_amoled_lcd_init_cmd_t lcd_init_cmds[] = {
      {0xFE, (uint8_t []){0x00}, 1, 0},
      {0xC4, (uint8_t []){0x80}, 1, 0},
      {0x35, (uint8_t []){0x00}, 1, 0},
      {0x3A, (uint8_t []){0x55}, 1, 0},
      {0x53, (uint8_t []){0x20}, 1, 0},
      {0x51, (uint8_t []){0xFF}, 1, 0},
      {0x63, (uint8_t []){0xFF}, 1, 0},
      {0x2A, (uint8_t []){0x00,0x00,0x00,0xEF}, 4, 0},
      {0x2B, (uint8_t []){0x00,0x00,0x01,0x27}, 4, 0},
      {0x11, (uint8_t []){0x00}, 0, 80},
      {0x29, (uint8_t []){0x00}, 0, 10},
  };
#elif CURRENT_SCREEN_SIZE == AM200Q460460LK_200_460x460
  #define EXAMPLE_LCD_H_RES              460
  #define EXAMPLE_LCD_V_RES              460
  #define EXAMPLE_LCD_X_GAP              0
  #define EXAMPLE_LCD_Y_GAP              0
  #define TOUCH_IC_CONFIG                CST820_ID
  #define AMOLED_QSPI_MAX_PCLK           40 * 1000 * 1000
  static const qspi_amoled_lcd_init_cmd_t lcd_init_cmds[] = {
    {0xFE, (uint8_t []){0x00}, 1, 0},
    {0x11, (uint8_t []){0x00}, 0, 120},// 退出睡眠模式
    {0x35, (uint8_t []){0x00}, 1, 0},// 开启撕裂效果
    {0xFE, (uint8_t []){0x00}, 1, 0},
    {0xC4, (uint8_t []){0x80}, 1, 0},// SPI 模式控制
    // {0x36, (uint8_t []){0x00}, 1, 0},// 设置内存数据访问控制
    {0x3A, (uint8_t []){0x55}, 1, 0},//// 设置像素格式 16位
    {0x53, (uint8_t []){0x20}, 1, 0},// 设置 CTRL 显示1
    {0x63, (uint8_t []){0xFF}, 1, 0},// 设置 HBM 模式下的亮度值
    {0x2A, (uint8_t []){0x00, 0x00, 0x01, 0xBF}, 4, 0},
    {0x2B, (uint8_t []){0x00, 0x00, 0x01, 0x6F}, 4, 0},
    {0x29, (uint8_t []){0x00}, 0, 60},// 打开显示器
    {0x51, (uint8_t []){0xFF}, 1, 0},// 设置正常模式下的亮度值
    {0x58, (uint8_t []){0x07}, 1, 10},// 设置正常模式下的亮度值
  };
#else
  #error "Unsupported screen size"
#endif

// 根据TOUCH_IC_CONFIG的配置，选择调用的头文件
#if EXAMPLE_USE_TOUCH

#if TOUCH_IC_CONFIG == CST816_ID
#include "esp_lcd_touch_cst816.h"
#define TOUCH_IO_I2C_CONFIG    ESP_LCD_TOUCH_IO_I2C_CST816_CONFIG
#define esp_lcd_touch_new_i2c  esp_lcd_touch_new_i2c_cst816

#elif TOUCH_IC_CONFIG == CST820_ID
#include "esp_lcd_touch_cst820.h"
#define TOUCH_IO_I2C_CONFIG    ESP_LCD_TOUCH_IO_I2C_CST820_CONFIG  
#define esp_lcd_touch_new_i2c  esp_lcd_touch_new_i2c_cst820

#elif TOUCH_IC_CONFIG == CHSC6417_ID
#include "esp_lcd_touch_chsc6417.h"
#define TOUCH_IO_I2C_CONFIG    ESP_LCD_TOUCH_IO_I2C_CHSC6417_CONFIG
#define esp_lcd_touch_new_i2c  esp_lcd_touch_new_i2c_chsc6417
#endif

esp_lcd_touch_handle_t tp = NULL;

#endif

static bool example_notify_lvgl_flush_ready(esp_lcd_panel_io_handle_t panel_io, esp_lcd_panel_io_event_data_t *edata, void *user_ctx)
{
    lv_disp_drv_t *disp_driver = (lv_disp_drv_t *)user_ctx;
    lv_disp_flush_ready(disp_driver);
    return false;
}

static void example_lvgl_flush_cb(lv_disp_drv_t *drv, const lv_area_t *area, lv_color_t *color_map)
{
    esp_lcd_panel_handle_t panel_handle = (esp_lcd_panel_handle_t) drv->user_data;
    const int offsetx1 = area->x1;
    const int offsetx2 = area->x2;
    const int offsety1 = area->y1;
    const int offsety2 = area->y2;

#if LCD_BIT_PER_PIXEL == 24
    uint8_t *to = (uint8_t *)color_map;
    uint8_t temp = 0;
    uint16_t pixel_num = (offsetx2 - offsetx1 + 1) * (offsety2 - offsety1 + 1);

    // Special dealing for first pixel
    temp = color_map[0].ch.blue;
    *to++ = color_map[0].ch.red;
    *to++ = color_map[0].ch.green;
    *to++ = temp;
    // Normal dealing for other pixels
    for (int i = 1; i < pixel_num; i++) {
        *to++ = color_map[i].ch.red;
        *to++ = color_map[i].ch.green;
        *to++ = color_map[i].ch.blue;
    }
#endif

    // copy a buffer's content to a specific area of the display
    esp_lcd_panel_draw_bitmap(panel_handle, offsetx1, offsety1, offsetx2 + 1, offsety2 + 1, color_map);
}

void example_lvgl_rounder_cb(struct _lv_disp_drv_t *disp_drv, lv_area_t *area)
{
    uint16_t x1 = area->x1;
    uint16_t x2 = area->x2;

    uint16_t y1 = area->y1;
    uint16_t y2 = area->y2;

    // round the start of coordinate down to the nearest 2M number
    area->x1 = (x1 >> 1) << 1;
    area->y1 = (y1 >> 1) << 1;
    // round the end of coordinate up to the nearest 2N+1 number
    area->x2 = ((x2 >> 1) << 1) + 1;
    area->y2 = ((y2 >> 1) << 1) + 1;
}

#if EXAMPLE_USE_TOUCH
static void example_lvgl_touch_cb(lv_indev_drv_t *drv, lv_indev_data_t *data)
{
    esp_lcd_touch_handle_t tp = (esp_lcd_touch_handle_t)drv->user_data;
    assert(tp);

    uint16_t tp_x;
    uint16_t tp_y;
    uint8_t tp_cnt = 0;
    /* Read data from touch controller into memory */
    esp_lcd_touch_read_data(tp);
    /* Read data from touch controller */
    bool tp_pressed = esp_lcd_touch_get_coordinates(tp, &tp_x, &tp_y, NULL, &tp_cnt, 1);
    if (tp_pressed && tp_cnt > 0) {
        data->point.x = tp_x;
        data->point.y = tp_y;
        data->state = LV_INDEV_STATE_PRESSED;
        ESP_LOGI(TAG, "Touch position: %d,%d,%d", tp_cnt, tp_x, tp_y);
    } else {
        data->state = LV_INDEV_STATE_RELEASED;
    }

}
#endif

static void example_increase_lvgl_tick(void *arg)
{
    /* Tell LVGL how many milliseconds has elapsed */
    lv_tick_inc(EXAMPLE_LVGL_TICK_PERIOD_MS);
}

static bool example_lvgl_lock(int timeout_ms)
{
    assert(lvgl_mux && "bsp_display_start must be called first");

    const TickType_t timeout_ticks = (timeout_ms == -1) ? portMAX_DELAY : pdMS_TO_TICKS(timeout_ms);
    return xSemaphoreTake(lvgl_mux, timeout_ticks) == pdTRUE;
}

static void example_lvgl_unlock(void)
{
    assert(lvgl_mux && "bsp_display_start must be called first");
    xSemaphoreGive(lvgl_mux);
}

static void example_lvgl_port_task(void *arg)
{
    ESP_LOGI(TAG, "Starting LVGL task");
    uint32_t task_delay_ms = EXAMPLE_LVGL_TASK_MAX_DELAY_MS;
    while (1) {
        // Lock the mutex due to the LVGL APIs are not thread-safe
        if (example_lvgl_lock(-1)) {
            task_delay_ms = lv_timer_handler();
            // Release the mutex
            example_lvgl_unlock();
        }
        if (task_delay_ms > EXAMPLE_LVGL_TASK_MAX_DELAY_MS) {
            task_delay_ms = EXAMPLE_LVGL_TASK_MAX_DELAY_MS;
        } else if (task_delay_ms < EXAMPLE_LVGL_TASK_MIN_DELAY_MS) {
            task_delay_ms = EXAMPLE_LVGL_TASK_MIN_DELAY_MS;
        }
        vTaskDelay(pdMS_TO_TICKS(task_delay_ms));
    }
}

static void i2c_scan(void)
{
    ESP_LOGI(TAG, "Scanning I2C bus...");
    for (uint8_t addr = 1; addr < 127; addr++) {
        i2c_cmd_handle_t cmd = i2c_cmd_link_create();
        i2c_master_start(cmd);
        i2c_master_write_byte(cmd, (addr << 1) | I2C_MASTER_WRITE, true);
        i2c_master_stop(cmd);
        esp_err_t ret = i2c_master_cmd_begin(I2C_NUM_0, cmd, 1000 / portTICK_PERIOD_MS);
        i2c_cmd_link_delete(cmd);

        if (ret == ESP_OK) {
            ESP_LOGI(TAG, "Found I2C device at address 0x%02x", addr);
        }
    }
    ESP_LOGI(TAG, "I2C scan completed.");
}

void app_main(void)
{
    static lv_disp_draw_buf_t disp_buf; // contains internal graphic buffer(s) called draw buffer(s)
    static lv_disp_drv_t disp_drv;      // contains callback functions

    ESP_LOGI(TAG, "Initialize SPI bus");
    const spi_bus_config_t buscfg = QSPI_AMOLED_PANEL_BUS_QSPI_CONFIG(EXAMPLE_PIN_NUM_LCD_PCLK,
                                                                 EXAMPLE_PIN_NUM_LCD_DATA0,
                                                                 EXAMPLE_PIN_NUM_LCD_DATA1,
                                                                 EXAMPLE_PIN_NUM_LCD_DATA2,
                                                                 EXAMPLE_PIN_NUM_LCD_DATA3,
                                                                 EXAMPLE_LCD_H_RES * EXAMPLE_LCD_V_RES * LCD_BIT_PER_PIXEL / 8);
    ESP_ERROR_CHECK(spi_bus_initialize(LCD_HOST, &buscfg, SPI_DMA_CH_AUTO));

    ESP_LOGI(TAG, "Install panel IO");
    esp_lcd_panel_io_handle_t io_handle = NULL;
    esp_lcd_panel_io_spi_config_t io_config = QSPI_AMOLED_PANEL_IO_QSPI_CONFIG(EXAMPLE_PIN_NUM_LCD_CS,
                                                                                example_notify_lvgl_flush_ready,
                                                                                &disp_drv);

    io_config.pclk_hz = (unsigned int)AMOLED_QSPI_MAX_PCLK;
    qspi_amoled_vendor_config_t vendor_config = {
        .init_cmds = lcd_init_cmds,
        .init_cmds_size = sizeof(lcd_init_cmds) / sizeof(lcd_init_cmds[0]),
        .flags = {
            .use_qspi_interface = 1,
        },
    };
    // Attach the LCD to the SPI bus
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)LCD_HOST, &io_config, &io_handle));

    esp_lcd_panel_handle_t panel_handle = NULL;
    const esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = EXAMPLE_PIN_NUM_LCD_RST,
        .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_RGB,
        .bits_per_pixel = LCD_BIT_PER_PIXEL,
        .vendor_config = &vendor_config,
    };
    ESP_LOGI(TAG, "Install QSPI_AMOLED panel driver");
    ESP_ERROR_CHECK(esp_lcd_new_panel_qspi_amoled(io_handle, &panel_config, &panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_reset(panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_init(panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_set_gap(panel_handle, EXAMPLE_LCD_X_GAP, EXAMPLE_LCD_Y_GAP));
    // 在打开屏幕或背光之前，用户可以将预定义的图案刷新到屏幕上
    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel_handle, true));
    // 设置屏幕亮度
    ESP_ERROR_CHECK(panel_qspi_amoled_set_brightness(panel_handle, 0xFF)); // 设置亮度为 15

#if EXAMPLE_USE_TOUCH
    ESP_LOGI(TAG, "Initialize I2C bus");
    const i2c_config_t i2c_conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = EXAMPLE_PIN_NUM_TOUCH_SDA,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_io_num = EXAMPLE_PIN_NUM_TOUCH_SCL,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = 200 * 1000,
    };
    ESP_ERROR_CHECK(i2c_param_config(TOUCH_HOST, &i2c_conf));
    // 设置 I2C 超时时间
    ESP_ERROR_CHECK(i2c_set_timeout(TOUCH_HOST, 0x02)); // 0x02: 2ms
    ESP_ERROR_CHECK(i2c_driver_install(TOUCH_HOST, i2c_conf.mode, 0, 0, 0));

    // Scan I2C devices
    i2c_scan();

    esp_lcd_panel_io_handle_t tp_io_handle = NULL;
    // const esp_lcd_panel_io_i2c_config_t tp_io_config = ESP_LCD_TOUCH_IO_I2C_CST820_CONFIG();
    const esp_lcd_panel_io_i2c_config_t tp_io_config = TOUCH_IO_I2C_CONFIG();
    // Attach the TOUCH to the I2C bus
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_i2c((esp_lcd_i2c_bus_handle_t)TOUCH_HOST, &tp_io_config, &tp_io_handle));

    const esp_lcd_touch_config_t tp_cfg = {
        .x_max = EXAMPLE_LCD_H_RES,
        .y_max = EXAMPLE_LCD_V_RES,
        .rst_gpio_num = EXAMPLE_PIN_NUM_TOUCH_RST,
        .int_gpio_num = EXAMPLE_PIN_NUM_TOUCH_INT,
        .levels = {
            .reset = 0,
            .interrupt = 0,
        },
        .flags = {
            .swap_xy = 0,
            .mirror_x = 0,
            .mirror_y = 0,
        },
    };

    ESP_LOGI(TAG, "Initialize touch controller");
    // ESP_ERROR_CHECK(esp_lcd_touch_new_i2c_cst820(tp_io_handle, &tp_cfg, &tp));
    ESP_ERROR_CHECK(esp_lcd_touch_new_i2c(tp_io_handle, &tp_cfg, &tp));
#endif

    ESP_LOGI(TAG, "Initialize LVGL library");
    lv_init();
    // alloc draw buffers used by LVGL
    // it's recommended to choose the size of the draw buffer(s) to be at least 1/10 screen sized
    lv_color_t *buf1 = heap_caps_malloc(EXAMPLE_LCD_H_RES * EXAMPLE_LVGL_BUF_HEIGHT * sizeof(lv_color_t), MALLOC_CAP_DMA);
    assert(buf1);
    lv_color_t *buf2 = heap_caps_malloc(EXAMPLE_LCD_H_RES * EXAMPLE_LVGL_BUF_HEIGHT * sizeof(lv_color_t), MALLOC_CAP_DMA);
    assert(buf2);
    // initialize LVGL draw buffers
    lv_disp_draw_buf_init(&disp_buf, buf1, buf2, EXAMPLE_LCD_H_RES * EXAMPLE_LVGL_BUF_HEIGHT);

    ESP_LOGI(TAG, "Register display driver to LVGL");
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = EXAMPLE_LCD_H_RES;
    disp_drv.ver_res = EXAMPLE_LCD_V_RES;
    disp_drv.flush_cb = example_lvgl_flush_cb;
    disp_drv.rounder_cb = example_lvgl_rounder_cb;
    disp_drv.draw_buf = &disp_buf;
    disp_drv.user_data = panel_handle;

    // 设置旋转角度
    // disp_drv.rotated = LV_DISP_ROT_90; // 旋转90度
    
    lv_disp_t *disp = lv_disp_drv_register(&disp_drv);

    ESP_LOGI(TAG, "Install LVGL tick timer");
    // Tick interface for LVGL (using esp_timer to generate 2ms periodic event)
    const esp_timer_create_args_t lvgl_tick_timer_args = {
        .callback = &example_increase_lvgl_tick,
        .name = "lvgl_tick"
    };
    esp_timer_handle_t lvgl_tick_timer = NULL;
    ESP_ERROR_CHECK(esp_timer_create(&lvgl_tick_timer_args, &lvgl_tick_timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(lvgl_tick_timer, EXAMPLE_LVGL_TICK_PERIOD_MS * 1000));

#if EXAMPLE_USE_TOUCH
    static lv_indev_drv_t indev_drv;    // Input device driver (Touch)
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.disp = disp;
    indev_drv.read_cb = example_lvgl_touch_cb;
    indev_drv.user_data = tp;
    lv_indev_drv_register(&indev_drv);
#endif

    lvgl_mux = xSemaphoreCreateMutex();
    assert(lvgl_mux);
    xTaskCreate(example_lvgl_port_task, "LVGL", EXAMPLE_LVGL_TASK_STACK_SIZE, NULL, EXAMPLE_LVGL_TASK_PRIORITY, NULL);

    ESP_LOGI(TAG, "Display LVGL demos");
    // Lock the mutex due to the LVGL APIs are not thread-safe
    if (example_lvgl_lock(-1)) {

        lv_demo_widgets();      /* A widgets example */
        // lv_demo_music();        /* A modern, smartphone-like music player demo. */
        // lv_demo_stress();       /* A stress test for LVGL. */
        // lv_demo_benchmark();    /* A demo to measure the performance of LVGL or to compare different settings. */

        // Release the mutex
        example_lvgl_unlock();
    }
}

