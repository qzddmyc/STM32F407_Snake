#include "sys.h"
#include "delay.h"
#include "usart.h"
#include "lcd.h"
#include "key.h"
#include "beep.h"
#include "snake.h"
#include "eeprom.h"
#include "led.h"
#include <string.h>

extern uint32_t score;
// extern uint8_t winner; // 【已移除双人对战】引入 snake.c 中的赢家标志位

// 探索者开发板按键物理键值
#define KEY0_PRES    1  // 最右侧按键 (PE4) -> 菜单：右选择  | 游戏：向右
#define KEY1_PRES    2  // 中右侧按键 (PE3) -> 菜单：切换光标 | 游戏：向下
#define KEY2_PRES    3  // 中左侧按键 (PE2) -> 菜单：左选择  | 游戏：向左
#define WKUP_PRES    4  // 最左侧按键 (PA0) -> 菜单：确认开始 | 游戏：向上

// 内部辅助函数：局部更新“难度选择”菜单（无闪烁）
void Draw_Difficulty_Menu(Difficulty diff, uint8_t focus) 
{
    BACK_COLOR = BLACK;
    
    // 如果焦点在难度选择上，大标题显示为白色，否则为灰色
    if (focus == 1) POINT_COLOR = WHITE;
    else POINT_COLOR = GRAY;
    LCD_ShowString(140, 280, 200, 16, 16, "Select Difficulty:");
    
    // 简单难度
    if (diff == DIFF_EASY) {
        POINT_COLOR = (focus == 1) ? GREEN : GRAY; // 聚焦时绿色，失焦时灰色
        LCD_ShowString(180, 310, 150, 16, 16, ">  EASY  <");
    } else {
        POINT_COLOR = GRAY;
        LCD_ShowString(180, 310, 150, 16, 16, "   EASY   ");
    }
    
    // 中等难度
    if (diff == DIFF_MEDIUM) {
        POINT_COLOR = (focus == 1) ? YELLOW : GRAY; // 聚焦时黄色
        LCD_ShowString(180, 340, 150, 16, 16, "> MEDIUM <");
    } else {
        POINT_COLOR = GRAY;
        LCD_ShowString(180, 340, 150, 16, 16, "  MEDIUM  ");
    }
    
    // 困难难度
    if (diff == DIFF_HARD) {
        POINT_COLOR = (focus == 1) ? RED : GRAY; // 聚焦时红色
        LCD_ShowString(180, 370, 150, 16, 16, ">  HARD  <");
    } else {
        POINT_COLOR = GRAY;
        LCD_ShowString(180, 370, 150, 16, 16, "   HARD   ");
    }
}

// 新增内部辅助函数：局部更新“游戏模式”菜单（无闪烁）
void Draw_Mode_Menu(Game_Mode mode, uint8_t focus)
{
    BACK_COLOR = BLACK;
    
    // 如果焦点在模式选择上，标题显示为白色，否则为灰色
    if (focus == 1) POINT_COLOR = WHITE;
    else POINT_COLOR = GRAY;
    LCD_ShowString(140, 440, 200, 16, 16, "Select Game Mode:");
    
    // 单人模式
    if (mode == MODE_SINGLE) {
        POINT_COLOR = (focus == 1) ? GREEN : GRAY;
        LCD_ShowString(160, 470, 200, 16, 16, "> SINGLE PLAYER <");
    } else {
        POINT_COLOR = GRAY;
        LCD_ShowString(160, 470, 200, 16, 16, "  SINGLE PLAYER  ");
    }
    
    // 【已移除双人对战】双人对战模式
    // if (mode == MODE_BATTLE) {
    //     POINT_COLOR = (focus == 1) ? CYAN : GRAY;
    //     LCD_ShowString(160, 500, 200, 16, 16, ">  BATTLE MODE  <");
    // } else {
    //     POINT_COLOR = GRAY;
    //     LCD_ShowString(160, 500, 200, 16, 16, "   BATTLE MODE   ");
    // }
}

