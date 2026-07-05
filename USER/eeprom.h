#ifndef __EEPROM_H
#define __EEPROM_H

#include "sys.h"

// 硬件接口初始化（重命名为 EEPROM_I2C_Init，避免同官方固件库冲突）
void EEPROM_I2C_Init(void);

// 读取和写入历史最高分 (offset: EASY=28, MEDIUM=29, HARD=30)
uint16_t EEPROM_Read_HighScore(uint8_t offset);
void EEPROM_Write_HighScore(uint8_t offset, uint16_t score);

#endif
