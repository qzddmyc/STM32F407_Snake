#ifndef __SNAKE_H
#define __SNAKE_H

#include "sys.h"
#include "lcd.h"

// 游戏网格区域定义 (适用于 480 * 800 屏幕)
#define GAME_X_START   24   // 左边距 24 像素
#define GAME_Y_START   40   // 上边距 40 像素
#define GAME_GRID_NUM_X 18   // 水平 18 格 (18 * 24 = 432 像素，左右总边距 48)
#define GAME_GRID_NUM_Y 30   // 垂直 30 格 (30 * 24 = 720 像素，上下总边距 80)
#define GRID_SIZE      24   // 增大网格大小到 24x24，画面显示更饱满清晰
// 游戏区物理大小：288 x 208 像素，居中在 320x240 屏幕上

// 颜色定义
#define COLOR_BG      BLACK
#define COLOR_WALL    GRAY
#define COLOR_SNAKE_H RED
#define COLOR_SNAKE_B GREEN
#define COLOR_FOOD    YELLOW
#define COLOR_GOLD    0xFE80 // 金色食物

// 方向定义
#define DIR_UP    1
#define DIR_DOWN  2
#define DIR_LEFT  3
#define DIR_RIGHT 4

// 坐标点结构体
typedef struct {
    int16_t x;
    int16_t y;
} Point;

// 蛇的结构体
#define MAX_SNAKE_LEN 200
typedef struct {
    Point body[MAX_SNAKE_LEN]; // body[0] 是蛇头，后面是蛇身
    uint16_t length;           // 蛇当前长度
    uint8_t direction;         // 蛇当前行进方向
} Snake;

// 游戏难度枚举定义
typedef enum {
    DIFF_EASY = 0,
    DIFF_MEDIUM,
    DIFF_HARD
} Difficulty;

// 游戏模式定义
typedef enum {
    MODE_SINGLE = 0  // 单人模式
} Game_Mode;

// 游戏状态定义
typedef enum {
    GAME_MENU = 0,
    GAME_RUNNING,
	GAME_PAUSED,
    GAME_OVER
} Game_State;

// 全局变量声明（方便在 main.c 中读取状态）
extern Game_State current_state;
extern Snake mySnake;
// 声明全局难度变量
extern Difficulty game_difficulty;
// 声明全局模式变量
extern Game_Mode game_mode;

// 金色食物与减速效果
extern Point  myGoldFood;
extern uint8_t gold_food_active;
extern uint8_t slow_active;
extern uint16_t slow_ticks;

// 磁铁与吸引效果
extern Point  myMagnet;
extern uint8_t magnet_active;
extern uint8_t attract_active;
extern uint16_t attract_ticks;

// 游戏逻辑接口函数
void Snake_Game_Init(void);  // 游戏初始化
void Snake_Game_Tick(void);  // 游戏每一步的节拍
void Snake_Change_Direction(uint8_t new_dir); // 改变蛇的方向
void Snake_Redraw(void);
#endif

