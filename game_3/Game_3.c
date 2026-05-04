#include "LCD1.h"
#include "ST7789V2_Driver.h"
#include "joystick.h"
#include "Menu.h"
#include "stm32l4xx_hal_gpio.h"

#include <math.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <sys/_intsup.h>

extern Joystick_cfg_t joystick_cfg;
extern Joystick_t joystick_data;
static GPIO_PinState lastB1State = GPIO_PIN_SET;

//menu
typedef enum{
    MENU_START = 0,
    MENU_INSTRUCTIONS,
    MENU_EXIT,
    MENU_COUNT
}MenuOption;

typedef enum{
    SCREEN_MENU = 0,
    SCREEN_GAME,
    SCREEN_INSTRUCTIONS,
    SCREEN_POINT_A,
    SCREEN_POINT_B,
    SCREEN_POINT_C,
    SCREEN_POINT_D,
    SCREEN_POINT_E,
    SCREEN_BATTLE_A,
    SCREEN_BATTLE_B,
    SCREEN_BOSS,
    SCREEN_END,
    SCREEN_WIN,
    SCREEN_EXIT
}ScreenState;

//draw menu
MenuOption currentOption = MENU_START;
ScreenState currentScreen = SCREEN_MENU;

extern ST7789V2_cfg_t cfg0;

void drawMainMenu(MenuOption selected)
{
    LCD_Set_Palette(PALETTE_DEFAULT);
    LCD_clear();

    // title
    LCD_printString("SEE", 30, 33, LCD_COLOUR_6,4);
    LCD_printString("YOU", 130, 33, LCD_COLOUR_11,4); 
    LCD_printString("TOMORROW", 25, 70, LCD_COLOUR_13,4);

    // menu text
    LCD_printString("START", 30, 130, LCD_COLOUR_3, 2);
    LCD_printString("INSTRUCTIONS", 30, 160, LCD_COLOUR_4, 2);
    LCD_printString("EXIT", 30, 190, LCD_COLOUR_5, 2);

    // selected mark
    if (selected == MENU_START)
    {
        LCD_printChar('o', 15, 130, LCD_COLOUR_7);
    }
    else if (selected == MENU_INSTRUCTIONS)
    {
        LCD_printChar('o', 15, 160, LCD_COLOUR_7);
    }
    else if (selected == MENU_EXIT)
    {
        LCD_printChar('o', 15, 190, LCD_COLOUR_7);
    }

    LCD_Refresh(&cfg0);
}

//struction
void drawInstructionsScreen(void)
{
    LCD_Set_Palette(PALETTE_DEFAULT);
    LCD_clear();

    LCD_printString("INSTRUCTIONS", 5, 30, LCD_COLOUR_11, 3);

    LCD_printString("Move: Joystick", 15, 90, LCD_COLOUR_3, 2);
    LCD_printString("Choose: Button", 15, 110, LCD_COLOUR_13, 2);

    LCD_printString("Goal:", 15, 130, LCD_COLOUR_4, 2);
    LCD_printString("Complete the", 15, 150, LCD_COLOUR_3, 2);
    LCD_printString("Story route", 15, 170, LCD_COLOUR_3, 2);

    LCD_printString("Press button", 15, 190, LCD_COLOUR_4, 2);
    LCD_printString("Return", 15, 210, LCD_COLOUR_5, 2);

    LCD_Refresh(&cfg0);
}

//map screen
bool pointDone[5] = {false, false, false, false, false};
bool pointE_unlocked = false;
int currentPoint = 0;
void checkUnlock(void);
bool isPointAvailable(int point);
void selectPointByDirection(Direction dir);
void updatePlayerPositionByPoint(void);
void drawMapScreen(void);

void drawEllipse(int x0, int y0, int a, int b, uint8_t colour)
{
    for (int deg = 0; deg < 360; deg += 3)
    {
        float rad = deg * 3.14159f / 180.0f;
        int x = x0 + (int)(a * cos(rad));
        int y = y0 + (int)(b * sin(rad));
        LCD_Set_Pixel(x, y, colour);
    }
}

void fillEllipse(int x0, int y0, int a, int b, uint8_t colour)
{
    for (int y = -b; y <= b; y++)
    {
        for (int x = -a; x <= a; x++)
        {
            if ((x * x * b * b) + (y * y * a * a) <= (a * a * b * b))
            {
                LCD_Set_Pixel(x0 + x, y0 + y, colour);
            }
        }
    }
}

int playerX = 30;
int playerY = 30;

int petX = 36;
int petY = 36;

