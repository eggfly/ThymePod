#include <stdio.h>
#include "i2c_bsp.h"


#define TEST_I2C_PORT I2C_NUM_0
//#define EXAMPLE_USE_TOUCH    //触控屏的实现


#ifndef EXAMPLE_USE_TOUCH

#define I2C_MASTER_SCL_IO 39
#define I2C_MASTER_SDA_IO 40

void I2C_master_Init(void)
{
  i2c_config_t conf = 
  {
    .mode = I2C_MODE_MASTER,
    .sda_io_num = I2C_MASTER_SDA_IO,         // 配置 SDA 的 GPIO
    .sda_pullup_en = GPIO_PULLUP_ENABLE,
    .scl_io_num = I2C_MASTER_SCL_IO,         // 配置 SCL 的 GPIO
    .scl_pullup_en = GPIO_PULLUP_ENABLE,
    .master.clk_speed = 300 * 1000,  // 为项目选择频率
    .clk_flags = 0,          // 可选项，可以使用 I2C_SCLK_SRC_FLAG_* 标志来选择 I2C 源时钟
  };
  ESP_ERROR_CHECK(i2c_param_config(TEST_I2C_PORT, &conf));
  ESP_ERROR_CHECK(i2c_driver_install(TEST_I2C_PORT, conf.mode,0,0,0));
}
#endif

uint8_t I2C_writr_buff(uint8_t addr,uint8_t reg,uint8_t *buf,uint8_t len)
{
  uint8_t ret;
  uint8_t *pbuf = (uint8_t*)malloc(len+1);
  pbuf[0] = reg;
  for(uint8_t i = 0; i<len; i++)
  {
    pbuf[i+1] = buf[i];
  }
  ret = i2c_master_write_to_device(TEST_I2C_PORT,addr,pbuf,len+1,1000);
  free(pbuf);
  pbuf = NULL;
  return ret;
}
uint8_t I2C_read_buff(uint8_t addr,uint8_t reg,uint8_t *buf,uint8_t len)
{
  uint8_t ret;
  ret = i2c_master_write_read_device(TEST_I2C_PORT,addr,&reg,1,buf,len,1000);
  return ret;
}
uint8_t I2C_master_write_read_device(uint8_t addr,uint8_t *writeBuf,uint8_t writeLen,uint8_t *readBuf,uint8_t readLen)
{
  uint8_t ret;
  ret = i2c_master_write_read_device(TEST_I2C_PORT,addr,writeBuf,writeLen,readBuf,readLen,1000);
  return ret;
}