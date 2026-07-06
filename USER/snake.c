#include "snake.h"
#include "beep.h"
#include "delay.h" 
#include "led.h"    
#include <stdlib.h> 
#include "usart.h"

// ==================== 0. 全局变量定义 ====================
Game_State current_state = GAME_MENU;
Difficulty game_difficulty = DIFF_MEDIUM; // game_difficulty 全局定义
Game_Mode game_mode = MODE_SINGLE;         // game_mode 全局定义
Snake mySnake;
Point myFood;                             // myFood 全局定义
uint32_t score = 0;
uint32_t food_eaten = 0;  // 实际吃到的食物个数（金苹果+1，非按分数换算）

// 金色食物与减速效果
Point  myGoldFood;
uint8_t gold_food_active = 0;
uint8_t slow_active = 0;
uint16_t slow_ticks = 0;
static uint8_t first_food_done = 0; // 首个食物生成标记

// 磁铁与吸引效果
Point  myMagnet;
uint8_t magnet_active = 0;
uint8_t attract_active = 0;
uint16_t attract_ticks = 0;

/* ==================== 1. 像素级绘制函数组 ==================== */

// 擦除网格（用背景色涂满）
static void Clear_Grid_Cell(int16_t x, int16_t y) {
    uint16_t sx = GAME_X_START + x * GRID_SIZE;
    uint16_t sy = GAME_Y_START + y * GRID_SIZE;
    LCD_Fill(sx, sy, sx + GRID_SIZE - 1, sy + GRID_SIZE - 1, COLOR_BG);
}

// 1号蛇身：绿色方块（内缩1像素黑边）
static void Draw_Snake_Body(int16_t x, int16_t y) {
    uint16_t sx = GAME_X_START + x * GRID_SIZE;
    uint16_t sy = GAME_Y_START + y * GRID_SIZE;
    LCD_Fill(sx, sy, sx + GRID_SIZE - 1, sy + GRID_SIZE - 1, COLOR_BG);
    LCD_Fill(sx + 1, sy + 1, sx + GRID_SIZE - 2, sy + GRID_SIZE - 2, COLOR_SNAKE_B);
}

// 1号蛇头：红色圆角方块 + 黑色眼
static void Draw_Snake_Head(int16_t x, int16_t y) {
    uint16_t sx = GAME_X_START + x * GRID_SIZE;
    uint16_t sy = GAME_Y_START + y * GRID_SIZE;
    
    LCD_Fill(sx, sy, sx + GRID_SIZE - 1, sy + GRID_SIZE - 1, COLOR_BG);
    LCD_Fill(sx + 1, sy + 1, sx + GRID_SIZE - 2, sy + GRID_SIZE - 2,
             slow_active ? COLOR_GOLD : COLOR_SNAKE_H);
    
    switch (mySnake.direction) {
        case DIR_UP:
            LCD_Fill(sx + 5, sy + 5, sx + 8, sy + 8, BLACK);   
            LCD_Fill(sx + 15, sy + 5, sx + 18, sy + 8, BLACK); 
            break;
        case DIR_DOWN:
            LCD_Fill(sx + 5, sy + 15, sx + 8, sy + 18, BLACK);  
            LCD_Fill(sx + 15, sy + 15, sx + 18, sy + 18, BLACK); 
            break;
        case DIR_LEFT:
            LCD_Fill(sx + 5, sy + 5, sx + 8, sy + 8, BLACK);   
            LCD_Fill(sx + 5, sy + 15, sx + 8, sy + 18, BLACK);  
            break;
        case DIR_RIGHT:
            LCD_Fill(sx + 15, sy + 5, sx + 18, sy + 8, BLACK);  
            LCD_Fill(sx + 15, sy + 15, sx + 18, sy + 18, BLACK); 
            break;
    }
}