void drawMapScreen(void)
{
    LCD_clear();

    // A: left top
    if (!pointDone[0])
    {
        fillEllipse(30, 30, 20, 8, LCD_COLOUR_2);
    }

    // B: right top
    if (!pointDone[1])
    {
        fillEllipse(200, 30, 20, 8, LCD_COLOUR_4);
    }

    // C: left bottom
    if (!pointDone[2])
    {
        fillEllipse(30, 200, 20, 8, LCD_COLOUR_6);
    }

    // D: right bottom
    if (!pointDone[3])
    {
        fillEllipse(200, 200, 20, 8, LCD_COLOUR_11);
    }

    // E: center (hidden until A-D done)
    if (pointE_unlocked && !pointDone[4])
    {
        fillEllipse(120, 120, 30, 12, LCD_COLOUR_15);
        fillEllipse(120, 120, 20, 8, LCD_COLOUR_3);
    }

    LCD_Draw_Circle(playerX, playerY, 3, LCD_COLOUR_5,1);   // User
    LCD_Draw_Circle(petX, petY, 2, LCD_COLOUR_3,1);        // Pet

    LCD_Refresh(&cfg0);
}

void finishPoint(int index)
{
    printf("finishPoint called, index = %d\r\n", index);
    pointDone[index] = true;
    checkUnlock();
    updatePlayerPositionByPoint();
    currentScreen = SCREEN_GAME;
    drawMapScreen();
}

void checkUnlock(void)
{
    if (pointDone[0] && pointDone[1] && pointDone[2] && pointDone[3] && !pointE_unlocked)
    {
        pointE_unlocked = true;

        currentPoint = 4; 
        updatePlayerPositionByPoint(); 
    }
}

bool isPointAvailable(int point)
{
    if (point >= 0 && point <= 3)
    {
        return !pointDone[point];
    }

    if (point == 4)
    {
        return pointE_unlocked && !pointDone[4];
    }

    return false;
}

void updatePlayerPositionByPoint(void)
{
    switch (currentPoint)
    {
        case 0:     // A
            playerX = 30;
            playerY = 30;
            break;

        case 1:     // B
            playerX = 200;
            playerY = 30;
            break;

        case 2:     // C
            playerX = 30;
            playerY = 200;
            break;

        case 3:     // D
            playerX = 200;
            playerY = 200;
            break;

        case 4:     // E
            playerX = 120;
            playerY = 120;
            petX = playerX + 6;
            petY = playerY + 6;
            break;

        default:
            break;
    }

    petX = playerX + 6;
    petY = playerY + 6;
}

void selectPointByDirection(Direction dir)
{
    switch (dir)
    {
        case N:
        case NW:
            if (isPointAvailable(0)) currentPoint = 0;   // A
            break;

        case NE:
        case E:
            if (isPointAvailable(1)) currentPoint = 1;   // B
            break;

        case SW:
        case W:
            if (isPointAvailable(2)) currentPoint = 2;   // C
            break;

        case S:    
        case SE:
            if (isPointAvailable(3)) currentPoint = 3;   // D
            break;

        default:
            break;
    }

    if (pointE_unlocked && isPointAvailable(4) && dir == CENTRE)
    {
        currentPoint = 4;
    }
    updatePlayerPositionByPoint();
}

//battle
bool battleFinished = false;
bool battleWon = false;

int soulX = 120;
int soulY = 180;

int currentBattleIndex = 0;

typedef struct
{
    int active;
    int x;
    int y;
    int width;
    int height;
    int speed;
} Laser;

Laser laser1;
Laser laser1_1;
Laser laser2;
Laser laser2_1;
Laser laser3;
Laser laser3_1;
Laser laser4;
Laser laser4_1;

int battlePlayerX = 60;
int battlePlayerY = 100;
int battlePlayerSize = 6;

//battle 1
int battle1HP = 3;
int battleTimer = 0;
int battle1Win = 0;
int battle1Lose = 0;

// Battle 2
int battle2HP = 5;
int laser2X = 10;
int laser2Dir = 1;
int battle2Win = 0;
int battle2Lose = 0;
int laser2_1Dir = 1;

// Boss Battle
int bossHP = 10;
int bossWin = 0;
int bossLose = 0;
int bossTimer = 0;
int bossLaserDir = 1;
int bossLaser3Dir = 1;
int bossLaser4Dir = 1;
int bossLaser3_1Dir = 1;
int bossLaser4_1Dir = 1;

void initBattleA(void)
{
    battlePlayerX = 60;
    battlePlayerY = 100;
    battlePlayerSize = 6;

    battle1HP = 3;
    battleTimer = 0;
    battle1Win = 0;
    battle1Lose = 0;

    laser1.active = 1;
    laser1.x = 20;// laser initial x position
    laser1.y = 20;     // laser initial y position
    laser1.width = 100;
    laser1.height = 6;
    laser1.speed = 0;

    laser1_1.active = 0;
    laser1_1.x = 120;   // laser1_1 initial x position
    laser1_1.y = 20;   // laser1_1 initial y position
    laser1_1.width = 100;
    laser1_1.height = 6;
    laser1_1.speed = 0; 
}

void initBattleB(void)
{
    battlePlayerX = 120;
    battlePlayerY = 120;
    battlePlayerSize = 8;

    battle2HP = 5;
    battle2Win = 0;
    battle2Lose = 0;
    battleTimer = 0;

    laser2.active = 1;
    laser2.x = 0;
    laser2.y = 50;
    laser2.width = 6;
    laser2.height = 100;
    laser2.speed = 3;
    laser2Dir = 1;
    laser2_1Dir = 1;

    laser2_1.active = 0;
    laser2_1.x = 0;
    laser2_1.y = 120;
    laser2_1.width = 6;
    laser2_1.height = 100;
    laser2_1.speed = 3;
}