// 内部辅助函数：局部更新"START"按钮（选中时实心白色+黑字，未选中时空心白框+白字）
void Draw_Start_Button(uint8_t focus)
{
    uint16_t game_area_w = GAME_GRID_NUM_X * GRID_SIZE;
    uint16_t btn_w = game_area_w * 2 / 10;   // 0.2 倍游戏区域宽度 = 86
    uint16_t btn_h = btn_w * 6 / 10;          // 0.6 倍按钮宽度 = 52
    uint16_t btn_x = (480 - btn_w) / 2 - 20;       // 水平居中
    uint16_t btn_y = (356 + 540 - btn_h) / 2; // 竖直居中于 HARD(356) 与提示(540) 之间
    uint16_t text_w = 5 * 8;                  // "START" 5 字符宽 40px
    uint16_t text_x = btn_x + (btn_w - text_w) / 2;
    uint16_t text_y = btn_y + (btn_h - 16) / 2;
    
    if (focus == 1) {
        // 选中：实心白色矩形 + 黑色文字
        LCD_Fill(btn_x, btn_y, btn_x + btn_w - 1, btn_y + btn_h - 1, WHITE);
        POINT_COLOR = BLACK;
        BACK_COLOR = WHITE;
    } else {
        // 未选中：空心白色矩形框 + 白色文字 + 黑色背景
        LCD_Fill(btn_x, btn_y, btn_x + btn_w - 1, btn_y + btn_h - 1, BLACK);
        POINT_COLOR = WHITE;
        LCD_DrawRectangle(btn_x, btn_y, btn_x + btn_w - 1, btn_y + btn_h - 1);
        POINT_COLOR = WHITE;
        BACK_COLOR = BLACK;
    }
    LCD_ShowString(text_x, text_y, text_w, 16, 16, "START");
}

static uint8_t menu_focus = 0; // 0: 难度选择聚焦， 1: Start 按钮聚焦

// 串口指令解析与执行（支持 \r\n 或无换行符两种形式）
void Serial_CMD_Handler(void)
{
    static uint16_t last_rx_len = 0;
    static uint8_t  rx_timeout = 0;
    uint16_t len;
    
    len = USART_RX_STA & 0x3FFF;
    if (len == 0) {
        last_rx_len = 0;
        rx_timeout = 0;
        return;
    }
    
    if (len != last_rx_len) {
        last_rx_len = len;
        rx_timeout = 0;
    }
    
    // 标准完成（\r\n）或超时（~100ms 无新数据）
    if (!(USART_RX_STA & 0x8000) && ++rx_timeout <= 10)
        return;
    
    USART_RX_BUF[len] = '\0';
    while (len > 0 && (USART_RX_BUF[len-1] == '\r' || USART_RX_BUF[len-1] == '\n'))
        USART_RX_BUF[--len] = '\0';
    
    if (strcmp((char*)USART_RX_BUF, "beep on") == 0) {
        beep_enable = 1;
        printf("[CMD] Beep: ON\r\n");
    } else if (strcmp((char*)USART_RX_BUF, "beep off") == 0) {
        beep_enable = 0;
        printf("[CMD] Beep: OFF\r\n");
    } else if (strcmp((char*)USART_RX_BUF, "diff easy") == 0) {
        if (current_state == GAME_MENU) {
            game_difficulty = DIFF_EASY;
            Draw_Difficulty_Menu(game_difficulty, (menu_focus == 0));
            printf("[CMD] Difficulty: EASY\r\n");
        } else {
            printf("[CMD] Difficulty change only allowed in MENU\r\n");
        }
    } else if (strcmp((char*)USART_RX_BUF, "diff medium") == 0) {
        if (current_state == GAME_MENU) {
            game_difficulty = DIFF_MEDIUM;
            Draw_Difficulty_Menu(game_difficulty, (menu_focus == 0));
            printf("[CMD] Difficulty: MEDIUM\r\n");
        } else {
            printf("[CMD] Difficulty change only allowed in MENU\r\n");
        }
    } else if (strcmp((char*)USART_RX_BUF, "diff hard") == 0) {
        if (current_state == GAME_MENU) {
            game_difficulty = DIFF_HARD;
            Draw_Difficulty_Menu(game_difficulty, (menu_focus == 0));
            printf("[CMD] Difficulty: HARD\r\n");
        } else {
            printf("[CMD] Difficulty change only allowed in MENU\r\n");
        }
    } else {
        printf("[CMD] Unknown: %s\r\n", USART_RX_BUF);
    }
    
    USART_RX_STA = 0;
    last_rx_len = 0;
    rx_timeout = 0;
}

