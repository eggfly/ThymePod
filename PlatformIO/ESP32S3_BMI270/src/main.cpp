
#include <Arduino.h>
#include <Wire.h>
#include "hal_imu.h"

#define I2C_SDA 2
#define I2C_SCL 3

#define TAG "MAIN"
#define MY_LOG(fmt, ...) ESP_LOGI(TAG, fmt, ##__VA_ARGS__)


void setup()
{
  Serial.begin(115200);
  bool i2cResult = Wire.begin(I2C_SDA, I2C_SCL);
  delay(5000);
  
  Serial.printf("I2C Result: %d\n", i2cResult);
  Serial.printf("IMU Address: %d\n", BMI2_I2C_PRIM_ADDR);
  // MY_LOG NOT WORK
  // MY_LOG("IMU use adddress: %d", BMI2_I2C_PRIM_ADDR);

  init_imu();
  Serial.println("init_imu called");
}

void loop()
{
  imu_t imu_data;
  read_imu(&imu_data);
  Serial.printf("imu_data: %f, %f, %f\n", imu_data.acc_x, imu_data.acc_y, imu_data.acc_z);
  delay(200);
}
