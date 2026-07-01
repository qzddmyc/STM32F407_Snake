#ifndef __EEPROM_H
#define __EEPROM_H

#include "sys.h"

// 硬件接口初始化（重命名为 EEPROM_I2C_Init，避免同官方固件库冲突）
void EEPROM_I2C_Init(void);

// 读取和写入历史最高分 (16位无符号整型，支持最高 65535 分)
uint16_t EEPROM_Read_HighScore(void);
void EEPROM_Write_HighScore(uint16_t score);

#endif
