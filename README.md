# STM32F407 贪吃蛇游戏

基于正点原子探索者 STM32F4 开发板的贪吃蛇游戏。

---

## 项目功能

- 经典贪吃蛇玩法：18×30 网格，像素级精准绘制
- 三级难度：EASY / MEDIUM / HARD，统一每 3 苹果升 1 级
- 三种食物：普通苹果、金色苹果（双倍分数+减速）、磁铁（远程吸引）
- 长按加速：按住当前方向键 200ms 后自动加速（30ms/帧），松手恢复；转向时自动进入 200ms 防误触冷却
- 暂停 / 结算 / 掉电保存最高分
- 串口调试：自动输出游戏状态 + 指令控制（蜂鸣器/难度切换）
- 蜂鸣器静音开关

---

## 项目结构

```
STM32F407_Snake/
├── CORE/              # ARM Cortex-M4 核心支持
├── FWLIB/             # STM32F4 标准外设库
├── HARDWARE/          # 板级驱动
│   ├── BEEP/          # PF8 蜂鸣器
│   ├── KEY/           # 按键扫描 PA0 PE2~4
│   ├── LCD/           # TFT-LCD FSMC 驱动
│   └── LED/           # PF9 PF10
├── SYSTEM/            # 系统模块
│   ├── delay/         # SysTick 延时
│   ├── sys/           # 系统初始化
│   └── usart/         # USART1 printf 重定向 中断接收
├── USER/              # 用户应用层
│   ├── main.c         # 主循环 状态机 串口指令
│   ├── snake.c        # 蛇逻辑 食物生成 碰撞检测
│   ├── snake.h        # 数据结构与接口
│   └── eeprom.c/h     # AT24C02 最高分读写
├── Sample_images/     # 食物图案设计稿 24×24
├── README.md          # 项目介绍
├── MANUAL.md          # 操作手册
└── HOWITRUNS.md       # 程序运行全流程
```

---

## 硬件平台

| 组件 | 说明 |
|------|------|
| MCU | STM32F407ZGT6, 168MHz |
| 屏幕 | TFT-LCD 480×800 竖屏, FSMC 驱动 |
| 按键 | WK_UP (PA0), KEY2 (PE2), KEY1 (PE3), KEY0 (PE4) |
| 蜂鸣器 | PF8, 支持静音开关 |
| LED | DS0 (PF9), DS1 (PF10) |
| 串口 | USART1 115200, PA9/PA10 (CH340) |
| EEPROM | AT24C02, 软件 I2C (PB8/PB9), 掉电保存最高分 |

---

## 文档

- **[操作手册](MANUAL.md)** — 菜单操作、游戏玩法、串口指令
- **[运行流程](HOWITRUNS.md)** — 程序完整执行逻辑、数据流、代码细节

---

## 注意事项

- 4.3/7 寸屏需外部 12V 1A 供电
- 串口须先初始化，否则 LCD 及 printf 不可用
- 蜂鸣器默认静音：`beep_enable = 0`，通过串口 `beep on` 开启

---

*正点原子 @ ALIENTEK*