void initBossBattle(void)
{
    battlePlayerX = 120;
    battlePlayerY = 120;
    battlePlayerSize = 8;

    bossHP = 10;
    bossWin = 0;
    bossLose = 0;
    bossTimer = 0;

    bossLaser3Dir = 1;
    bossLaser4Dir = 1;
    bossLaser3_1Dir = 1;
    bossLaser4_1Dir = 1;

    //hoziontal laser
    laser3.active = 1;
    laser3.x = 20;
    laser3.y = 70;
    laser3.width = 90;
    laser3.height = 6;
    laser3.speed = 3;

    laser3_1.active = 0;
    laser3_1.x = 130;
    laser3_1.y = 70;
    laser3_1.width = 90;
    laser3_1.height = 6;
    laser3_1.speed = 3;

    //vertical laser
    laser4.active = 1;
    laser4.x = 20;
    laser4.y = 70;
    laser4.width = 6;
    laser4.height = 90;
    laser4.speed = 3;

    laser4_1.active = 0;
    laser4_1.x = 20;
    laser4_1.y = 130;
    laser4_1.width = 6;
    laser4_1.height = 90;
    laser4_1.speed = 3;
}

void drawBattle1Screen(void)
{
    LCD_clear();

    //border
    LCD_Draw_Rect(20, 50, 200, 170, LCD_COLOUR_3, 0);

    // HP display
    char hpText[20];
    sprintf(hpText, "HP:%d", battle1HP);
    LCD_printString(hpText, 10, 5, LCD_COLOUR_3, 2);

    //  main character
    LCD_Draw_Rect(battlePlayerX, battlePlayerY, battlePlayerSize, battlePlayerSize, LCD_COLOUR_4, 1);

    //  laser
    if (laser1.active)
    {
        LCD_Draw_Rect(laser1.x, laser1.y, laser1.width, laser1.height, LCD_COLOUR_2, 1);
    }

    if (laser1_1.active)
    {
        LCD_Draw_Rect(laser1_1.x, laser1_1.y, laser1_1.width, laser1_1.height, LCD_COLOUR_2, 1);
    }

    LCD_Refresh(&cfg0);
}

void drawBattle2Screen(void)
{
    LCD_clear();

    LCD_Draw_Rect(20, 50, 200, 170, LCD_COLOUR_3, 0);

    char hpText[20];
    sprintf(hpText, "HP:%d", battle2HP);
    LCD_printString(hpText, 10, 5, LCD_COLOUR_3, 2);

    LCD_Draw_Rect(battlePlayerX, battlePlayerY, battlePlayerSize, battlePlayerSize, LCD_COLOUR_4, 1);

    if (laser2.active)
    {
        LCD_Draw_Rect(laser2.x, laser2.y, laser2.width, laser2.height, LCD_COLOUR_2, 1);
    }

    if (laser2_1.active)
    {
        LCD_Draw_Rect(laser2_1.x, laser2_1.y, laser2_1.width, laser2_1.height, LCD_COLOUR_2, 1);
    }

    LCD_Refresh(&cfg0);
}

void drawBossBattleScreen(void)
{
    LCD_clear();

    LCD_Draw_Rect(20, 50, 200, 170, LCD_COLOUR_3, 0);

    char hpText[20];
    sprintf(hpText, "HP:%d", bossHP);
    LCD_printString(hpText, 10, 5, LCD_COLOUR_3, 2);

    LCD_printString("BOSS", 75, 5, LCD_COLOUR_1, 2);

    LCD_Draw_Rect(battlePlayerX, battlePlayerY, battlePlayerSize, battlePlayerSize, LCD_COLOUR_4, 1);

    if (laser3.active)
    {
        LCD_Draw_Rect(laser3.x, laser3.y, laser3.width, laser3.height, LCD_COLOUR_2, 1);
    }

    if (laser3_1.active)
    {
        LCD_Draw_Rect(laser3_1.x, laser3_1.y, laser3_1.width, laser3_1.height, LCD_COLOUR_2, 1);
    }

    if (laser4.active)
    {
        LCD_Draw_Rect(laser4.x, laser4.y, laser4.width, laser4.height, LCD_COLOUR_2, 1);
    }

    if (laser4_1.active)
    {
        LCD_Draw_Rect(laser4_1.x, laser4_1.y, laser4_1.width, laser4_1.height, LCD_COLOUR_2, 1);
    }

    LCD_Refresh(&cfg0);
}

void drawWinScreen(void)
{
    LCD_clear();
    LCD_printString("You win!", 30, 60, LCD_COLOUR_3, 2);

    LCD_Refresh(&cfg0);
}

UserInput input;

