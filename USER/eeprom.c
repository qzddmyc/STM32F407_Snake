#include "eeprom.h"
#include "delay.h"

// PB9 引脚输入输出方向切换宏定义（适配探索者 STM32F407）
#define SDA_IN()  {GPIOB->MODER&=~(3<<(9*2));GPIOB->MODER|=0<<(9*2);} // PB9输入
#define SDA_OUT() {GPIOB->MODER&=~(3<<(9*2));GPIOB->MODER|=1<<(9*2);} // PB9输出

#define I2C_SCL      PBout(8)     // SCL -> PB8
#define I2C_SDA_OUT  PBout(9)     // SDA 输出 -> PB9
#define I2C_SDA_IN   PBin(9)      // SDA 输入 -> PB9

// 软件 I2C 初始化
void EEPROM_I2C_Init(void) {
    GPIO_InitTypeDef GPIO_InitStructure;
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE); // 使使能 GPIOB 时钟
    
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8 | GPIO_Pin_9;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
    GPIO_Init(GPIOB, &GPIO_InitStructure);
    
    I2C_SCL = 1;
    I2C_SDA_OUT = 1;
}

// 产生 I2C 起始信号
static void I2C_Start(void) {
    SDA_OUT();
    I2C_SDA_OUT = 1;
    I2C_SCL = 1;
    delay_us(4);
    I2C_SDA_OUT = 0; // START: SCL高电平时，SDA由高变低
    delay_us(4);
    I2C_SCL = 0;     // 钳住总线
}

// 产生 I2C 停止信号
static void I2C_Stop(void) {
    SDA_OUT();
    I2C_SCL = 0;
    I2C_SDA_OUT = 0;
    delay_us(4);
    I2C_SCL = 1;
    delay_us(4);
    I2C_SDA_OUT = 1; // STOP: SCL高电平时，SDA由低变高
    delay_us(4);
}

// 等待应答信号 (ACK)
static uint8_t I2C_Wait_Ack(void) {
    uint8_t ucErrTime = 0;
    SDA_IN();
    I2C_SDA_OUT = 1; 
    delay_us(2);
    I2C_SCL = 1; 
    delay_us(4); // 延迟4us，确保 SCL 物理电平完全变高并稳定
    while (I2C_SDA_IN) {
        ucErrTime++;
        if (ucErrTime > 250) {
            I2C_Stop();
            return 1;
        }
    }
    I2C_SCL = 0;
    return 0;
}

// I2C 发送一个字节
static void I2C_Send_Byte(uint8_t txd) {
    uint8_t t;
    SDA_OUT();
    I2C_SCL = 0;
    for (t = 0; t < 8; t++) {
        I2C_SDA_OUT = (txd & 0x80) >> 7;
        txd <<= 1;
        delay_us(4); // 延长低电平维持时间
        I2C_SCL = 1;
        delay_us(4); // 延长高电平维持时间
        I2C_SCL = 0;
        delay_us(4);
    }
}

// I2C 读取一个字节
static uint8_t I2C_Read_Byte(unsigned char ack) {
    unsigned char i, receive = 0;
    SDA_IN();
    for (i = 0; i < 8; i++) {
        I2C_SCL = 0;
        delay_us(4); // 延长低电平时间
        I2C_SCL = 1;
        delay_us(4); // 关键：必须延时4us，等待SCL电平完全变高且SDA数据线物理电平稳定！
        receive <<= 1;
        if (I2C_SDA_IN) receive++;
        delay_us(2);
    }
    I2C_SCL = 0;
    SDA_OUT();
    I2C_SDA_OUT = !ack;
    delay_us(4);
    I2C_SCL = 1;
    delay_us(4);
    I2C_SCL = 0;
    return receive;
}

// AT24C02 读一个字节
static uint8_t AT24C02_ReadOneByte(uint16_t ReadAddr) {
    uint8_t temp = 0;
    I2C_Start();
    I2C_Send_Byte(0xA0); 
    I2C_Wait_Ack();
    I2C_Send_Byte(ReadAddr); 
    I2C_Wait_Ack();
    I2C_Start();
    I2C_Send_Byte(0xA1); 
    I2C_Wait_Ack();
    temp = I2C_Read_Byte(0); 
    I2C_Stop();
    return temp;
}

// AT24C02 写一个字节
static void AT24C02_WriteOneByte(uint16_t WriteAddr, uint8_t DataToWrite) {
    I2C_Start();
    I2C_Send_Byte(0xA0);
    I2C_Wait_Ack();
    I2C_Send_Byte(WriteAddr);
    I2C_Wait_Ack();
    I2C_Send_Byte(DataToWrite);
    I2C_Wait_Ack();
    I2C_Stop();
    delay_ms(10); // AT24C02 物理写入周期，不可缩短
}

// 供外部调用的最高分读取函数 (offset: EASY=28, MEDIUM=29, HARD=30)
uint16_t EEPROM_Read_HighScore(uint8_t offset) {
    uint16_t addr = offset * 2;
    uint8_t high = AT24C02_ReadOneByte(addr);
    uint8_t low = AT24C02_ReadOneByte(addr + 1);
    uint16_t record = (high << 8) | low;
    
    if (record == 0xFFFF) return 0;
    return record;
}

// 供外部调用的最高分保存函数
void EEPROM_Write_HighScore(uint8_t offset, uint16_t score) {
    uint16_t addr = offset * 2;
    uint8_t high = (score >> 8) & 0xFF;
    uint8_t low = score & 0xFF;
    AT24C02_WriteOneByte(addr, high);
    AT24C02_WriteOneByte(addr + 1, low);
}