// 绘制苹果食物
static void Draw_Food_Apple(int16_t x, int16_t y) {
    uint16_t sx = GAME_X_START + x * GRID_SIZE;
    uint16_t sy = GAME_Y_START + y * GRID_SIZE;
    
    LCD_Fill(sx, sy, sx + GRID_SIZE - 1, sy + GRID_SIZE - 1, COLOR_BG);
    LCD_Fill(sx + 5, sy + 7, sx + 18, sy + 20, RED);
    
    POINT_COLOR = COLOR_BG; 
    LCD_DrawPoint(sx + 5, sy + 7);
    LCD_DrawPoint(sx + 18, sy + 7);
    LCD_DrawPoint(sx + 5, sy + 20);
    LCD_DrawPoint(sx + 18, sy + 20);
    
    LCD_Fill(sx + 11, sy + 3, sx + 12, sy + 6, WHITE);
    LCD_Fill(sx + 13, sy + 3, sx + 16, sy + 4, GREEN);
}

// 绘制金色苹果食物
static void Draw_Gold_Food_Apple(int16_t x, int16_t y) {
    uint16_t sx = GAME_X_START + x * GRID_SIZE;
    uint16_t sy = GAME_Y_START + y * GRID_SIZE;
    
    LCD_Fill(sx, sy, sx + GRID_SIZE - 1, sy + GRID_SIZE - 1, COLOR_BG);
    LCD_Fill(sx + 5, sy + 7, sx + 18, sy + 20, COLOR_GOLD);
    
    POINT_COLOR = COLOR_BG; 
    LCD_DrawPoint(sx + 5, sy + 7);
    LCD_DrawPoint(sx + 18, sy + 7);
    LCD_DrawPoint(sx + 5, sy + 20);
    LCD_DrawPoint(sx + 18, sy + 20);
    
    LCD_Fill(sx + 11, sy + 3, sx + 12, sy + 6, WHITE);
    LCD_Fill(sx + 13, sy + 3, sx + 16, sy + 4, GREEN);
}

// 绘制U型磁铁
static void Draw_Magnet(int16_t x, int16_t y) {
    uint16_t sx = GAME_X_START + x * GRID_SIZE;
    uint16_t sy = GAME_Y_START + y * GRID_SIZE;
    
    LCD_Fill(sx, sy, sx + GRID_SIZE - 1, sy + GRID_SIZE - 1, COLOR_BG);
    
    // 左极柱 RED (5,3)~(8,15)
    LCD_Fill(sx + 5, sy + 3, sx + 8, sy + 15, RED);
    // 右极柱 BLUE (15,3)~(18,15)
    LCD_Fill(sx + 15, sy + 3, sx + 18, sy + 15, BLUE);
    
    // Row 16-18: 7R(5~11) + 7B(12~18)
    LCD_Fill(sx + 5, sy + 16, sx + 11, sy + 16, RED);
    LCD_Fill(sx + 12, sy + 16, sx + 18, sy + 16, BLUE);
    LCD_Fill(sx + 5, sy + 17, sx + 11, sy + 17, RED);
    LCD_Fill(sx + 12, sy + 17, sx + 18, sy + 17, BLUE);
    LCD_Fill(sx + 5, sy + 18, sx + 11, sy + 18, RED);
    LCD_Fill(sx + 12, sy + 18, sx + 18, sy + 18, BLUE);
    
    // Row 19: 6R(6~11) + 6B(12~17) — 两端外角抹黑
    LCD_Fill(sx + 6, sy + 19, sx + 11, sy + 19, RED);
    LCD_Fill(sx + 12, sy + 19, sx + 17, sy + 19, BLUE);
    
    // 顶部金属帽 WHITE (5,3)~(8,3) / (15,3)~(18,3)
    LCD_Fill(sx + 5, sy + 3, sx + 8, sy + 3, WHITE);
    LCD_Fill(sx + 15, sy + 3, sx + 18, sy + 3, WHITE);
    
    // 外角圆角（6处）
    POINT_COLOR = COLOR_BG;
    LCD_DrawPoint(sx + 5, sy + 3);
    LCD_DrawPoint(sx + 8, sy + 3);
    LCD_DrawPoint(sx + 15, sy + 3);
    LCD_DrawPoint(sx + 18, sy + 3);
    LCD_DrawPoint(sx + 5, sy + 19);
    LCD_DrawPoint(sx + 18, sy + 19);
}

