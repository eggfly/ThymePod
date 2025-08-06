
#include <Arduino.h>
#include "hal_battery_fuel_gauge.h"

#define TAG "FUEL_GAUGE"

Adafruit_MAX17048 maxlipo;

#define MY_LOG(fmt, ...) ESP_LOGI(TAG, fmt, ##__VA_ARGS__)

void init_fuel_gauge()
{
  Serial.println("开始初始化MAX17048...");
  
  // 尝试多次初始化
  int attempts = 0;
  const int max_attempts = 5;
  
  while (!maxlipo.begin() && attempts < max_attempts)
  {
    attempts++;
    Serial.printf("尝试 %d/%d: 无法找到Adafruit MAX17048\n", attempts, max_attempts);
    Serial.println("请确保电池已连接且I2C连接正确!");
    
    // 检查I2C设备是否响应
    Wire.beginTransmission(0x36);
    byte error = Wire.endTransmission();
    Serial.printf("I2C地址0x36响应状态: %d (0=成功, 1=数据过长, 2=NACK地址, 3=NACK数据, 4=其他错误)\n", error);
    
    delay(2000);
  }
  
  if (attempts >= max_attempts) {
    Serial.println("MAX17048初始化失败，达到最大尝试次数");
    return;
  }
  
  Serial.println("MAX17048初始化成功!");
  
  // 获取芯片ID
  uint8_t chipID = maxlipo.getChipID();
  Serial.printf("找到MAX17048，芯片ID: 0x%02X\n", chipID);
  
  // 获取IC版本
  uint16_t version = maxlipo.getICversion();
  Serial.printf("IC版本: 0x%04X\n", version);
  
  // 检查设备是否就绪
  if (maxlipo.isDeviceReady()) {
    Serial.println("设备就绪状态: 是");
  } else {
    Serial.println("设备就绪状态: 否");
  }
  
  // 执行快速启动来重新学习电池特性
  Serial.println("执行快速启动...");
  maxlipo.quickStart();
  delay(1000);
  
  // 读取初始数据
  float initialVoltage = maxlipo.cellVoltage();
  float initialPercent = maxlipo.cellPercent();
  Serial.printf("初始电压: %.3f V\n", initialVoltage);
  Serial.printf("初始电量: %.1f %%\n", initialPercent);
  
  // 检查是否需要学习
  if (initialPercent == 0.0 && initialVoltage > 3.0) {
    Serial.println("警告: 电池电量显示为0%，但电压正常。这可能需要学习周期。");
    Serial.println("建议: 让设备运行一段时间，或者进行完整的充放电循环。");
  }
}

bool read_fuel_gauge(fuel_gauge_t *fuel_gauge)
{
  if (fuel_gauge == nullptr) {
    Serial.println("错误: fuel_gauge指针为空");
    return false;
  }
  
  // 检查设备是否就绪
  if (!maxlipo.isDeviceReady()) {
    Serial.println("警告: MAX17048设备未就绪");
  }
  
  float cellVoltage = maxlipo.cellVoltage();
  if (isnan(cellVoltage))
  {
    Serial.println("错误: 无法读取电池电压，请检查电池连接!");
    return false;
  }
  
  fuel_gauge->cellVoltage = cellVoltage;
  fuel_gauge->cellPercent = maxlipo.cellPercent();
  fuel_gauge->chargeRate = maxlipo.chargeRate();
  
  // 检查读取的数据是否合理
  if (fuel_gauge->cellVoltage < 0 || fuel_gauge->cellVoltage > 5.0) {
    Serial.printf("警告: 电池电压值异常: %f V\n", fuel_gauge->cellVoltage);
  }
  
  if (fuel_gauge->cellPercent < 0 || fuel_gauge->cellPercent > 100) {
    Serial.printf("警告: 电池百分比值异常: %f %%\n", fuel_gauge->cellPercent);
  }
  
  // 检查电池状态
  static int zero_percent_count = 0;
  if (fuel_gauge->cellPercent == 0.0 && fuel_gauge->cellVoltage > 3.0) {
    zero_percent_count++;
    if (zero_percent_count % 10 == 0) { // 每10次打印一次警告
      Serial.println("诊断: 电池电量持续显示为0%，可能的原因:");
      Serial.println("1. MAX17048需要学习周期");
      Serial.println("2. 电池类型不匹配");
      Serial.println("3. 需要重新校准");
      Serial.printf("已连续 %d 次读取到0%%电量\n", zero_percent_count);
    }
  } else {
    zero_percent_count = 0;
  }
  
  return true;
}

void print_fuel_gauge(fuel_gauge_t fuel_gauge)
{
  Serial.println("=== 电池状态 ===");
  Serial.printf("电池电压: %.3f V\n", fuel_gauge.cellVoltage);
  Serial.printf("电池电量: %.1f %%\n", fuel_gauge.cellPercent);
  Serial.printf("充电速率: %.2f %%/h\n", fuel_gauge.chargeRate);
  Serial.println("================");
}

void hibernate_fuel_gauge()
{
  maxlipo.hibernate();
}

void reset_and_learn_battery()
{
  Serial.println("=== 重置并重新学习电池 ===");
  
  // 重置MAX17048
  Serial.println("1. 重置MAX17048...");
  if (maxlipo.reset()) {
    Serial.println("   重置成功");
  } else {
    Serial.println("   重置失败");
  }
  
  delay(1000);
  
  // 执行快速启动
  Serial.println("2. 执行快速启动...");
  maxlipo.quickStart();
  delay(2000);
  
  // 读取重置后的数据
  Serial.println("3. 读取重置后的数据...");
  float voltage = maxlipo.cellVoltage();
  float percent = maxlipo.cellPercent();
  float rate = maxlipo.chargeRate();
  
  Serial.printf("   电压: %.3f V\n", voltage);
  Serial.printf("   电量: %.1f %%\n", percent);
  Serial.printf("   充电速率: %.2f %%/h\n", rate);
  
  Serial.println("=== 重置完成 ===");
}