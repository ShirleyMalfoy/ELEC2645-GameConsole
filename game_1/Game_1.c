#include "Game_1.h"
#include "LCD.h"
#include "Menu.h"
#include "main.h"
#include "InputHandler.h"
#include "ST7789V2_Driver.h"
#include "joystick.h"
#include "stm32l4xx_hal_gpio.h"
#include <stdio.h>

extern uint16_t adc_buf[2];
extern TIM_HandleTypeDef htim2;
extern ST7789V2_cfg_t cfg0;
extern Joystick_cfg_t joystick_cfg;
extern Joystick_t joystick_data;

#define BLACK   0x0000
#define WHITE   0xFFFF
#define RED     0xF800
#define GREEN   0x07E0
#define YELLOW  0xFFE0
#define GRAY    0x8410

#define BTN_PORT GPIOC
#define BTN_PIN  GPIO_PIN_2

#define SW_PORT  GPIOC
#define SW_PIN   GPIO_PIN_3

#define LED_PORT GPIOB
#define LED_PIN  GPIO_PIN_6

Game1_t game1;

int lane_x[3] = {40, 120, 200};
uint32_t rand_num = 12345;

uint32_t my_rand(void)
{
    rand_num = rand_num * 1103515245 + 12345;
    return (rand_num >> 16) & 0x7FFF;
}

int get_rand_lane(void)
{
    return my_rand() % 3;
}

int button_down(GPIO_TypeDef *port, uint16_t pin)
{
    if (HAL_GPIO_ReadPin(port, pin) == GPIO_PIN_RESET)
    {
        HAL_Delay(20);

        if (HAL_GPIO_ReadPin(port, pin) == GPIO_PIN_RESET)
        {
            return 1;
        }
    }

    return 0;
}

int get_joystick(void)
{
    int dir;

    Joystick_Read(&joystick_cfg, &joystick_data);

    if (joystick_data.direction == W)
    {
        dir = -1;
        HAL_Delay(120);
    }
    else if (joystick_data.direction == E)
    {
        dir = 1;
        HAL_Delay(120);
    }

    return dir;
}

void buzzer_on(int freq, int time)
{
    uint32_t tim_clk;
    uint32_t psc;
    uint32_t arr;

    tim_clk = HAL_RCC_GetPCLK1Freq();
    psc = 79;
    arr = (tim_clk / (psc + 1)) / freq - 1;

    __HAL_TIM_SET_PRESCALER(&htim2, psc);
    __HAL_TIM_SET_AUTORELOAD(&htim2, arr);
    __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_3, arr / 2);

    HAL_Delay(time);

    __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_3, 0);
}

void draw_car(int x, int y, uint16_t color)
{
    LCD_Draw_Rect(x - 9, y, 18, 24, color,1);
    LCD_Draw_Rect(x - 5, y + 5, 10, 12, LCD_COLOUR_0,1);
}

void draw_item(int x, int y)
{
    LCD_Draw_Rect(x - 5, y, 10, 10, LCD_COLOUR_6,1);
}

void draw_road(void)
{
    LCD_Fill_Buffer(BLACK);

    LCD_Draw_Line(80, 20, 80, 240, LCD_COLOUR_1);
    LCD_Draw_Line(160, 20, 160, 240, LCD_COLOUR_1);
    LCD_Draw_Line(0, 20, 240, 20, LCD_COLOUR_13);
}

void draw_score(void)
{
    char text[30];

    sprintf(text, "Score:%lu Lv:%lu", game1.score, game1.level);

    LCD_Draw_Rect(0, 0, 240, 18, BLACK,1);
    LCD_printString(text, 4, 2, WHITE, 1);
}

void draw_game_over(void)
{
    char text[30];

    LCD_Draw_Rect(20, 75, 200, 95, BLACK,1);

    LCD_printString("GAME OVER", 55, 85, RED, 3);

    sprintf(text, "Score: %lu", game1.score);
    LCD_printString(text, 75, 115, WHITE, 1);

    LCD_printString("PC2: Restart", 35, 140, WHITE, 2);
    LCD_printString("PC3: Menu", 35, 155, WHITE, 2);

    LCD_Refresh(&cfg0);
}

void reset_enemy(int i, int y)
{
    game1.enemies[i].lane = get_rand_lane();
    game1.enemies[i].y = y;
    game1.enemies[i].active = 1;
}

void Game1_Init(Game1_t *g)
{
    g->state = GAME_START;
    g->sound = SOUND_NONE;
    g->score = 0;
    g->level = 1;
    g->last_tick = 0;
    g->update_interval_ms = 120;
}