/* ==================== 2. 核心游戏控制 ==================== */

// 食物生成
static void Generate_Food(void) {
    uint8_t on_snake = 1;
    uint16_t i; 
    
    while (on_snake) {
        on_snake = 0;
        myFood.x = rand() % GAME_GRID_NUM_X;
        myFood.y = rand() % GAME_GRID_NUM_Y;
        
        // 检查是否生成在蛇身上
        for (i = 0; i < mySnake.length; i++) {
            if (mySnake.body[i].x == myFood.x && mySnake.body[i].y == myFood.y) {
                on_snake = 1; 
                break;
            }
        }
    }
    Draw_Food_Apple(myFood.x, myFood.y); 
    printf("[FOOD] Generated at (%d, %d)\r\n", myFood.x, myFood.y);
    
    // 特殊物品生成（概率按难度区分，互斥，首个食物不触发）
    if (first_food_done) {
        uint8_t roll = rand() % 100;
        uint8_t spawn_type = 0; // 0=无, 1=金, 2=磁
        uint8_t gold_pct, magnet_pct;
        
        if (game_difficulty == DIFF_EASY)           { gold_pct = 35; magnet_pct = 15; }
        else if (game_difficulty == DIFF_MEDIUM)    { gold_pct = 25; magnet_pct = 10; }
        else                                        { gold_pct = 15; magnet_pct =  5; }
        
        if (!gold_food_active && !magnet_active) {
            if (roll < gold_pct)                spawn_type = 1;
            else if (roll < gold_pct + magnet_pct) spawn_type = 2;
        } else if (gold_food_active && !magnet_active) {
            if (roll < magnet_pct)              spawn_type = 2;
        } else if (!gold_food_active && magnet_active) {
            if (roll < gold_pct)                spawn_type = 1;
        }
        
        if (spawn_type != 0) {
            Point *target = (spawn_type == 1) ? &myGoldFood : &myMagnet;
            uint16_t retry = 0;
            on_snake = 1;
            while (on_snake && retry < 1000) {
                on_snake = 0;
                target->x = rand() % GAME_GRID_NUM_X;
                target->y = rand() % GAME_GRID_NUM_Y;
                if (target->x == myFood.x && target->y == myFood.y) on_snake = 1;
                for (i = 0; i < mySnake.length && !on_snake; i++) {
                    if (mySnake.body[i].x == target->x && mySnake.body[i].y == target->y)
                        on_snake = 1;
                }
                retry++;
            }
            if (retry < 1000) {
                if (spawn_type == 1) {
                    gold_food_active = 1;
                    Draw_Gold_Food_Apple(myGoldFood.x, myGoldFood.y);
                    printf("[GOLD] Generated at (%d, %d)\r\n", myGoldFood.x, myGoldFood.y);
                } else {
                    magnet_active = 1;
                    Draw_Magnet(myMagnet.x, myMagnet.y);
                    printf("[MAGNET] Generated at (%d, %d)\r\n", myMagnet.x, myMagnet.y);
                }
            }
        }
    }
    first_food_done = 1;
}

// 改变蛇的方向
void Snake_Change_Direction(uint8_t new_dir) {
    if (new_dir == DIR_UP && mySnake.direction != DIR_DOWN) mySnake.direction = DIR_UP;
    if (new_dir == DIR_DOWN && mySnake.direction != DIR_UP) mySnake.direction = DIR_DOWN;
    if (new_dir == DIR_LEFT && mySnake.direction != DIR_RIGHT) mySnake.direction = DIR_LEFT;
    if (new_dir == DIR_RIGHT && mySnake.direction != DIR_LEFT) mySnake.direction = DIR_RIGHT;
}

// 死亡爆闪警报辅助函数
static void Play_Death_Alert(void) {
    uint8_t j;
    for (j = 0; j < 4; j++) {
        LED0 = 0; 
        BEEP_ON(); 
        delay_ms(80);
        LED0 = 1; 
        BEEP = 0; 
        delay_ms(80);
    }
}

