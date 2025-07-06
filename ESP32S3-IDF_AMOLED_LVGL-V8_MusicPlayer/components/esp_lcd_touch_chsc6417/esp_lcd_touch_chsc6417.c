/*
 * SPDX-FileCopyrightText: 2015-2023 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_system.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_check.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_touch.h"

#include "esp_lcd_touch_chsc6417.h"

#define POINT_NUM_MAX       (1)

#define DATA_START_REG      (0x00)
#define CHIP_ID_REG         (0xA7)

static const char *TAG = "CHSC6417";

static esp_err_t read_data(esp_lcd_touch_handle_t tp);
static bool get_xy(esp_lcd_touch_handle_t tp, uint16_t *x, uint16_t *y, uint16_t *strength, uint8_t *point_num, uint8_t max_point_num);
static esp_err_t del(esp_lcd_touch_handle_t tp);

static esp_err_t i2c_read_bytes(esp_lcd_touch_handle_t tp, uint16_t reg, uint8_t *data, uint8_t len);

static esp_err_t reset(esp_lcd_touch_handle_t tp);
static esp_err_t read_id(esp_lcd_touch_handle_t tp);

esp_err_t esp_lcd_touch_new_i2c_chsc6417(const esp_lcd_panel_io_handle_t io, const esp_lcd_touch_config_t *config, esp_lcd_touch_handle_t *tp)
{
    ESP_RETURN_ON_FALSE(io, ESP_ERR_INVALID_ARG, TAG, "Invalid io");
    ESP_RETURN_ON_FALSE(config, ESP_ERR_INVALID_ARG, TAG, "Invalid config");
    ESP_RETURN_ON_FALSE(tp, ESP_ERR_INVALID_ARG, TAG, "Invalid touch handle");

    /* Prepare main structure */
    esp_err_t ret = ESP_OK;
    esp_lcd_touch_handle_t chsc6417 = calloc(1, sizeof(esp_lcd_touch_t));
    ESP_GOTO_ON_FALSE(chsc6417, ESP_ERR_NO_MEM, err, TAG, "Touch handle malloc failed");

    /* Communication interface */
    chsc6417->io = io;
    /* Only supported callbacks are set */
    chsc6417->read_data = read_data;
    chsc6417->get_xy = get_xy;
    chsc6417->del = del;
    /* Mutex */
    chsc6417->data.lock.owner = portMUX_FREE_VAL;
    /* Save config */
    memcpy(&chsc6417->config, config, sizeof(esp_lcd_touch_config_t));

    /* Prepare pin for touch interrupt */
    if (chsc6417->config.int_gpio_num != GPIO_NUM_NC) {
        const gpio_config_t int_gpio_config = {
            .mode = GPIO_MODE_INPUT,
            .intr_type = (chsc6417->config.levels.interrupt ? GPIO_INTR_POSEDGE : GPIO_INTR_NEGEDGE),
            .pin_bit_mask = BIT64(chsc6417->config.int_gpio_num)
        };
        ESP_GOTO_ON_ERROR(gpio_config(&int_gpio_config), err, TAG, "GPIO intr config failed");

        /* Register interrupt callback */
        if (chsc6417->config.interrupt_callback) {
            esp_lcd_touch_register_interrupt_callback(chsc6417, chsc6417->config.interrupt_callback);
        }
    }
    /* Prepare pin for touch controller reset */
    if (chsc6417->config.rst_gpio_num != GPIO_NUM_NC) {
        const gpio_config_t rst_gpio_config = {
            .mode = GPIO_MODE_OUTPUT,
            .pin_bit_mask = BIT64(chsc6417->config.rst_gpio_num)
        };
        ESP_GOTO_ON_ERROR(gpio_config(&rst_gpio_config), err, TAG, "GPIO reset config failed");
    }
    /* Reset controller */
    ESP_GOTO_ON_ERROR(reset(chsc6417), err, TAG, "Reset failed");
    /* Read product id */
    // ESP_GOTO_ON_ERROR(read_id(chsc6417), err, TAG, "Read version failed");
    *tp = chsc6417;

    ESP_LOGI(TAG, "LCD touch panel create success, version: %d.%d.%d", ESP_LCD_TOUCH_CHSC6417_VER_MAJOR, ESP_LCD_TOUCH_CHSC6417_VER_MINOR,
             ESP_LCD_TOUCH_CHSC6417_VER_PATCH);

    return ESP_OK;
err:
    if (chsc6417) {
        del(chsc6417);
    }
    ESP_LOGE(TAG, "Initialization failed!");
    return ret;
}