void runBattle1(void)
{

    Joystick_Read(&joystick_cfg, &joystick_data);
    input = Joystick_GetInput(&joystick_data);

    int playerSpeed = 4;//speed of player movement

    if (input.direction == W || input.direction == NW || input.direction == SW)
        battlePlayerX -= playerSpeed;

    if (input.direction == E || input.direction == NE || input.direction == SE)
        battlePlayerX += playerSpeed;

    if (input.direction == N || input.direction == NE || input.direction == NW)
        battlePlayerY -= playerSpeed;

    if (input.direction == S || input.direction == SE || input.direction == SW)
        battlePlayerY += playerSpeed;

    // limit player within bounds
    if (battlePlayerX < 20)
        battlePlayerX = 20;
    if (battlePlayerX > 20 + 200 - battlePlayerSize)
        battlePlayerX = 20 + 200 - battlePlayerSize;

    if (battlePlayerY < 50)
        battlePlayerY = 50;
    if (battlePlayerY > 50 + 170 - battlePlayerSize)
        battlePlayerY = 50 + 170 - battlePlayerSize;

    // move laser
    if (laser1.active)
    {
        laser1.y += 3;
        if (laser1.y > 210)
            laser1.y = 30;
    }

    if (battleTimer>150)
    {
        laser1_1.active = 1;
        laser1_1.y += 3;
        if(laser1_1.y > 210)
            laser1_1.y = 30;
    }

    // collision detection
    if (laser1.active &&
        battlePlayerX < laser1.x + laser1.width &&
        battlePlayerX + battlePlayerSize > laser1.x &&
        battlePlayerY < laser1.y + laser1.height &&
        battlePlayerY + battlePlayerSize > laser1.y)
    {
        battle1HP--;
        laser1.y = 30;

        if (battle1HP <= 0)
        {
            battle1Lose = 1;
            return;
        }
    }

    if (laser1_1.active &&
        battlePlayerX < laser1_1.x + laser1_1.width &&
        battlePlayerX + battlePlayerSize > laser1_1.x &&
        battlePlayerY < laser1_1.y + laser1_1.height &&
        battlePlayerY + battlePlayerSize > laser1_1.y)
    {
        battle1HP--;
        laser1_1.y = 30;

        if (battle1HP <= 0)
        {
            battle1Lose = 1;
            return;
        }
    }

    // time-based victory condition
    battleTimer++;
    if (battleTimer > 300)
    {
        battle1Win = 1;
        return;
    }

    drawBattle1Screen();
    HAL_Delay(30);
}

void runBattle2(void)
{

    Joystick_Read(&joystick_cfg, &joystick_data);
    input = Joystick_GetInput(&joystick_data);

    int playerSpeed = 4;//speed of player movement

    if (input.direction == W || input.direction == NW || input.direction == SW)
        battlePlayerX -= playerSpeed;

    if (input.direction == E || input.direction == NE || input.direction == SE)
        battlePlayerX += playerSpeed;

    if (input.direction == N || input.direction == NE || input.direction == NW)
        battlePlayerY -= playerSpeed;

    if (input.direction == S || input.direction == SE || input.direction == SW)
        battlePlayerY += playerSpeed;


    if (battlePlayerX < 20)
        battlePlayerX = 20;
    if (battlePlayerX > 20 + 200 - battlePlayerSize)
        battlePlayerX = 20 + 200 - battlePlayerSize;

    if (battlePlayerY < 50)
        battlePlayerY = 50;
    if (battlePlayerY > 50 + 170 - battlePlayerSize)
        battlePlayerY = 50 + 170 - battlePlayerSize;


    laser2.x += laser2.speed * laser2Dir;
    if(laser2.x <20)
    {
        laser2.x = 20;
        laser2Dir = 1;
    }

    if(laser2.x > 20 + 200 - laser2.width)
    {
        laser2.x = 20 + 200 - laser2.width;
        laser2Dir = -1;
    }

    if (battleTimer > 200)
    {
        laser2_1.active = 1;
        laser2_1.x += laser2_1.speed * laser2_1Dir;
        if(laser2_1.x <20)
        {
            laser2_1.x = 20;
            laser2_1Dir = 1;
        }

        if(laser2_1.x > 20 + 200 - laser2_1.width)
        {
            laser2_1.x = 20 + 200 - laser2_1.width;
            laser2_1Dir = -1;
        }
    }


    if (laser2.active &&
        battlePlayerX < laser2.x + laser2.width &&
        battlePlayerX + battlePlayerSize > laser2.x &&
        battlePlayerY < laser2.y + laser2.height &&
        battlePlayerY + battlePlayerSize > laser2.y)
    {
        battle2HP--;
        battlePlayerX = 60;
        battlePlayerY = 100;

        if (battle2HP <= 0)
            battle2Lose = 1;
    }

    if (laser2_1.active &&
        battlePlayerX < laser2_1.x + laser2_1.width &&
        battlePlayerX + battlePlayerSize > laser2_1.x &&
        battlePlayerY < laser2_1.y + laser2_1.height &&
        battlePlayerY + battlePlayerSize > laser2_1.y)
    {
        battle2HP--;
        battlePlayerX = 60;
        battlePlayerY = 100;

        if (battle2HP <= 0)
            battle2Lose = 1;
    }


    battleTimer++;
    if (battleTimer > 400)
        battle2Win = 1;

    drawBattle2Screen();
    HAL_Delay(30);
}

