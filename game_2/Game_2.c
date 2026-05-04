#include "main.h"
#include "adc.h"
#include "usart.h"
#include "gpio.h"
#include "ST7789V2_Driver.h"
#include "joystick.h"
#include "Menu.h"
#include "stm32l4xx_hal_gpio.h"

void PeriphCommonClock_Config(void); // setups the peripheral clocks i.e. ADC, RNG etc.

#include "Joystick.h" // include the Joystick driver functions
#include "LCD.h" // include the LCD driver functions
#include <math.h> // for sqrtf, atan2f, etc.
#include <stdint.h> // for uint8_t etc. (already included in LCD.h but good practice to include if using uint8_t etc.)
#include <stdio.h> // for printf, sprintf etc.


MenuState Game2_Run(void)
{


    // Configure the LCD just like Unit 3.1 labs
  ST7789V2_cfg_t cfg0 = {
    .setup_done = 0,
    .spi = SPI2,
    .RST = {.port = GPIOB, .pin = GPIO_PIN_2},
    .BL = {.port = GPIOB, .pin = GPIO_PIN_1},
    .DC = {.port = GPIOB, .pin = GPIO_PIN_11},
    .CS = {.port = GPIOB, .pin = GPIO_PIN_12},
    .MOSI = {.port = GPIOB, .pin = GPIO_PIN_15},
    .SCLK = {.port = GPIOB, .pin = GPIO_PIN_13},
    .dma = {.instance = DMA1, .channel = DMA1_Channel5}
  };


  // Initialize LCD - note that this also powers on the display and backlight
  // and also if the display was previously powered on, it will probably show the last image
  // that was displayed before reset
  LCD_init(&cfg0);
  LCD_Fill_Buffer(0);
  LCD_Refresh(&cfg0);

    printf("Joystick Lab lets go!...\n");

  // Configure joystick
  Joystick_cfg_t joystick_cfg = {
    .adc = &hadc1, // ADC handle from MX_ADC1_Init()
    .x_channel = ADC_CHANNEL_1, // Internally the ADC_CHannel_1 is connected to pin A1 on the arduino header on the Nucleo board
    .y_channel = ADC_CHANNEL_2, // Internally the ADC_CHannel_2 is connected to pin A2 on the arduino header on the Nucleo board
    .sampling_time = ADC_SAMPLETIME_47CYCLES_5, // sampling time for ADC in cycles of the ADC clock () this is the miniumn required for the joystick value to stabilise
    .center_x = JOYSTICK_DEFAULT_CENTER_X,
    .center_y = JOYSTICK_DEFAULT_CENTER_Y,
    .deadzone = JOYSTICK_DEADZONE,
    .setup_done = 0
  };
  

  // Data structure to hold joystick readings
  Joystick_t joystick_data;
   Joystick_Init(&joystick_cfg);
  
  printf("Calibrating joystick - keep it centered...\n");
  Joystick_Calibrate(&joystick_cfg);
 int menu = 0;
int screen = 0;
int joy_ready = 1; 
int coins = 20;
int wheat_seed = 2;
int wheat = 0;
int fish = 0;

int crop_planted = 0;
int crop_ready = 0;
uint32_t plant_time = 0;

int water_count = 0;
int watered = 0;

uint32_t water_fx_time = 0;
int growth_bonus = 0;  
int fishing_state = 0;          // 0=idle, 1=waiting, 2=hooked, 3=caught
uint32_t fishing_start_time = 0;
uint32_t fishing_anim_time = 0;

int shop_menu = 0;
char status_msg[32] = "Welcome!";
uint32_t msg_time = 0;

int weather = 0;
int rare_fish = 0;
int caught_rare = 0;

 printf("Calibration complete: X=%d, Y=%d\n", joystick_cfg.center_x, joystick_cfg.center_y);

 
HAL_Delay(1000);

 void draw_weather_icon_big(void)
{
    if (weather == 0)
    {
        // Sun
        LCD_Draw_Rect(185, 18, 18, 18, 1, 1);   

        //Flashlight
        LCD_Draw_Rect(191, 8, 6, 6, 1, 1);
        LCD_Draw_Rect(191, 40, 6, 6, 1, 1);
        LCD_Draw_Rect(175, 24, 6, 6, 1, 1);
        LCD_Draw_Rect(207, 24, 6, 6, 1, 1);

         LCD_Draw_Rect(178, 11, 5, 5, 1, 1);
        LCD_Draw_Rect(205, 11, 5, 5, 1, 1);
        LCD_Draw_Rect(178, 37, 5, 5, 1, 1);
        LCD_Draw_Rect(205, 37, 5, 5, 1, 1);
    }
    else
    {
        // Cloud
        LCD_Draw_Rect(178, 20, 34, 10, 1, 1);
        LCD_Draw_Rect(184, 14, 10, 8, 1, 1);
        LCD_Draw_Rect(197, 12, 12, 10, 1, 1);

        //Raniy
        if (((HAL_GetTick() / 180) % 2) == 0)
        {
            LCD_Draw_Rect(184, 36, 3, 10, 1, 1);
            LCD_Draw_Rect(194, 36, 3, 10, 1, 1);
            LCD_Draw_Rect(204, 36, 3, 10, 1, 1);
        }
        else
        {
            LCD_Draw_Rect(189, 36, 3, 10, 1, 1);
            LCD_Draw_Rect(199, 36, 3, 10, 1, 1);
        }
    }
}

  while (1)
  {
 Joystick_Read(&joystick_cfg, &joystick_data);

    if (joystick_data.direction == CENTRE)
    {
        joy_ready = 1;
    }

    if (screen == 0)
    {
        if (joy_ready && joystick_data.direction == N)
        {
            menu--;
            joy_ready = 0;
        }
        else if (joy_ready && joystick_data.direction == S)
        {
            menu++;
            joy_ready = 0;
        }
        else if (joy_ready && joystick_data.direction == E)
        {
            if (menu == 4)
            {
            LCD_Fill_Buffer(0);
            LCD_Refresh(&cfg0);
            return MENU_STATE_HOME;
            }
            else
            {
            screen = menu + 1;
            }

            joy_ready = 0;
        }

        if (menu < 0) menu = 4;
        if (menu > 4) menu = 0;

        LCD_Fill_Buffer(0);
        LCD_printString("FARM GAME", 60, 40, 1, 3);

        if (menu == 0)
            LCD_printString("> Farm", 80, 90, 1, 3);
        else
            LCD_printString("Farm", 100, 90, 1, 3);

        if (menu == 1)
            LCD_printString("> Shop", 80, 120, 1, 3);
        else
            LCD_printString("Shop", 100, 120, 1, 3);

        if (menu == 2)
            LCD_printString("> Inventory", 80, 150, 1, 3);
        else
            LCD_printString("Inventory", 100, 150, 1, 3);

        if (menu == 3)
            LCD_printString("> Fishing", 80, 180, 1, 3);
        else
            LCD_printString("Fishing", 100, 180, 1, 3);

        if(menu == 4)
            LCD_printString("> Exit", 80, 210, 1, 3);
        else
            LCD_printString("Exit", 100, 210, 1, 3);

        LCD_Refresh(&cfg0);
    }
    
    else if (screen == 1)
    {
        uint32_t now = HAL_GetTick();
    uint32_t elapsed = 0;
    uint32_t grow_time = 15000;   // Normally 15s

    // Automaticlly watering in rainy days , growth time reduce to half 7.5s
    if (weather == 1)
    {
        grow_time = 7500;
    }

    if (crop_planted)
    {
        elapsed = now - plant_time;

        if (elapsed >= grow_time && watered)
            crop_ready = 1;
        else
            crop_ready = 0;
    }

    // LEFT -> Back
    if (joy_ready && joystick_data.direction == W)
    {
        screen = 0;
        joy_ready = 0;
    }
    // RIGHT -> Plant/Harvest
    else if (joy_ready && joystick_data.direction == E)
    {
        if (!crop_planted && wheat_seed > 0)
        {
            crop_planted = 1;
            crop_ready = 0;
            watered = 0;
            plant_time = HAL_GetTick();
            water_fx_time = 0;
            wheat_seed--;

            weather = rand() % 2;   // 0 sunny, 1 rainy

            // Automaticlly watering
            if (weather == 1)
            {
                watered = 1;
                water_fx_time = HAL_GetTick();
                sprintf(status_msg, "Rain waters crop");
                msg_time = HAL_GetTick();
            }
            else
            {
                sprintf(status_msg, "Sunny day");
                msg_time = HAL_GetTick();
            }

            joy_ready = 0;
        }
        else if (crop_planted && crop_ready)
        {
            crop_planted = 0;
            crop_ready = 0;
            watered = 0;
            plant_time = 0;
            water_fx_time = 0;
            wheat++;
            joy_ready = 0;

            sprintf(status_msg, "Harvest +1 wheat");
            msg_time = HAL_GetTick();
        }
    }
    // DOWN -> Watering 
    else if (joy_ready && joystick_data.direction == S)
    {
        if (crop_planted && !watered && weather == 0)
        {
            watered = 1;
            water_fx_time = HAL_GetTick();
            joy_ready = 0;

            sprintf(status_msg, "Watered");
            msg_time = HAL_GetTick();
        }
    }

    char buf[32];

    LCD_Fill_Buffer(0);

    LCD_printString("FARM", 85, 15, 1, 3);
    draw_weather_icon_big();

    sprintf(buf, "Seeds:%d", wheat_seed);
    LCD_printString(buf, 15, 45, 1, 2);

    sprintf(buf, "Wheat:%d", wheat);
    LCD_printString(buf, 15, 70, 1, 2);

    if (!crop_planted)
    {
        LCD_printString("Empty field", 45, 105, 1, 2);
        LCD_printString("RIGHT PLANT", 30, 145, 1, 2);
    }
    else
    {
        // Watering effect（600ms）
        if ((HAL_GetTick() - water_fx_time) < 600 && watered)
        {
            LCD_printString("WATER!", 150, 120, 1, 2);
        }

        // first stage of seed
        if (elapsed < (grow_time / 2))
        {
            LCD_printString("SEED", 85, 95, 1, 2);
            LCD_printString("  .  ", 90, 130, 1, 2);
        }
        else
        {
            // the last stage of seed and harvest ,all of them keep wheat animation
            int wobble = (((HAL_GetTick() / 250) % 2) == 0) ? 0 : 3;

            LCD_printString("WHEAT", 75, 90, 1, 2);
            LCD_printString(" \\|/ ", 80, 120 + wobble, 1, 2);
            LCD_printString("--*--", 75, 140 + wobble, 1, 2);
        }

        // State
        if (watered)
            LCD_printString("Water:YES", 15, 175, 1, 1);
        else
            LCD_printString("Water:NO", 15, 175, 1, 1);

        if (weather == 0)
            LCD_printString("Sun", 140, 175, 1, 1);
        else
            LCD_printString("Rain", 140, 175, 1, 1);

        // Harvest hint
        if (crop_ready)
        {
            if (((HAL_GetTick() / 300) % 2) == 0)
            {
                LCD_printString("READY!", 150, 120, 1, 2);
            }

            LCD_printString("RIGHT HARVEST", 20, 195, 1, 1);
        }
        else
        {
            if (!watered && weather == 0)
                LCD_printString("DOWN WATER", 25, 195, 1, 1);
        }
    }

    LCD_printString("LEFT BACK", 35, 215, 1, 1);

    if (HAL_GetTick() - msg_time < 2000)
    {
        LCD_printString(status_msg, 10, 225, 1, 1);
    }

    LCD_Refresh(&cfg0);   
    }
 
    else if (screen == 2)
    {
       if (joy_ready && joystick_data.direction == W)
    {
        screen = 0;
        joy_ready = 0;
    }
    else if (joy_ready && joystick_data.direction == N)
    {
        shop_menu--;
        joy_ready = 0;
    }
    else if (joy_ready && joystick_data.direction == S)
    {
        shop_menu++;
        joy_ready = 0;
    }
    else if (joy_ready && joystick_data.direction == E)
    {
         if (shop_menu == 0)
    {
        if (coins >= 2)
    {
        coins -= 2;
        wheat_seed++;
        sprintf(status_msg, "Bought 1 seed");
        msg_time = HAL_GetTick();
    }
}
else if (shop_menu == 1)   // Sell Wheat
{
    if (wheat > 0)
    {
        wheat--;
        coins += 3;
        sprintf(status_msg, "Sold wheat +3");
        msg_time = HAL_GetTick();
    }
}
else if (shop_menu == 2)   // Sell Fish
{
    if (fish > 0)
    {
        fish--;
        coins += 5;
        sprintf(status_msg, "Sold fish +5");
        msg_time = HAL_GetTick();
    }
}
else if (shop_menu == 3)   // Sell Rare Fish
{
    if (rare_fish > 0)
    {
        rare_fish--;
        coins += 12;   // Rare fish has higher price
        sprintf(status_msg, "Sold rare fish +12");
        msg_time = HAL_GetTick();
    }
}
        joy_ready = 0;
    }

    if (shop_menu < 0) shop_menu = 3;
    if (shop_menu > 3) shop_menu = 0;

    char buf[32];

    LCD_Fill_Buffer(0);

    LCD_printString("SHOP", 80, 15, 1, 3);

   if (shop_menu == 0)
    LCD_printString("> Buy Seeds -2", 15, 55, 1, 2);
else
    LCD_printString("Buy Seeds -2", 30, 55, 1, 2);

if (shop_menu == 1)
    LCD_printString("> Sell Wheat +3", 15, 85, 1, 2);
else
    LCD_printString("Sell Wheat +3", 30, 85, 1, 2);

if (shop_menu == 2)
    LCD_printString("> Sell Fish +5", 15, 115, 1, 2);
else
    LCD_printString("Sell Fish +5", 30, 115, 1, 2);

if (shop_menu == 3)
    LCD_printString("> Rare Fish +12", 15, 145, 1, 2);
else
    LCD_printString("Rare Fish +12", 30, 145, 1, 2);

  
    sprintf(buf, "Coins:%d", coins);
    LCD_printString(buf, 20, 170, 1, 2);

    sprintf(buf, "Seeds:%d", wheat_seed);
    LCD_printString(buf, 20, 195, 1, 2);

    LCD_printString("LEFT BACK", 125, 205, 1, 1);
if (HAL_GetTick() - msg_time < 2000)
{
    LCD_printString(status_msg, 10, 225, 1, 1);
}

    LCD_Refresh(&cfg0);
    }
   
    else if (screen == 3)
    {
      if (joy_ready && joystick_data.direction == W)
    {
        screen = 0;
        joy_ready = 0;
    }

    char buf[32];

    LCD_Fill_Buffer(0);
    LCD_printString("INVENTORY", 40, 20, 1, 3);

sprintf(buf, "Coins:%d", coins);
LCD_printString(buf, 20, 55, 1, 2);

sprintf(buf, "Seeds:%d", wheat_seed);
LCD_printString(buf, 20, 85, 1, 2);

sprintf(buf, "Wheat:%d", wheat);
LCD_printString(buf, 20, 115, 1, 2);

sprintf(buf, "Fish:%d", fish);
LCD_printString(buf, 20, 145, 1, 2);

sprintf(buf, "Rare fish:%d", rare_fish);
LCD_printString(buf, 20, 175, 1, 2);

LCD_printString("LEFT BACK", 35, 205, 1, 1);
   
if (HAL_GetTick() - msg_time < 2000)
{
    LCD_printString(status_msg, 10, 225, 1, 1);
}

    LCD_Refresh(&cfg0);
}
    
    
    
    else if (screen == 4)
    {
        uint32_t now = HAL_GetTick();
    int fish_x = 150;
    int fish_y = 145;

    // LEFT -> back
    if (joy_ready && joystick_data.direction == W)
    {
        screen = 0;
        joy_ready = 0;
        fishing_state = 0;
    }
    // RIGHT -> start fishing
    else if (joy_ready && joystick_data.direction == E)
    {
        if (fishing_state == 0)
        {
            fishing_state = 1;
            fishing_start_time = HAL_GetTick();
            fishing_anim_time = HAL_GetTick();
            joy_ready = 0;
        }
        else if (fishing_state == 3)
        {
            fishing_state = 0;
            joy_ready = 0;
        }
    }

    // State
    if (fishing_state == 1)
    {
       
        if (now - fishing_start_time > 2000)
        {
            if ((rand() % 100) < 60)
            {
                fishing_state = 2;   // hooked
                fishing_anim_time = HAL_GetTick();
                 if (coins >= 40)
            {
                // the probablity of catch an rare fish rise when player has moer than 40 coins
                if ((rand() % 100) < 30)
                    caught_rare = 1;
                else
                    caught_rare = 0;
            }
            else
            {
                //Normally catch an rare fish only has 10% probablity 
                if ((rand() % 100) < 10)
                    caught_rare = 1;
                else
                    caught_rare = 0;
            }
                
            }
            else
            {
                fishing_state = 0;   // fail
            caught_rare = 0;

            sprintf(status_msg, "No fish...");
            msg_time = HAL_GetTick();

            }
        }
    }
    else if (fishing_state == 2)
    {
        // The special effect of catching fish (1s)
        if (now - fishing_anim_time > 1000)
        {
            fishing_state = 3;
             if (caught_rare)
        {
            rare_fish++;
            sprintf(status_msg, "Rare fish!");
        }
        else
        {
            fish++;
            sprintf(status_msg, "Caught fish!");
        }
            msg_time = HAL_GetTick();
        }
    }

    LCD_Fill_Buffer(0);

    // Title
    LCD_printString("FISHING", 65, 10, 1, 3);

    // ---------- scene ----------
    // river bank
    LCD_printString("------------------------------", 0, 58, 1, 1);

    // water face
    LCD_printString("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~", 0, 125, 1, 1);
    LCD_printString("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~", 0, 145, 1, 1);
    LCD_printString("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~", 0, 165, 1, 1);

    
    LCD_printString(" /\\_/\\\\ ", 5, 70, 1, 1);   // 头
    LCD_printString("( o.o )", 5, 85, 1, 1);
    LCD_printString(" > ^ < ", 5, 100, 1, 1);

   
    // arm
    LCD_printString("____", 40, 108, 1, 1);

    // ---------- fishing rod----------
    // fishing rod
    LCD_printString("\\", 72, 92, 1, 2);
    LCD_printString("\\", 82, 102, 1, 2);
    LCD_printString("\\", 92, 112, 1, 2);

    // fish tape
    LCD_printString("|", 108, 112, 1, 2);
    LCD_printString("|", 108, 124, 1, 2);

    // fish hook
    LCD_printString("J", 104, 134, 1, 2);

    // ---------- fish ----------
    if (fishing_state == 0)
    {
        // swing left and right while in standby mode
        if (((HAL_GetTick() / 300) % 2) == 0)
        {
            LCD_printString("><>", 145, 150, 1, 2);
        }
        else
        {
            LCD_printString("<><", 145, 150, 1, 2);
        }

        LCD_printString("RIGHT FISH", 30, 190, 1, 2);
    }
    else if (fishing_state == 1)
    {
        // waiting 
        if (((HAL_GetTick() / 250) % 2) == 0)
        {
            fish_x = 140;
        }
        else
        {
            fish_x = 150;
        }

        LCD_printString("><>", fish_x, 150, 1, 2);
        LCD_printString("WAIT...", 75, 190, 1, 2);
    }
    else if (fishing_state == 2)
    {
      
        int lift = (now - fishing_anim_time) / 120;
        if (lift > 6) lift = 6;

        fish_y = 150 - (lift * 8);

        LCD_printString("><>", 120, fish_y, 1, 2);

        if (((HAL_GetTick() / 200) % 2) == 0)
        {
            LCD_printString("HOOKED!", 65, 190, 1, 2);
        }
    }
    else if (fishing_state == 3)
    {
      
        LCD_printString("><>", 95, 95, 1, 2);
        if (caught_rare)
        LCD_printString("RARE FISH!", 45, 190, 1, 2);
    else
        LCD_printString("CAUGHT!", 65, 190, 1, 2);
    }


    {
        char buf[32];
        sprintf(buf, "Fish:%d", fish);
        LCD_printString(buf, 10, 215, 1, 1);
    }

    LCD_printString("LEFT BACK", 120, 215, 1, 1);

    if (HAL_GetTick() - msg_time < 2000)
{
    LCD_printString(status_msg, 10, 225, 1, 1);
}
    
    LCD_Refresh(&cfg0);
    }

    HAL_Delay(120);
 }

}