int main(void)
{
    uint8_t key_val = 0;
    uint32_t timer_count = 0;
    
    // 变量定义（兼容 C89）
    Game_State last_state = GAME_OVER; 
    uint32_t last_score = 999;         
    uint32_t current_speed_threshold = 20; 
    uint32_t speed_level = 1;          
    uint16_t high_score = 0;           
    uint8_t is_new_record = 0;         

    uint32_t game_time_sec = 0;        
    uint32_t ms_count = 0;             
    uint32_t last_game_time_sec = 9999;
    uint32_t temp_mm = 0;              
    uint32_t temp_ss = 0;              

    // 1. 系统外设初始化
    delay_init(168);     
    uart_init(115200);   
    LCD_Init();          
    KEY_Init();          
    BEEP_Init();         
    LED_Init();          
    EEPROM_I2C_Init();   

    // 2. 硬件与引脚电平稳定延时
    delay_ms(300);       
    KEY_Scan(0);         

    // 3. 读取最高记录
    high_score = 0;//EEPROM_Read_HighScore();

    // 4. 设置竖屏
    LCD_Display_Dir(0);  
    LCD_Clear(BLACK);
    BACK_COLOR = BLACK; 

    printf("[INIT] Snake game ready\r\n"); // 验证 printf 基本输出

    while(1)
    {
        delay_ms(10); 
        key_val = KEY_Scan(0); 
        Serial_CMD_Handler();

        // ================== 组合键检测（KEY2 和 KEY0 同时按下） ==================
        if (current_state == GAME_RUNNING && KEY2 == 0 && KEY0 == 0) 
        {
            current_state = GAME_PAUSED;
            BEEP_ON(); delay_ms(80); BEEP = 0; 
            while (KEY2 == 0 || KEY0 == 0) {
                delay_ms(10);
            }
        }

        // ================== 状态刷新与悬浮窗局部刷新逻辑 ==================
        if (current_state != last_state) 
        {
            if (current_state == GAME_PAUSED) 
            {
                POINT_COLOR = COLOR_WALL;
                LCD_DrawRectangle(99, 299, 381, 501); 
                LCD_Fill(100, 300, 380, 500, BLACK);   
                
                POINT_COLOR = YELLOW;
                BACK_COLOR = BLACK;
                LCD_ShowString(185, 320, 150, 24, 24, "- PAUSED -");
                
                POINT_COLOR = WHITE;
                LCD_ShowString(110, 385, 260, 16, 16, "WK_UP : RESUME (Keep)");
                LCD_ShowString(110, 425, 260, 16, 16, "KEY1  : EXIT (Back)");
            }
            else if (current_state == GAME_RUNNING && last_state == GAME_PAUSED)	//从暂停界面回到游戏
            {
                LCD_Fill(99, 299, 381, 501, BLACK);
                Snake_Redraw();
            }
            else 
            {
                LCD_Clear(BLACK); 
                
                if (current_state == GAME_MENU) 
                {
                    menu_focus = 0; // 每次回到菜单，默认光标停留在难度选择上
                    
                    POINT_COLOR = RED;
                    LCD_ShowString(140, 120, 200, 24, 24, "SNAKE GAME");
                    
                    POINT_COLOR = YELLOW;
                    LCD_ShowString(170, 180, 200, 16, 16, "BEST: ");
                    LCD_ShowNum(230, 180, high_score, 3, 16);
                    
                    // POINT_COLOR = WHITE;
                    // LCD_ShowString(110, 240, 300, 16, 16, "Press WK_UP to Start");
                    
                    // 局部更新选择菜单（难度选择 + Start 按钮）
                    Draw_Difficulty_Menu(game_difficulty, 1);
                    Draw_Start_Button(0); // 初始未选中 Start
                    
                    POINT_COLOR = GRAY;
                    LCD_ShowString(100, 540, 300, 16, 16, "WK_UP : Move UP      KEY1: Move DOWN");      
                    LCD_ShowString(100, 560, 300, 16, 16, "KEY2  : Move LEFT    KEY0: Move RIGHT");    
                    
                    POINT_COLOR = WHITE;
                    LCD_ShowString(100, 610, 300, 16, 16, "Menu Controls:");
                    POINT_COLOR = GRAY;
                    LCD_ShowString(100, 630, 300, 16, 16, "KEY2/KEY0: Move Focus (Left/Right)");
                    LCD_ShowString(100, 650, 320, 16, 16, "WK_UP/KEY1: Change Difficulty (Up/Down)");
                    LCD_ShowString(100, 670, 300, 16, 16, "KEY0 on START: Confirm & Play");
                } 
                else if (current_state == GAME_RUNNING) //从主菜单来到游戏
                {
                    POINT_COLOR = COLOR_WALL;
                    LCD_DrawRectangle(
                        GAME_X_START - 1, 
                        GAME_Y_START - 1, 
                        GAME_X_START + GAME_GRID_NUM_X * GRID_SIZE, 
                        GAME_Y_START + GAME_GRID_NUM_Y * GRID_SIZE
                    );
                    
                    Snake_Game_Init(); 
                    timer_count = 0; 
                    last_score = 999; 
                    is_new_record = 0; 
                    
                    game_time_sec = 0;
                    ms_count = 0;
                    last_game_time_sec = 9999; 
                } 
                else if (current_state == GAME_OVER) 
                {
                    // 1. 如果是单人模式，进行历史最高纪录结算与断电保存
                    if (game_mode == MODE_SINGLE) 
                    {
                        if (score > high_score) 
                        {
                            high_score = score;
                            EEPROM_Write_HighScore(high_score); 
                            is_new_record = 1;
                            
                            BEEP_ON(); delay_ms(60); BEEP = 0; delay_ms(60);
                            BEEP_ON(); delay_ms(60); BEEP = 0; delay_ms(60);
                            BEEP_ON(); delay_ms(60); BEEP = 0;
                        }
                    }

                    // 2. 绘制结算页
                    POINT_COLOR = RED;
                    LCD_ShowString(140, 160, 200, 24, 24, "GAME OVER");
                    
                    if (game_mode == MODE_SINGLE) 
                    {
                        // 单人模式：显示得分、时间、新纪录标签
                        POINT_COLOR = WHITE;
                        LCD_ShowString(160, 220, 200, 16, 16, "Score: ");
                        LCD_ShowNum(220, 220, score, 3, 16); 
                        
                        temp_mm = game_time_sec / 60;
                        temp_ss = game_time_sec % 60;
                        LCD_ShowString(160, 260, 200, 16, 16, "Time:  ");
                        LCD_ShowNum(220, 260, temp_mm, 2, 16);
                        LCD_ShowString(236, 260, 10, 16, 16, ":");
                        LCD_ShowNum(244, 260, temp_ss, 2, 16);
                        
                        if (is_new_record) 
                        {
                            POINT_COLOR = YELLOW;
                            LCD_ShowString(140, 310, 200, 16, 16, "NEW RECORD!");
                        }
                    }
                    // 【已移除双人对战】else 
                    // {
                    //     // 双人对战模式：显示争夺总分，并炫酷点出赢家！
                    //     POINT_COLOR = WHITE;
                    //     LCD_ShowString(160, 220, 200, 16, 16, "Total: "); // 同屏抢夺总分
                    //     LCD_ShowNum(220, 220, score, 3, 16);
                    //     
                    //     // 根据 winner 的值，用不同颜色的大字画出最终赢家
                    //     if (winner == 1) {
                    //         POINT_COLOR = GREEN; // 1号玩家胜，亮绿色大字
                    //         LCD_ShowString(140, 270, 220, 24, 24, "RED PLAYER WINS!");
                    //     } else if (winner == 2) {
                    //         POINT_COLOR = CYAN;  // 2号玩家胜，亮青色大字
                    //         LCD_ShowString(140, 270, 220, 24, 24, "BLUE PLAYER WINS!");
                    //     } else {
                    //         POINT_COLOR = YELLOW;// 战平同归于尽
                    //         LCD_ShowString(160, 270, 200, 24, 24, "DRAW GAME!");
                    //     }
                    // }
                    
                    POINT_COLOR = WHITE;
                    LCD_ShowString(110, 380, 300, 16, 16, "Press KEY0 to Restart");
                }
            }
            last_state = current_state; 
        }

        // ================== 游戏主状态机 ==================
        switch (current_state) 
        {
            case GAME_MENU:
                // KEY2（左）：焦点左移。Start → 难度选择；难度选择 → 无反应（左边界）
                if (key_val == KEY2_PRES) 
                {
                    if (menu_focus == 1) // 当前在 Start，左移到难度选择
                    {
                        menu_focus = 0;
                        Draw_Start_Button(0);
                        Draw_Difficulty_Menu(game_difficulty, 1);
                        BEEP_ON(); delay_ms(30); BEEP = 0;
                    }
                    // menu_focus == 0（难度选择）：无反应，已到达左边界
                }
                
                // KEY0（右）：焦点右移 / 确认进入游戏
                if (key_val == KEY0_PRES) 
                {
                    if (menu_focus == 0) // 当前在难度选择，右移到 Start
                    {
                        menu_focus = 1;
                        Draw_Difficulty_Menu(game_difficulty, 0);
                        Draw_Start_Button(1);
                        BEEP_ON(); delay_ms(30); BEEP = 0;
                    }
                    else if (menu_focus == 1) // 当前在 Start，确认进入游戏
                    {
                        current_state = GAME_RUNNING;
                        BEEP_ON(); delay_ms(30); BEEP = 0;
                    }
                }
                
                // WK_UP（上）：仅在难度选择聚焦时，向上调整难度（HARD→MEDIUM→EASY，EASY为上边界）
                if (key_val == WKUP_PRES && menu_focus == 0) 
                {
                    if (game_difficulty == DIFF_HARD) {
                        game_difficulty = DIFF_MEDIUM;
                        Draw_Difficulty_Menu(game_difficulty, 1);
                        BEEP_ON(); delay_ms(30); BEEP = 0;
                    } else if (game_difficulty == DIFF_MEDIUM) {
                        game_difficulty = DIFF_EASY;
                        Draw_Difficulty_Menu(game_difficulty, 1);
                        BEEP_ON(); delay_ms(30); BEEP = 0;
                    }
                    // DIFF_EASY：无反应，已到达上边界
                }
                
                // KEY1（下）：仅在难度选择聚焦时，向下调整难度（EASY→MEDIUM→HARD，HARD为下边界）
                if (key_val == KEY1_PRES && menu_focus == 0) 
                {
                    if (game_difficulty == DIFF_EASY) {
                        game_difficulty = DIFF_MEDIUM;
                        Draw_Difficulty_Menu(game_difficulty, 1);
                        BEEP_ON(); delay_ms(30); BEEP = 0;
                    } else if (game_difficulty == DIFF_MEDIUM) {
                        game_difficulty = DIFF_HARD;
                        Draw_Difficulty_Menu(game_difficulty, 1);
                        BEEP_ON(); delay_ms(30); BEEP = 0;
                    }
                    // DIFF_HARD：无反应，已到达下边界
                }
                break;

            case GAME_RUNNING:
                // 按键映射
                if (key_val == WKUP_PRES) Snake_Change_Direction(DIR_UP);    
                if (key_val == KEY2_PRES) Snake_Change_Direction(DIR_LEFT);  
                if (key_val == KEY1_PRES) Snake_Change_Direction(DIR_DOWN);
                if (key_val == KEY0_PRES) Snake_Change_Direction(DIR_RIGHT);

                // HUD 局部刷新与变速公式
                if (score != last_score) 
                {
                    uint32_t food_eaten = score / 10;
                    
                    if (game_difficulty == DIFF_EASY) 
                    {
                        uint32_t decrease = food_eaten / 2;
                        if (decrease < 15) {
                            current_speed_threshold = 25 - decrease;
                        } else {
                            current_speed_threshold = 10;
                        }
                        speed_level = 26 - current_speed_threshold; 
                    } 
                    else if (game_difficulty == DIFF_MEDIUM) 
                    {
                        if (food_eaten < 13) {
                            current_speed_threshold = 18 - food_eaten;
                        } else {
                            current_speed_threshold = 5;
                        }
                        speed_level = 19 - current_speed_threshold; 
                    } 
                    else 
                    {
                        if (food_eaten < 8) {
                            current_speed_threshold = 11 - food_eaten;
                        } else {
                            current_speed_threshold = 3; 
                        }
                        speed_level = 12 - current_speed_threshold; 
                    }

                    POINT_COLOR = WHITE;
                    BACK_COLOR = BLACK; 

                    LCD_ShowString(30, 12, 100, 16, 16, "Score: ");
                    LCD_ShowNum(90, 12, score, 3, 16);

                    LCD_ShowString(300, 12, 100, 16, 16, "Speed: L");
                    LCD_ShowNum(370, 12, speed_level, 2, 16);

                    printf("[SCORE] %d pts | Speed L%d (threshold=%d)\r\n",
                           (int)score, (int)speed_level, (int)current_speed_threshold);

                    last_score = score;
                }

                // 生存时间计数 (10ms 步进)
                ms_count += 10;
                if (ms_count >= 1000) 
                {
                    ms_count = 0;
                    game_time_sec++;
                }

                // HUD 生存时间刷新 (防闪烁)
                if (game_time_sec != last_game_time_sec) 
                {
                    uint32_t mm = game_time_sec / 60;
                    uint32_t ss = game_time_sec % 60;
                    
                    POINT_COLOR = WHITE;
                    BACK_COLOR = BLACK;
                    
                    if (last_game_time_sec == 9999) 
                    {
                        LCD_ShowString(150, 12, 100, 16, 16, "TIME: ");
                        LCD_ShowString(214, 12, 10, 16, 16, ":");
                    }
                    
                    LCD_ShowNum(198, 12, mm, 2, 16);
                    LCD_ShowNum(222, 12, ss, 2, 16);
                    
                    last_game_time_sec = game_time_sec;
                }

                timer_count++;
                if (timer_count >= current_speed_threshold) 
                {
                    timer_count = 0;
                    Snake_Game_Tick(); 
                }
                break;

            case GAME_PAUSED:
                if (key_val == WKUP_PRES) 
                {
                    current_state = GAME_RUNNING;
                    BEEP_ON(); delay_ms(30); BEEP = 0;
                }
                if (key_val == KEY1_PRES) 
                {
                    current_state = GAME_MENU;
                    BEEP_ON(); delay_ms(30); BEEP = 0;
                }
                break;

            case GAME_OVER:
                if (key_val == KEY0_PRES) 
                {
                    current_state = GAME_MENU;
                }
                break;
        }
    }
}