// 游戏数据初始化
void Snake_Game_Init(void) {
    uint16_t i; 

    LED0 = 1; 
    LED1 = 1;
    score = 0;
    food_eaten = 0;
    gold_food_active = 0;
    slow_active = 0;
    slow_ticks = 0;
    magnet_active = 0;
    attract_active = 0;
    attract_ticks = 0;
    first_food_done = 0;
    
    // 铺底黑色游戏区域
    LCD_Fill(
        GAME_X_START, 
        GAME_Y_START, 
        GAME_X_START + GAME_GRID_NUM_X * GRID_SIZE - 1, 
        GAME_Y_START + GAME_GRID_NUM_Y * GRID_SIZE - 1, 
        COLOR_BG
    );
    
    if (game_mode == MODE_SINGLE) 
    {
        // ================== 初始化 ==================
        mySnake.length = 3;
        mySnake.direction = DIR_RIGHT;
        
        mySnake.body[0].x = GAME_GRID_NUM_X / 2;     
        mySnake.body[0].y = GAME_GRID_NUM_Y / 2;
        
        mySnake.body[1].x = mySnake.body[0].x - 1;   
        mySnake.body[1].y = mySnake.body[0].y;
        
        mySnake.body[2].x = mySnake.body[0].x - 2;   
        mySnake.body[2].y = mySnake.body[0].y;
        
        // 绘制蛇
        Draw_Snake_Head(mySnake.body[0].x, mySnake.body[0].y); 
        for (i = 1; i < mySnake.length; i++) {
            Draw_Snake_Body(mySnake.body[i].x, mySnake.body[i].y); 
        }
    } 
    Generate_Food();
}

// 暂停恢复时重绘 
void Snake_Redraw(void) {
    uint16_t i;
    
    for (i = 1; i < mySnake.length; i++) {
        Draw_Snake_Body(mySnake.body[i].x, mySnake.body[i].y);
    }
    Draw_Snake_Head(mySnake.body[0].x, mySnake.body[0].y);
    
    Draw_Food_Apple(myFood.x, myFood.y);
    if (gold_food_active)
        Draw_Gold_Food_Apple(myGoldFood.x, myGoldFood.y);
    if (magnet_active)
        Draw_Magnet(myMagnet.x, myMagnet.y);
}

