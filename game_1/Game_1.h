#ifndef GAME1_H
#define GAME1_H

#include "main.h"
#include "menu.h"

#define LANE_COUNT 3
#define ENEMY_COUNT 2
#define LCD_W 240
#define LCD_H 240
#define ROAD_TOP 20
#define PLAYER_Y 205
#define CAR_W 18
#define CAR_H 24
#define ITEM_BONUS_SCORE 3
#define SCORE_LEVEL_STEP 5

typedef enum
{
    GAME_START = 0,
    GAME_RUNNING,
    GAME_OVER
} GameState;

typedef enum
{
    SOUND_NONE = 0,
    SOUND_CRASH,
    SOUND_ITEM
} SoundEvent;

typedef struct
{
    int lane;
    int y;
} Car;

typedef struct
{
    int lane;
    int y;
    int active;
} Enemy;

typedef struct
{
    int lane;
    int y;
    int active;
} Item;

typedef struct
{
    GameState state;
    SoundEvent sound;
    Car player;
    Enemy enemies[ENEMY_COUNT];
    Item item;
    uint32_t score;
    uint32_t level;
    uint32_t last_tick;
    uint32_t update_interval_ms;
} Game1_t;

MenuState Game1_Run(void);
void Game1_Init(Game1_t *g);
void Game1_Reset(Game1_t *g);
void Game1_Update(Game1_t *g, int dir);
void Game1_Draw(Game1_t *g);

#endif