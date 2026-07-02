#include "snake.h"
#include "beep.h"
#include "delay.h" 
#include "led.h"    
#include <stdlib.h> 
#include "usart.h"

// ==================== 0. 全局变量定义（已全部补齐，解决 L6218E 链接错误） ====================
Game_State current_state = GAME_MENU;
Difficulty game_difficulty = DIFF_MEDIUM; // 修复：game_difficulty 全局定义
Game_Mode game_mode = MODE_SINGLE;         // 修复：game_mode 全局定义
Snake mySnake;
Snake mySnake2;                           // 2 号蛇变量
Point myFood;                             // 修复：myFood 全局定义
uint32_t score = 0;
// 新增：定义全局对战赢家变量
uint8_t winner = 0;

/* ==================== 1. 像素级精细化绘制函数组 ==================== */

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
    LCD_Fill(sx + 1, sy + 1, sx + GRID_SIZE - 2, sy + GRID_SIZE - 2, COLOR_SNAKE_H);
    
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

// 2号蛇身：青色方块（内缩1像素黑边，凸显关节）
static void Draw_Snake2_Body(int16_t x, int16_t y) {
    uint16_t sx = GAME_X_START + x * GRID_SIZE;
    uint16_t sy = GAME_Y_START + y * GRID_SIZE;
    LCD_Fill(sx, sy, sx + GRID_SIZE - 1, sy + GRID_SIZE - 1, COLOR_BG);
    LCD_Fill(sx + 1, sy + 1, sx + GRID_SIZE - 2, sy + GRID_SIZE - 2, COLOR_SNAKE2_B);
}