void runBossBattle(void)
{

    Joystick_Read(&joystick_cfg, &joystick_data);
    input = Joystick_GetInput(&joystick_data);

    int playerSpeed = 4;//speed of player movement

    if (input.direction == W || input.direction == NW || input.direction == SW)
        battlePlayerX -= playerSpeed;

    if (input.direction == E || input.direction == NE || input.direction == SE)
        battlePlayerX += playerSpeed;

    if (input.direction == N || input.direction == NE || input.direction == NW)
        battlePlayerY -= playerSpeed;

    if (input.direction == S || input.direction == SE || input.direction == SW)
        battlePlayerY += playerSpeed;


    if (battlePlayerX < 20)
        battlePlayerX = 20;
    if (battlePlayerX > 20 + 200 - battlePlayerSize)
        battlePlayerX = 20 + 200 - battlePlayerSize;

    if (battlePlayerY < 50)
        battlePlayerY = 50;
    if (battlePlayerY > 50 + 170 - battlePlayerSize)
        battlePlayerY = 50 + 170 - battlePlayerSize;


    laser3.y += laser3.speed * bossLaser3Dir;

    if (laser3.y < 20)
    {
        laser3.y = 20;
        bossLaser3Dir = 1;
    }
    if (laser3.y > 170 - laser3.height)
    {
        laser3.y = 170 - laser3.height;
        bossLaser3Dir = -1;
    }

    if (bossTimer > 250)
    {
        laser3_1.active = 1;
        laser3_1.y += laser3_1.speed * bossLaser3_1Dir;

        if (laser3_1.y < 50)
        {
            laser3_1.y = 50;
            bossLaser3_1Dir = 1;
        }
        if (laser3_1.y > 170 - laser3_1.height)
        {
            laser3_1.y = 170 - laser3_1.height;
            bossLaser3_1Dir = -1;
        }
    }


    laser4.x += laser4.speed * bossLaser4Dir;

    if (laser4.x < 20)
    {
        laser4.x = 20;
        bossLaser4Dir = 1;
    }
    if (laser4.x > 220 - laser4.width)
    {
        laser4.x = 220 - laser4.width;
        bossLaser4Dir = -1;
    }

    if (bossTimer > 250)
    {
        laser4_1.active = 1;
        laser4_1.x += laser4_1.speed * bossLaser4_1Dir;

        if (laser4_1.x < 20)
        {
            laser4_1.x = 20;
            bossLaser4_1Dir = 1;
        }
        if (laser4_1.x > 220 - laser4_1.width)
        {
            laser4_1.x = 220 - laser4_1.width;
            bossLaser4_1Dir = -1;
        }
    }


    if (laser3.active &&
        battlePlayerX < laser3.x + laser3.width &&
        battlePlayerX + battlePlayerSize > laser3.x &&
        battlePlayerY < laser3.y + laser3.height &&
        battlePlayerY + battlePlayerSize > laser3.y)
    {
        bossHP--;
        battlePlayerX = 60;
        battlePlayerY = 100;
    }

    if (laser3_1.active &&
        battlePlayerX < laser3_1.x + laser3_1.width &&
        battlePlayerX + battlePlayerSize > laser3_1.x &&
        battlePlayerY < laser3_1.y + laser3_1.height &&
        battlePlayerY + battlePlayerSize > laser3_1.y)
    {
        bossHP--;
        battlePlayerX = 60;
        battlePlayerY = 100;
    }


    if (laser4.active &&
        battlePlayerX < laser4.x + laser4.width &&
        battlePlayerX + battlePlayerSize > laser4.x &&
        battlePlayerY < laser4.y + laser4.height &&
        battlePlayerY + battlePlayerSize > laser4.y)
    {
        bossHP--;
        battlePlayerX = 60;
        battlePlayerY = 100;
    }

    if (laser4_1.active &&
        battlePlayerX < laser4_1.x + laser4_1.width &&
        battlePlayerX + battlePlayerSize > laser4_1.x &&
        battlePlayerY < laser4_1.y + laser4_1.height &&
        battlePlayerY + battlePlayerSize > laser4_1.y)
    {
        bossHP--;
        battlePlayerX = 60;
        battlePlayerY = 100;
    }

    if  (bossHP <= 0)
        bossLose = 1;
        bossTimer++;
    if (bossTimer > 500)
        bossWin = 1;

    drawBossBattleScreen();
    HAL_Delay(30);
}

//NPC
void drawDialogueScreen(const char* name,
                        const char* line1,
                        const char* line2,
                        const char* line3,
                        uint8_t nameColour)
{
    LCD_clear();

    // name
    LCD_printString(name, 20, 20, nameColour, 2);

    // dialogue box
    LCD_Draw_Rect(10, 150, 220, 70, LCD_COLOUR_1, 0);

    // dialogue text
    LCD_printString(line1, 20, 165, LCD_COLOUR_1, 2);
    LCD_printString(line2, 20, 180, LCD_COLOUR_1, 2);
    LCD_printString(line3, 20, 195, LCD_COLOUR_1, 2);

    // prompt
    LCD_printString("Press B1", 70, 225, LCD_COLOUR_3, 2);

    LCD_Refresh(&cfg0);
}

