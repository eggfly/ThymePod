
#include <Arduino.h>
#include "hal_battery_fuel_gauge.h"


#define I2C_SDA 2
#define I2C_SCL 3

void scanI2C() {
  Serial.println("开始I2C扫描...");
  for (byte address = 1; address < 127; address++) {
    Wire.beginTransmission(address);
    byte error = Wire.endTransmission();
    
    if (error == 0) {
      Serial.printf("在地址 0x%02X 发现I2C设备\n", address);
    } else if (error == 4) {
      Serial.printf("在地址 0x%02X 发现未知错误\n", address);
    }
  }
  Serial.println("I2C扫描完成");
}

void setup()
{
  Serial.begin(115200);
  delay(5000);
  Serial.println("ESP32S3 MAX17048 启动中...");
  
  bool i2cResult = Wire.begin(I2C_SDA, I2C_SCL);
  Serial.printf("I2C初始化结果: %d\n", i2cResult);
  
  // 设置I2C时钟频率
  Wire.setClock(100000); // 设置为100kHz，降低速度以提高稳定性
  Serial.println("I2C时钟频率设置为100kHz");
  
  delay(1000);
  
  // 扫描I2C设备
  scanI2C();
  
  delay(1000);
  
  Serial.println("开始初始化燃料计...");
  init_fuel_gauge();
  Serial.println("燃料计初始化完成");
}

void loop()
{
  static unsigned long lastResetCheck = 0;
  static int consecutiveZeroPercent = 0;
  const unsigned long RESET_CHECK_INTERVAL = 30000; // 30秒检查一次
  
  fuel_gauge_t fuel_gauge;
  if (read_fuel_gauge(&fuel_gauge)) {
    print_fuel_gauge(fuel_gauge);
    
    // 检查是否需要重置
    if (fuel_gauge.cellPercent == 0.0 && fuel_gauge.cellVoltage > 3.0) {
      consecutiveZeroPercent++;
    } else {
      consecutiveZeroPercent = 0;
    }
    
    // 如果连续30次（约60秒）都是0%，且已经过了30秒，则尝试重置
    if (consecutiveZeroPercent >= 30 && (millis() - lastResetCheck) > RESET_CHECK_INTERVAL) {
      Serial.println("检测到持续0%电量，尝试重置电池学习...");
      reset_and_learn_battery();
      consecutiveZeroPercent = 0;
      lastResetCheck = millis();
    }
  } else {
    Serial.println("读取燃料计数据失败");
  }
  delay(2000);
}