// 2号蛇头：蓝色圆角方块 + 雪白双眼（在深蓝色下更明显）
static void Draw_Snake2_Head(int16_t x, int16_t y) {
    uint16_t sx = GAME_X_START + x * GRID_SIZE;
    uint16_t sy = GAME_Y_START + y * GRID_SIZE;
    
    LCD_Fill(sx, sy, sx + GRID_SIZE - 1, sy + GRID_SIZE - 1, COLOR_BG);
    LCD_Fill(sx + 1, sy + 1, sx + GRID_SIZE - 2, sy + GRID_SIZE - 2, COLOR_SNAKE2_H);
    
    switch (mySnake2.direction) {
        case DIR_UP:
            LCD_Fill(sx + 5, sy + 5, sx + 8, sy + 8, WHITE);   
            LCD_Fill(sx + 15, sy + 5, sx + 18, sy + 8, WHITE); 
            break;
        case DIR_DOWN:
            LCD_Fill(sx + 5, sy + 15, sx + 8, sy + 18, WHITE);  
            LCD_Fill(sx + 15, sy + 15, sx + 18, sy + 18, WHITE); 
            break;
        case DIR_LEFT:
            LCD_Fill(sx + 5, sy + 5, sx + 8, sy + 8, WHITE);   
            LCD_Fill(sx + 5, sy + 15, sx + 8, sy + 18, WHITE);  
            break;
        case DIR_RIGHT:
            LCD_Fill(sx + 15, sy + 5, sx + 18, sy + 18, WHITE);  
            LCD_Fill(sx + 15, sy + 15, sx + 18, sy + 18, WHITE); 
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

/* ==================== 2. 核心游戏控制与双蛇生存逻辑 ==================== */

// 共享食物生成
static void Generate_Food(void) {
    uint8_t on_snake = 1;
    uint16_t i; 
    
    while (on_snake) {
        on_snake = 0;
        myFood.x = rand() % GAME_GRID_NUM_X;
        myFood.y = rand() % GAME_GRID_NUM_Y;
        
        // 1. 检查是否生成在 1 号蛇身上
        for (i = 0; i < mySnake.length; i++) {
            if (mySnake.body[i].x == myFood.x && mySnake.body[i].y == myFood.y) {
                on_snake = 1; 
                break;
            }
        }
        
        // 2. 双人模式下，检查是否生成在 2 号蛇身上
        if (game_mode == MODE_BATTLE && !on_snake) {
            for (i = 0; i < mySnake2.length; i++) {
                if (mySnake2.body[i].x == myFood.x && mySnake2.body[i].y == myFood.y) {
                    on_snake = 1; 
                    break;
                }
            }
        }
    }
    Draw_Food_Apple(myFood.x, myFood.y); 
    printf("[FOOD] Generated at (%d, %d)\r\n", myFood.x, myFood.y);
}

// 改变 1 号蛇方向
void Snake_Change_Direction(uint8_t new_dir) {
    if (new_dir == DIR_UP && mySnake.direction != DIR_DOWN) mySnake.direction = DIR_UP;
    if (new_dir == DIR_DOWN && mySnake.direction != DIR_UP) mySnake.direction = DIR_DOWN;
    if (new_dir == DIR_LEFT && mySnake.direction != DIR_RIGHT) mySnake.direction = DIR_LEFT;
    if (new_dir == DIR_RIGHT && mySnake.direction != DIR_LEFT) mySnake.direction = DIR_RIGHT;
}

// 修复：改变 2 号蛇的行进方向（解决 Snake2_Change_Direction 链接错误）
void Snake2_Change_Direction(uint8_t new_dir) {
    if (new_dir == DIR_UP && mySnake2.direction != DIR_DOWN) mySnake2.direction = DIR_UP;
    if (new_dir == DIR_DOWN && mySnake2.direction != DIR_UP) mySnake2.direction = DIR_DOWN;
    if (new_dir == DIR_LEFT && mySnake2.direction != DIR_RIGHT) mySnake2.direction = DIR_LEFT;
    if (new_dir == DIR_RIGHT && mySnake2.direction != DIR_LEFT) mySnake2.direction = DIR_RIGHT;
}

// 修复：键盘单字符拦截解析器函数（解决 Snake2_Change_Direction_By_Char 链接错误）
uint8_t Snake2_Change_Direction_By_Char(char c) {
    if (current_state == GAME_RUNNING && game_mode == MODE_BATTLE) {
        if (c == 'w' || c == 'W') { Snake2_Change_Direction(DIR_UP);    return 1; }
        if (c == 's' || c == 'S') { Snake2_Change_Direction(DIR_DOWN);  return 1; }
        if (c == 'a' || c == 'A') { Snake2_Change_Direction(DIR_LEFT);  return 1; }
        if (c == 'd' || c == 'D') { Snake2_Change_Direction(DIR_RIGHT); return 1; }
    }
    return 0; 
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

// 单/双人模式数据差异化初始化（已更新为：对角安全平行出生）
void Snake_Game_Init(void) {
    uint16_t i; 

    LED0 = 1; 
    LED1 = 1;
    score = 0;
    
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
        // ================== A. 单人模式初始化 ==================
        mySnake.length = 3;
        mySnake.direction = DIR_RIGHT;
        
        mySnake.body[0].x = GAME_GRID_NUM_X / 2;     
        mySnake.body[0].y = GAME_GRID_NUM_Y / 2;
        
        mySnake.body[1].x = mySnake.body[0].x - 1;   
        mySnake.body[1].y = mySnake.body[0].y;
        
        mySnake.body[2].x = mySnake.body[0].x - 2;   
        mySnake.body[2].y = mySnake.body[0].y;
        
        // 绘制单人蛇
        Draw_Snake_Head(mySnake.body[0].x, mySnake.body[0].y); 
        for (i = 1; i < mySnake.length; i++) {
            Draw_Snake_Body(mySnake.body[i].x, mySnake.body[i].y); 
        }
    } 
    else 
    {
        // ================== B. 双人对战模式初始化 (对角平行安全出生) ==================
        // 1号玩家（红色，左上角）：向下出生
        mySnake.length = 3;
        mySnake.direction = DIR_DOWN;
        mySnake.body[0].x = 2;     
        mySnake.body[0].y = 2; // 蛇头
        mySnake.body[1].x = 2;   
        mySnake.body[1].y = 1; // 身体1
        mySnake.body[2].x = 2;   
        mySnake.body[2].y = 0; // 身体2
        
        // 2号玩家（蓝色，右下角）：向上出生（GAME_GRID_NUM_X - 3 即第 15 列， GAME_GRID_NUM_Y - 3 即第 27 行）
        mySnake2.length = 3;
        mySnake2.direction = DIR_UP;
        mySnake2.body[0].x = GAME_GRID_NUM_X - 3;     
        mySnake2.body[0].y = GAME_GRID_NUM_Y - 3; // 蛇头
        mySnake2.body[1].x = mySnake2.body[0].x;   
        mySnake2.body[1].y = mySnake2.body[0].y + 1; // 身体在头下方
        mySnake2.body[2].x = mySnake2.body[0].x;   
        mySnake2.body[2].y = mySnake2.body[0].y + 2;
        
        // 绘制 1 号玩家蛇（红/绿）
        Draw_Snake_Head(mySnake.body[0].x, mySnake.body[0].y); 
        for (i = 1; i < mySnake.length; i++) {
            Draw_Snake_Body(mySnake.body[i].x, mySnake.body[i].y); 
        }
        
        // 绘制 2 号玩家蛇（蓝/青）
        Draw_Snake2_Head(mySnake2.body[0].x, mySnake2.body[0].y); 
        for (i = 1; i < mySnake2.length; i++) {
            Draw_Snake2_Body(mySnake2.body[i].x, mySnake2.body[i].y); 
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
    
    if (game_mode == MODE_BATTLE) {
        for (i = 1; i < mySnake2.length; i++) {
            Draw_Snake2_Body(mySnake2.body[i].x, mySnake2.body[i].y);
        }
        Draw_Snake2_Head(mySnake2.body[0].x, mySnake2.body[0].y);
    }
    
    Draw_Food_Apple(myFood.x, myFood.y);
}

// 核心时钟节拍：单人/双人并行动画与全方位碰撞致死算法
void Snake_Game_Tick(void) {
    uint16_t i;

    if (current_state != GAME_RUNNING) return;

    if (game_mode == MODE_SINGLE) 
    {
        // ================== A. 单人模式运行逻辑 (保持原有优化) ==================
        Point old_tail;
        Point next_head;
        uint8_t ate_food = 0;

        old_tail = mySnake.body[mySnake.length - 1];
        next_head = mySnake.body[0];
        
        switch (mySnake.direction) {
            case DIR_UP:    next_head.y--; break;
            case DIR_DOWN:  next_head.y++; break;
            case DIR_LEFT:  next_head.x--; break;
            case DIR_RIGHT: next_head.x++; break;
        }
        
        // 碰撞：撞墙
        if (next_head.x < 0 || next_head.x >= GAME_GRID_NUM_X ||
            next_head.y < 0 || next_head.y >= GAME_GRID_NUM_Y) {
            printf("[DEAD] Hit wall at (%d, %d)\r\n", next_head.x, next_head.y);
            current_state = GAME_OVER;
            winner = 0; 
            Play_Death_Alert();
            return;
        }
        
        // 碰撞：撞自身
        for (i = 1; i < mySnake.length - 1; i++) {
            if (next_head.x == mySnake.body[i].x && next_head.y == mySnake.body[i].y) {
                printf("[DEAD] Self-collision at (%d, %d)\r\n", next_head.x, next_head.y);
                current_state = GAME_OVER;
                winner = 0;
                Play_Death_Alert();
                return;
            }
        }
        
        // 吃到苹果
        if (next_head.x == myFood.x && next_head.y == myFood.y) {
            ate_food = 1;
            LED1 = 0; // 绿灯闪
            BEEP_ON(); delay_ms(30); BEEP = 0; 
            LED1 = 1;
        }
        
        if (ate_food) {
            if (mySnake.length < MAX_SNAKE_LEN) mySnake.length++;
            score += 10;
        }
        
        for (i = mySnake.length - 1; i > 0; i--) {
            mySnake.body[i] = mySnake.body[i - 1];
        }
        mySnake.body[0] = next_head;
        
        if (!ate_food) {
            Clear_Grid_Cell(old_tail.x, old_tail.y);
        } else {
            Generate_Food();
        }
        
        Draw_Snake_Body(mySnake.body[1].x, mySnake.body[1].y);
        Draw_Snake_Head(mySnake.body[0].x, mySnake.body[0].y);
    }
    else 
    {
        // ================== B. 双人对战模式运行逻辑 (双蛇并行算法) ==================
        Point old_tail_1, old_tail_2;
        Point next_head_1, next_head_2;
        uint8_t p1_dead = 0;
        uint8_t p2_dead = 0;
        uint8_t ate_food_1 = 0;
        uint8_t ate_food_2 = 0;

        old_tail_1 = mySnake.body[mySnake.length - 1];
        old_tail_2 = mySnake2.body[mySnake2.length - 1];

        next_head_1 = mySnake.body[0];
        next_head_2 = mySnake2.body[0];

        // 1. 计算双蛇下一帧预计位置
        switch (mySnake.direction) {
            case DIR_UP:    next_head_1.y--; break;
            case DIR_DOWN:  next_head_1.y++; break;
            case DIR_LEFT:  next_head_1.x--; break;
            case DIR_RIGHT: next_head_1.x++; break;
        }
        switch (mySnake2.direction) {
            case DIR_UP:    next_head_2.y--; break;
            case DIR_DOWN:  next_head_2.y++; break;
            case DIR_LEFT:  next_head_2.x--; break;
            case DIR_RIGHT: next_head_2.x++; break;
        }

        // 2. 碰撞算法 A：边界撞墙判定
        if (next_head_1.x < 0 || next_head_1.x >= GAME_GRID_NUM_X ||
            next_head_1.y < 0 || next_head_1.y >= GAME_GRID_NUM_Y) p1_dead = 1;

        if (next_head_2.x < 0 || next_head_2.x >= GAME_GRID_NUM_X ||
            next_head_2.y < 0 || next_head_2.y >= GAME_GRID_NUM_Y) p2_dead = 1;

        // 3. 碰撞算法 B：双蛇头对头迎面相撞
        if (next_head_1.x == next_head_2.x && next_head_1.y == next_head_2.y) {
            p1_dead = 1;
            p2_dead = 1;
        }

        // 4. 碰撞算法 C：1号蛇（红色）碰撞检查
        if (!p1_dead) {
            // 是否咬到自己
            for (i = 1; i < mySnake.length - 1; i++) {
                if (next_head_1.x == mySnake.body[i].x && next_head_1.y == mySnake.body[i].y) {
                    p1_dead = 1; break;
                }
            }
            // 是否撞上 2 号蛇（蓝色）身体的任意一节
            for (i = 0; i < mySnake2.length; i++) {
                if (next_head_1.x == mySnake2.body[i].x && next_head_1.y == mySnake2.body[i].y) {
                    p1_dead = 1; break;
                }
            }
        }

        // 5. 碰撞算法 D：2号蛇（蓝色）碰撞检查
        if (!p2_dead) {
            // 是否咬到自己
            for (i = 1; i < mySnake2.length - 1; i++) {
                if (next_head_2.x == mySnake2.body[i].x && next_head_2.y == mySnake2.body[i].y) {
                    p2_dead = 1; break;
                }
            }
            // 是否撞上 1 号蛇（红色）身体的任意一节
            for (i = 0; i < mySnake.length; i++) {
                if (next_head_2.x == mySnake.body[i].x && next_head_2.y == mySnake.body[i].y) {
                    p2_dead = 1; break;
                }
            }
        }

        // 6. 结算大赢家：若任一玩家死亡，锁定胜负并发出红色闪光警报
        if (p1_dead || p2_dead) {
            current_state = GAME_OVER;
            if (p1_dead && p2_dead) {
                winner = 0; // 同归于尽/平局
            } else if (p1_dead && !p2_dead) {
                winner = 2; // 1号死，2号蓝色获胜
            } else {
                winner = 1; // 2号死，1号红色获胜
            }
            Play_Death_Alert(); // 响起爆闪警报
            return;
        }

        // 7. 抢苹果判定
        if (next_head_1.x == myFood.x && next_head_1.y == myFood.y) {
            ate_food_1 = 1;
            LED1 = 0; // 红色抢到，绿色LED闪
            BEEP_ON(); delay_ms(30); BEEP = 0;
            LED1 = 1;
        }
        if (next_head_2.x == myFood.x && next_head_2.y == myFood.y) {
            ate_food_2 = 1;
            LED0 = 0; // 蓝色抢到，红色LED闪（硬件区分反馈）
            BEEP_ON(); delay_ms(30); BEEP = 0;
            LED0 = 1;
        }

        // 8. 蛇关节增长
        if (ate_food_1) {
            if (mySnake.length < MAX_SNAKE_LEN) mySnake.length++;
            score += 10; // 共用分数栏累计
        }
        if (ate_food_2) {
            if (mySnake2.length < MAX_SNAKE_LEN) mySnake2.length++;
            score += 10; 
        }

        // 9. 物理坐标更新：双蛇独立位移
        for (i = mySnake.length - 1; i > 0; i--) {
            mySnake.body[i] = mySnake.body[i - 1];
        }
        mySnake.body[0] = next_head_1;

        for (i = mySnake2.length - 1; i > 0; i--) {
            mySnake2.body[i] = mySnake2.body[i - 1];
        }
        mySnake2.body[0] = next_head_2;

        // 10. 增量局部刷新优化（零闪烁，高流畅）
        if (!ate_food_1) {
            Clear_Grid_Cell(old_tail_1.x, old_tail_1.y);
        }
        if (!ate_food_2) {
            Clear_Grid_Cell(old_tail_2.x, old_tail_2.y);
        }
        
        // 如果任意一条蛇吃到苹果，重新生成共享食物
        if (ate_food_1 || ate_food_2) {
            Generate_Food();
        }

        // 重绘 1 号蛇头部和身体关节
        Draw_Snake_Body(mySnake.body[1].x, mySnake.body[1].y);
        Draw_Snake_Head(mySnake.body[0].x, mySnake.body[0].y);

        // 重绘 2 号蛇头部和身体关节
        Draw_Snake2_Body(mySnake2.body[1].x, mySnake2.body[1].y);
        Draw_Snake2_Head(mySnake2.body[0].x, mySnake2.body[0].y);
    }
}