//A_NPC
void drawA_Dialogue1(void)
{
    drawDialogueScreen("NPC A", "Thank you for", "your hard work", "all these while.", LCD_COLOUR_2);
}

void drawA_Dialogue2(void)
{
    drawDialogueScreen("NPC B", "From now on,", "no more goodbyes.", "", LCD_COLOUR_2);
}

void drawA_Dialogue3(void)
{
    drawDialogueScreen("NPC C", "Trailblazer,", "become king.", "", LCD_COLOUR_2);
}

//B_NPC
void drawB_Dialogue1(void)
{
    drawDialogueScreen("NPC D", "The place you", "reside is the gentle", "sea of flowers.", LCD_COLOUR_4);
}

void drawB_Dialogue2(void)
{
    drawDialogueScreen("NPC E", "The seeds you've", "sown are", "already sprouting.", LCD_COLOUR_4);
}

void drawB_Dialogue3(void)
{
    drawDialogueScreen("NPC F", "It's you", "who healed", "the sky.", LCD_COLOUR_4);
}

//C_NPC
void drawC_Dialogue1(void)
{
    drawDialogueScreen("NPC G", "Homeward you come,", "distant wind.", "", LCD_COLOUR_6);
}

void drawC_Dialogue2(void)
{
    drawDialogueScreen("NPC H", "Did you see? This", "world has ushered", "in the dawn.", LCD_COLOUR_6);
}

void drawC_Dialogue3(void)
{
    drawDialogueScreen("NPC I", "May the next", "feast be", "eternal.", LCD_COLOUR_6);
}

//D_NPC
void drawD_Dialogue1(void)
{
    drawDialogueScreen("NPC J", "The stars shall", "sing your", "great journey.", LCD_COLOUR_5);
}

void drawD_Dialogue2(void)
{
    drawDialogueScreen("NPC K", "We were never", "apart.", "", LCD_COLOUR_5);
}

void drawD_Dialogue3(void)
{
    drawDialogueScreen("NPC L", "Let's walk", "towards", "tomorrow together.", LCD_COLOUR_5);
}

//E_NPC
void drawE_Dialogue1(void)
{
    drawDialogueScreen("Boss", "I've been corrupted.", "Please defeat me.", "Don't bow to Him.", LCD_COLOUR_15);
}

void drawE_Dialogue2(void)
{
    drawDialogueScreen("Boss", "Just as the ", "first time", "we met.", LCD_COLOUR_15);
}

//END
void drawClearScreen(void)
{
    LCD_clear();
    LCD_printString("YOU WIN!", 60, 80, LCD_COLOUR_2, 2);
    LCD_printString("Press B1 to exit", 35, 120, LCD_COLOUR_3, 2);
    LCD_Refresh(&cfg0);
}

// Main game loop
int aDialogueStep = 0;
int bDialogueStep = 0;
int cDialogueStep = 0;
int dDialogueStep = 0;
int eDialogueStep = 0;

GPIO_PinState lastB1State_A = GPIO_PIN_SET;
GPIO_PinState lastB1State_B = GPIO_PIN_SET;
GPIO_PinState lastB1State_C = GPIO_PIN_SET;
GPIO_PinState lastB1State_D = GPIO_PIN_SET;
GPIO_PinState lastB1State_E = GPIO_PIN_SET;
GPIO_PinState lastB1State_WIN = GPIO_PIN_SET;

int pointX[5] = {30, 200, 30, 200, 120};
int pointY[5] = {30, 30, 200, 200, 120};