// 核心时钟节拍：蛇移动、碰撞检测与食物判定
void Snake_Game_Tick(void) {
    uint16_t i;

    if (current_state != GAME_RUNNING) return;

    if (game_mode == MODE_SINGLE) 
    {
        Point old_tail;
        Point next_head;
        uint8_t ate_normal = 0;
        uint8_t ate_golden = 0;
        // 磁铁吸引范围判定（3x3 内 + 不越界）
        #define IN_RANGE(hx, hy, fx, fy) \
            ((int16_t)(hx)-(int16_t)(fx) >= -1 && (int16_t)(hx)-(int16_t)(fx) <= 1 && \
             (int16_t)(hy)-(int16_t)(fy) >= -1 && (int16_t)(hy)-(int16_t)(fy) <= 1)

        old_tail = mySnake.body[mySnake.length - 1];
        next_head = mySnake.body[0];
        
        switch (mySnake.direction) {
            case DIR_UP:    next_head.y--; break;
            case DIR_DOWN:  next_head.y++; break;
            case DIR_LEFT:  next_head.x--; break;
            case DIR_RIGHT: next_head.x++; break;
        }
        
        // 计算蛇头前方一格（磁铁吸引时排除此格）
        int16_t front_x = next_head.x, front_y = next_head.y;
        switch (mySnake.direction) {
            case DIR_UP:    front_y--; break;
            case DIR_DOWN:  front_y++; break;
            case DIR_LEFT:  front_x--; break;
            case DIR_RIGHT: front_x++; break;
        }
        
        // 碰撞：撞墙
        if (next_head.x < 0 || next_head.x >= GAME_GRID_NUM_X ||
            next_head.y < 0 || next_head.y >= GAME_GRID_NUM_Y) {
            printf("[DEAD] Hit wall at (%d, %d)\r\n", next_head.x, next_head.y);
            current_state = GAME_OVER;
            Play_Death_Alert();
            return;
        }
        
        // 碰撞：撞自身
        for (i = 1; i < mySnake.length - 1; i++) {
            if (next_head.x == mySnake.body[i].x && next_head.y == mySnake.body[i].y) {
                printf("[DEAD] Self-collision at (%d, %d)\r\n", next_head.x, next_head.y);
                current_state = GAME_OVER;
                Play_Death_Alert();
                return;
            }
        }
        
        // 吃到普通苹果（磁铁吸引时 3x3 范围有效，排除前方一格）
        if (attract_active ? (IN_RANGE(next_head.x, next_head.y, myFood.x, myFood.y) && (myFood.x != front_x || myFood.y != front_y))
                           : (next_head.x == myFood.x && next_head.y == myFood.y)) {
            ate_normal = 1;
            // 磁铁远程吃到时，需显式擦除旧食物（蛇头不在该格）
            if (attract_active && !(next_head.x == myFood.x && next_head.y == myFood.y))
                Clear_Grid_Cell(myFood.x, myFood.y);
            LED1 = 0; BEEP_ON(); delay_ms(30); BEEP = 0; LED1 = 1;
        }
        
        // 吃到金色苹果（2 倍分数 + 5 秒减速；磁铁吸引时 3x3 范围有效，排除前方一格）
        if (gold_food_active && (attract_active ? (IN_RANGE(next_head.x, next_head.y, myGoldFood.x, myGoldFood.y) && (myGoldFood.x != front_x || myGoldFood.y != front_y))
                                                : (next_head.x == myGoldFood.x && next_head.y == myGoldFood.y))) {
            ate_golden = 1;
            gold_food_active = 0;
            Clear_Grid_Cell(myGoldFood.x, myGoldFood.y);
            slow_active = 1;
            slow_ticks = 500;
            printf("[GOLD] Eaten! +20 pts, speed slowed 5s\r\n");
            LED1 = 0; BEEP_ON(); delay_ms(30); BEEP = 0; LED1 = 1;
        }
        
        // 吃到磁铁（15 秒吸引效果；3x3 范围有效，排除前方一格）
        if (magnet_active && (attract_active ? (IN_RANGE(next_head.x, next_head.y, myMagnet.x, myMagnet.y) && (myMagnet.x != front_x || myMagnet.y != front_y))
                                             : (next_head.x == myMagnet.x && next_head.y == myMagnet.y))) {
            magnet_active = 0;
            Clear_Grid_Cell(myMagnet.x, myMagnet.y);
            attract_active = 1;
            attract_ticks = 1500;
            printf("[MAGNET] Eaten! Attraction active 15s\r\n");
            LED1 = 0; BEEP_ON(); delay_ms(30); BEEP = 0; LED1 = 1;
        }
        
        // 蛇身增长与分数（磁铁不增长蛇身、不加分）
        uint8_t length_before = mySnake.length;
        if (ate_normal || ate_golden) {
            if (mySnake.length < MAX_SNAKE_LEN) mySnake.length++;
            score += ate_golden ? 20 : 10;
            if (ate_normal) food_eaten++;
            if (ate_golden) food_eaten++;
        }
        
        for (i = mySnake.length - 1; i > 0; i--) {
            mySnake.body[i] = mySnake.body[i - 1];
        }
        mySnake.body[0] = next_head;
        
        // 擦尾：蛇未实际变长时擦除旧尾（含 length 封顶、磁铁等情况）
        if (mySnake.length == length_before) {
            Clear_Grid_Cell(old_tail.x, old_tail.y);
        }
        if (ate_normal) {
            Generate_Food();
        }
        
        Draw_Snake_Body(mySnake.body[1].x, mySnake.body[1].y);
        Draw_Snake_Head(mySnake.body[0].x, mySnake.body[0].y);
    }
}