void Game1_Reset(Game1_t *g)
{
    g->state = GAME_RUNNING;
    g->sound = SOUND_NONE;
    g->score = 0;
    g->level = 1;
    g->last_tick = HAL_GetTick();
    g->update_interval_ms = 120;

    g->player.lane = 1;
    g->player.y = PLAYER_Y;

    reset_enemy(0, 50);
    reset_enemy(1, 120);

    g->item.active = 0;
    g->item.lane = 0;
    g->item.y = 0;

    HAL_GPIO_WritePin(LED_PORT, LED_PIN, GPIO_PIN_RESET);
}

void Game1_Update(Game1_t *g, int dir)
{
    int i;
    int player_x;
    int enemy_x;
    int item_x;
    int dy;

    if (g->state != GAME_RUNNING)
    {
        return;
    }

    if (HAL_GetTick() - g->last_tick < g->update_interval_ms)
    {
        return;
    }

    g->last_tick = HAL_GetTick();
    g->sound = SOUND_NONE;

    if (dir == -1 && g->player.lane > 0)
    {
        g->player.lane--;
    }

    if (dir == 1 && g->player.lane < 2)
    {
        g->player.lane++;
    }

    for (i = 0; i < 2; i++)
    {
        g->enemies[i].y = g->enemies[i].y + 8 + g->level;

        if (g->enemies[i].y > 240)
        {
            g->score++;
            reset_enemy(i, -20 - my_rand() % 100);
        }
    }

    if (g->item.active == 0)
    {
        if (my_rand() % 80 == 0)
        {
            g->item.active = 1;
            g->item.lane = get_rand_lane();
            g->item.y = -10;
        }
    }
    else
    {
        g->item.y = g->item.y + 7;

        if (g->item.y > 240)
        {
            g->item.active = 0;
        }
    }

    player_x = lane_x[g->player.lane];

    for (i = 0; i < 2; i++)
    {
        enemy_x = lane_x[g->enemies[i].lane];
        dy = g->enemies[i].y - g->player.y;

        if (enemy_x == player_x && dy < CAR_H && dy > -CAR_H)
        {
            g->state = GAME_OVER;
            g->sound = SOUND_CRASH;
            return;
        }
    }

    if (g->item.active == 1)
    {
        item_x = lane_x[g->item.lane];
        dy = g->item.y - g->player.y;

        if (item_x == player_x && dy < CAR_H && dy > -12)
        {
            g->score = g->score + ITEM_BONUS_SCORE;
            g->item.active = 0;
            g->sound = SOUND_ITEM;
        }
    }

    g->level = 1 + g->score / SCORE_LEVEL_STEP;

    if (g->level < 12)
    {
        g->update_interval_ms = 120 - (g->level - 1) * 6;
    }
    else
    {
        g->update_interval_ms = 55;
    }
}

void Game1_Draw(Game1_t *g)
{
    int i;

    LCD_Fill_Buffer(BLACK);

    draw_road();
    draw_score();

    for (i = 0; i < 2; i++)
    {
        draw_car(lane_x[g->enemies[i].lane], g->enemies[i].y, LCD_COLOUR_2);
    }

    if (g->item.active == 1)
    {
        draw_item(lane_x[g->item.lane], g->item.y);
    }

    draw_car(lane_x[g->player.lane], g->player.y, LCD_COLOUR_3);

    if (g->state == GAME_OVER)
    {
        draw_game_over();
    }

    LCD_Refresh(&cfg0);
}

MenuState Game1_Run(void)
{
    int dir;
    GPIO_PinState lastB1State = GPIO_PIN_SET;
    Game1_Reset(&game1);
    Game1_Draw(&game1);

     while (1)
    {
        Input_Read();

        dir = get_joystick();

        GPIO_PinState currentB1State = HAL_GPIO_ReadPin(B1_GPIO_Port, B1_Pin);

    if (current_input.btn2_pressed)
    {
        HAL_GPIO_WritePin(LED_PORT, LED_PIN, GPIO_PIN_RESET);
        return MENU_STATE_HOME;
    }

    if (game1.state == GAME_RUNNING)
    {
        Game1_Update(&game1, dir);
        Game1_Draw(&game1);

        if (game1.sound == SOUND_CRASH)
        {
            buzzer_on(600, 250);
            game1.sound = SOUND_NONE;
        }

        if (game1.sound == SOUND_ITEM)
        {
            buzzer_on(1800, 80);
            game1.sound = SOUND_NONE;
        }
    }

        if (game1.state == GAME_OVER)
        {
            HAL_GPIO_TogglePin(LED_PORT, LED_PIN);
            HAL_Delay(120);

            GPIO_PinState currentB1State = HAL_GPIO_ReadPin(B1_GPIO_Port, B1_Pin);

            if (lastB1State == GPIO_PIN_SET && currentB1State == GPIO_PIN_RESET)
            {
                    Game1_Reset(&game1);
                    Game1_Draw(&game1);
            }
        }

        lastB1State = currentB1State;
    }
}