static esp_err_t read_data(esp_lcd_touch_handle_t tp)
{
    typedef struct {
        uint8_t num;
        uint8_t x_h : 4;
        uint8_t : 4;
        uint8_t x_l;
        uint8_t y_h : 4;
        uint8_t : 4;
        uint8_t y_l;
    } data_t;

    data_t point;
    
    uint8_t lvalue[3]={0};
    // uint8_t gesture_id=0;
    uint16_t x=0;
    uint16_t y=0;
    esp_err_t err = i2c_read_bytes(tp, 0xE0, (uint8_t *)lvalue, sizeof(lvalue));
    // 断言
    ESP_RETURN_ON_ERROR(err, TAG, "I2C read failed");
    
    // 如果读取失败，将num置为0
    if (err != ESP_OK) {
        point.num = 0;
        x = 0;
        y = 0;
    }else{
        point.num = lvalue[0] & 0x03; // 获取触摸点数量
        x = (uint16_t)((((lvalue[0] & 0x40) >> 6) << 8) | lvalue[1]);
        y = (uint16_t)(((lvalue[0] & 0x80) >> 7) << 8) | lvalue[2];
    }
    // gesture_id =  lvalue[0];

    // ESP_LOGI(TAG, "Touch position: %d,%d,%d", point.num, x, y);

    portENTER_CRITICAL(&tp->data.lock);
    point.num = (point.num > POINT_NUM_MAX ? POINT_NUM_MAX : point.num);
    tp->data.points = point.num;
    /* Fill all coordinates */
    for (int i = 0; i < point.num; i++) {
        tp->data.coords[i].x = x;
        tp->data.coords[i].y = y;
    }
    portEXIT_CRITICAL(&tp->data.lock);

    return ESP_OK;
}

// void read_touch()
// {
//   uint8_t data1[1]={0xE0};
//   uint8_t lvalue[3];

//   i2c_write(TOUCH_I2C_ADD,TOUCH_I2C_W,data1,1);
//   delay(1);
//   i2c_read(TOUCH_I2C_ADD,TOUCH_I2C_R,lvalue,3);

//   point_num = lvalue[0] & 0x03;
  
//   x = (uint16_t)((((lvalue[0] & 0x40) >> 6) << 8) | lvalue[1]);
//   y = (uint16_t)(((lvalue[0] & 0x80) >> 7) << 8) | lvalue[2];
// }


static bool get_xy(esp_lcd_touch_handle_t tp, uint16_t *x, uint16_t *y, uint16_t *strength, uint8_t *point_num, uint8_t max_point_num)
{
    portENTER_CRITICAL(&tp->data.lock);
    /* Count of points */
    *point_num = (tp->data.points > max_point_num ? max_point_num : tp->data.points);
    for (size_t i = 0; i < *point_num; i++) {
        x[i] = tp->data.coords[i].x;
        y[i] = tp->data.coords[i].y;

        if (strength) {
            strength[i] = tp->data.coords[i].strength;
        }
    }
    /* Invalidate */
    tp->data.points = 0;
    portEXIT_CRITICAL(&tp->data.lock);

    return (*point_num > 0);
}

static esp_err_t del(esp_lcd_touch_handle_t tp)
{
    /* Reset GPIO pin settings */
    if (tp->config.int_gpio_num != GPIO_NUM_NC) {
        gpio_reset_pin(tp->config.int_gpio_num);
        if (tp->config.interrupt_callback) {
            gpio_isr_handler_remove(tp->config.int_gpio_num);
        }
    }
    if (tp->config.rst_gpio_num != GPIO_NUM_NC) {
        gpio_reset_pin(tp->config.rst_gpio_num);
    }
    /* Release memory */
    free(tp);

    return ESP_OK;
}

static esp_err_t reset(esp_lcd_touch_handle_t tp)
{
    if (tp->config.rst_gpio_num != GPIO_NUM_NC) {
        ESP_RETURN_ON_ERROR(gpio_set_level(tp->config.rst_gpio_num, tp->config.levels.reset), TAG, "GPIO set level failed");
        vTaskDelay(pdMS_TO_TICKS(200));
        ESP_RETURN_ON_ERROR(gpio_set_level(tp->config.rst_gpio_num, !tp->config.levels.reset), TAG, "GPIO set level failed");
        vTaskDelay(pdMS_TO_TICKS(200));
    }

    return ESP_OK;
}

static esp_err_t read_id(esp_lcd_touch_handle_t tp)
{
    uint8_t id;
    ESP_RETURN_ON_ERROR(i2c_read_bytes(tp, CHIP_ID_REG, &id, 1), TAG, "I2C read failed");
    ESP_LOGI(TAG, "IC id: %d", id);
    return ESP_OK;
}

static esp_err_t i2c_read_bytes(esp_lcd_touch_handle_t tp, uint16_t reg, uint8_t *data, uint8_t len)
{
    ESP_RETURN_ON_FALSE(data, ESP_ERR_INVALID_ARG, TAG, "Invalid data");

    return esp_lcd_panel_io_rx_param(tp->io, reg, data, len);
}