MenuState Game3_Run(void)
{
    currentOption = MENU_START;
    currentScreen = SCREEN_MENU;
    currentPoint = 0;
    playerX = pointX[currentPoint];
    playerY = pointY[currentPoint];

    petX = playerX + 6;
    petY = playerY + 6;

    drawMainMenu(currentOption);

    while (1)
    {
        if (currentScreen == SCREEN_MENU)
        {
            Joystick_Read(&joystick_cfg, &joystick_data);
            UserInput input = Joystick_GetInput(&joystick_data);
            // up
            if (input.direction == N)
            {
                if (currentOption > MENU_START)
                {
                    currentOption--;
                    drawMainMenu(currentOption);
                    HAL_Delay(200);
                }
            }
            // down
            if (input.direction == S)
            {
                if (currentOption < MENU_EXIT)
                {
                    currentOption++;
                    drawMainMenu(currentOption);
                    HAL_Delay(200);
                }
            }

            GPIO_PinState currentB1State = HAL_GPIO_ReadPin(B1_GPIO_Port, B1_Pin);

            if (lastB1State == GPIO_PIN_SET && currentB1State == GPIO_PIN_RESET)
            {
                HAL_Delay(20);

                if (currentOption == MENU_START)
                {
                    currentScreen = SCREEN_GAME;
                    drawMapScreen();
                }       
                else if (currentOption == MENU_INSTRUCTIONS)
                {
                    currentScreen = SCREEN_INSTRUCTIONS;
                    drawInstructionsScreen();
                }
                else if (currentOption == MENU_EXIT)
                {
                    return MENU_STATE_HOME;
                }
            }

            lastB1State = currentB1State;
        }

        else if (currentScreen == SCREEN_INSTRUCTIONS)
        {
            GPIO_PinState currentB1State = HAL_GPIO_ReadPin(B1_GPIO_Port, B1_Pin);

            if (lastB1State == GPIO_PIN_SET && currentB1State == GPIO_PIN_RESET)
            {
                HAL_Delay(20);
                currentScreen = SCREEN_MENU;
                drawMainMenu(currentOption);
            }

            lastB1State = currentB1State;
        }

        else if (currentScreen == SCREEN_GAME)
        {
            Joystick_Read(&joystick_cfg, &joystick_data);
            UserInput input = Joystick_GetInput(&joystick_data);

            GPIO_PinState currentB1State = HAL_GPIO_ReadPin(B1_GPIO_Port, B1_Pin);

            if (input.direction != CENTRE && input.magnitude > 0.3f)
            {
                HAL_Delay(150);
                selectPointByDirection(input.direction);

                playerX = pointX[currentPoint];
                playerY = pointY[currentPoint];

                petX = playerX + 6;
                petY = playerY + 6;

                drawMapScreen();

                do
                {
                    Joystick_Read(&joystick_cfg, &joystick_data);
                    input = Joystick_GetInput(&joystick_data);
                    HAL_Delay(20);
                }
                while (input.direction != CENTRE);
            }
            


            if (lastB1State == GPIO_PIN_SET && currentB1State == GPIO_PIN_RESET)
            {
                HAL_Delay(20);

                if (currentPoint == 0 && !pointDone[0])
                {
                    currentScreen = SCREEN_POINT_A;
                    aDialogueStep = 0;
                    drawA_Dialogue1();
                }
                else if (currentPoint == 1 && !pointDone[1])
                {
                    currentScreen = SCREEN_POINT_B;
                    bDialogueStep = 0;
                    drawB_Dialogue1();
                }
                else if (currentPoint == 2 && !pointDone[2])
                {
                    currentScreen = SCREEN_POINT_C;
                    cDialogueStep = 0;
                    drawC_Dialogue1();
                }
                else if (currentPoint == 3 && !pointDone[3])
                {
                    currentScreen = SCREEN_POINT_D;
                    dDialogueStep = 0;
                    drawD_Dialogue1();
                }
                else if (currentPoint == 4 && pointE_unlocked && !pointDone[4])
                {
                    currentScreen = SCREEN_POINT_E;
                    eDialogueStep = 0;
                    drawE_Dialogue1();
                }
            }
            lastB1State = currentB1State;
        }

        else if (currentScreen == SCREEN_WIN)
        {
            GPIO_PinState currentB1State_WIN = HAL_GPIO_ReadPin(B1_GPIO_Port, B1_Pin);

            if (lastB1State_WIN == GPIO_PIN_SET && currentB1State_WIN == GPIO_PIN_RESET)
            {
                HAL_Delay(20);

                if (currentBattleIndex == 1)
                {
                    printf("WIN button pressed, going back map\r\n");
                    initBattleA();
                    finishPoint(1);// finish B point, back to map
                }
                else if (currentBattleIndex == 2)
                {
                    initBattleB();
                    finishPoint(3);   // finish D point, back to map
                }
                else if (currentBattleIndex == 3)
                {
                    initBossBattle();
                    pointDone[4] = true;
                    currentScreen = SCREEN_END;
                    drawClearScreen();
                }
                lastB1State_WIN = currentB1State_WIN;
                continue;
            }
            lastB1State_WIN = currentB1State_WIN;
        }   

        else if (currentScreen == SCREEN_BATTLE_A)
        {
            runBattle1();

            if (battle1Win)
            {
                battle1Win = 0;
                currentScreen = SCREEN_WIN;
                lastB1State_WIN = HAL_GPIO_ReadPin(B1_GPIO_Port, B1_Pin);
                drawWinScreen();
                continue;
            }
            else if (battle1Lose)
            {
                battle1Lose = 0;
                initBattleA();
                drawBattle1Screen();
                continue;
            }
        }

        else if (currentScreen == SCREEN_BATTLE_B)
        {
            runBattle2();

            if (battle2Win)
            {
                battle2Win = 0;
                currentScreen = SCREEN_WIN;
                lastB1State_WIN = HAL_GPIO_ReadPin(B1_GPIO_Port, B1_Pin);
                drawWinScreen();
                continue;
            }
            else if (battle2Lose)
            {
                battle2Lose = 0;
                initBattleB();
                drawBattle2Screen();
                continue;
            }
        }

        else if (currentScreen == SCREEN_BOSS)
        {
            runBossBattle();

            if (bossWin)
            {
                bossWin = 0;
                currentScreen = SCREEN_WIN;
                lastB1State_WIN = HAL_GPIO_ReadPin(B1_GPIO_Port, B1_Pin);
                drawWinScreen();
                continue;
            }           
            else if (bossLose)
            {
                bossLose = 0;
                initBossBattle();
                drawBossBattleScreen();
                continue;
            }
        }

        else if (currentScreen == SCREEN_POINT_A)
        {
            GPIO_PinState currentB1State_A = HAL_GPIO_ReadPin(B1_GPIO_Port, B1_Pin);

            if (lastB1State_A == GPIO_PIN_SET && currentB1State_A == GPIO_PIN_RESET)
            {
                HAL_Delay(20);

                if (aDialogueStep == 0)
                {
                    aDialogueStep = 1;
                    drawA_Dialogue2();
                }
                else if (aDialogueStep == 1)
                {
                    aDialogueStep = 2;
                    drawA_Dialogue3();
                }
                else if (aDialogueStep == 2)
                {
                    finishPoint(0);        // A点完成，回地图
                }
            }

            lastB1State_A = currentB1State_A;
        }

        else if (currentScreen == SCREEN_POINT_B)
        {
            GPIO_PinState currentB1State_B = HAL_GPIO_ReadPin(B1_GPIO_Port, B1_Pin);

            if (lastB1State_B == GPIO_PIN_SET && currentB1State_B == GPIO_PIN_RESET)
            {
                HAL_Delay(20);

                if (bDialogueStep == 0)
                {
                    bDialogueStep = 1;
                    drawB_Dialogue2();
                }
                else if (bDialogueStep == 1)
                {
                    bDialogueStep = 2;
                    drawB_Dialogue3();
                }

                else if (bDialogueStep == 2)
                {
                    currentBattleIndex = 1;
                    currentScreen = SCREEN_BATTLE_A;
                    initBattleA();
                    drawBattle1Screen();

                    lastB1State_B = currentB1State_B;
                    continue;
                }
            }

            lastB1State_B = currentB1State_B;
        }

        else if (currentScreen == SCREEN_POINT_C)
        {
            GPIO_PinState currentB1State_C = HAL_GPIO_ReadPin(B1_GPIO_Port, B1_Pin);

            if (lastB1State_C == GPIO_PIN_SET && currentB1State_C == GPIO_PIN_RESET)
            {
                HAL_Delay(20);

                if (cDialogueStep == 0)
                {
                    cDialogueStep = 1;
                    drawC_Dialogue2();
                }
                else if (cDialogueStep == 1)
                {
                    cDialogueStep = 2;
                    drawC_Dialogue3();
                }
                else if (cDialogueStep == 2)
                {
                    finishPoint(2);
                }
            }

            lastB1State_C = currentB1State_C;
        }

        else if (currentScreen == SCREEN_POINT_D)
        {
            GPIO_PinState currentB1State_D = HAL_GPIO_ReadPin(B1_GPIO_Port, B1_Pin);

            if (lastB1State_D == GPIO_PIN_SET && currentB1State_D == GPIO_PIN_RESET)
            {
                HAL_Delay(20);

                if (dDialogueStep == 0)
                {
                    dDialogueStep = 1;
                    drawD_Dialogue2();
                }
                else if (dDialogueStep == 1)
                {
                    dDialogueStep = 2;
                    drawD_Dialogue3();
                }
                 else if (dDialogueStep == 2)
                {
                    currentBattleIndex = 2;
                    currentScreen = SCREEN_BATTLE_B;
                    initBattleB();
                    drawBattle2Screen();

                    lastB1State_D = currentB1State_D;
                    continue;
                }
            }

            lastB1State_D = currentB1State_D;
        }

        else if (currentScreen == SCREEN_POINT_E)
        {
            GPIO_PinState currentB1State_E = HAL_GPIO_ReadPin(B1_GPIO_Port, B1_Pin);

            if (lastB1State_E == GPIO_PIN_SET && currentB1State_E == GPIO_PIN_RESET)
            {
                HAL_Delay(20);

                if (eDialogueStep == 0)
                {
                    eDialogueStep = 1;
                    drawE_Dialogue2();
                }
                 else if (eDialogueStep == 1)
                {
                    eDialogueStep = 2;
                    currentBattleIndex = 3;
                    currentScreen = SCREEN_BOSS;
                    initBossBattle();
                    drawBossBattleScreen();

                    lastB1State_E = currentB1State_E;
                    continue;
                }
            }
            lastB1State_E = currentB1State_E;
        }

        else if (currentScreen == SCREEN_END)
        {
            GPIO_PinState currentB1State = HAL_GPIO_ReadPin(B1_GPIO_Port, B1_Pin);

            if (lastB1State_E == GPIO_PIN_SET && currentB1State == GPIO_PIN_RESET)
            {
                HAL_Delay(20);
                return MENU_START;   //Exit game
            }

            lastB1State_E = currentB1State;
        }
    }
}