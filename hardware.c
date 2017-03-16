#include "hardware.h"
#include "stm8s.h"
#include"stm8s_clk.h"
#include "stm8s_gpio.h"
#include "stm8s_itc.h"
#include "stm8s_tim4.h"
#include "stm8s_flash.h"

#include "ltkrn.h"


uint16_t *eeToken = (uint16_t*)FLASH_DATA_START_PHYSICAL_ADDRESS;
int16_t *eeTemp1 = (int16_t *)(FLASH_DATA_START_PHYSICAL_ADDRESS + 2);
uint8_t *eeTempMode = (uint8_t *)(FLASH_DATA_START_PHYSICAL_ADDRESS + 4);


uint32_t sys_msec = 0;
uint32_t sys_sec = 0;

uint8_t _DIG_MASK[][2] =
{
  {0x00, 0xEF}, // B, D порты. маска обнулени€
  {0x20, 0xFF},
  {0x10, 0xFF}
};


/*
 A D5
 B A2
 C C7
 D D3
 E D1
 F A1
 G C6
DP D2
*/
uint8_t _SYM_MASK[3] = {0xF9, 0xD1, 0x3F}; // A, D, C маски чтобы не затронуть остальные биты портов
uint8_t _SYM[][3] =
{
  { 1 << 2 | 1 << 1, 1 << 5 | 1 << 1 | 1 << 3, 1 << 7}, // 0
  {1 << 2, 0, 1 << 7}, // 1
  {1 << 2, 1 << 5 | 1 << 1 | 1 << 3, 1 << 6}, // 2
  {1 << 2, 1 << 3 | 1 << 5, 1<<6 | 1 << 7}, // 3
  {1 << 2 | 1 << 1, 0, 1 << 6 | 1 << 7}, // 4
  {1 << 1, 1 << 5 | 1 << 3, 1 << 6 | 1 << 7}, // 5
  {1 << 1, 1 << 1 | 1 << 5 | 1 << 3, 1 << 6 | 1 << 7}, // 6
  {1 << 2, 1 << 5, 1 << 7}, // 7
  {1 << 1 | 1 << 2, 1 << 5 | 1 << 1 | 1 << 3, 1 << 6 | 1 << 7}, // 8
  {1 << 1 | 1 << 2, 1 << 5 | 1 << 3, 1 << 6 | 1 << 7}, // 9
  {0x00, 0x00, 0x00}, //
};

uint8_t LED[3] = {0x00, 0x00, 0x00};
uint8_t LED_DP[3] = {0x00, 0x00, 0x00};
uint8_t LED_OFF[3] = {0x00, 0x00, 0x00};
uint8_t LED_CURR = 0;

uint8_t _GPIOB, _GPIOD;

uint8_t delay = 0;

// переполнение таймера 4
INTERRUPT_HANDLER(TIM4_handler,ITC_IRQ_TIM4_OVF)
{

  krn_timer_warp --;
  if (krn_timer_warp == 0)
  {
    krn_timer_cnt ++;
    krn_timer_current ++;
    krn_timer_warp = 10;
    krn_dispatch();
  }


  TIM4->SR1 = (uint8_t)(~((uint8_t)(TIM4_IT_UPDATE)));
  sys_msec++;
  if (sys_msec == 1000)
  {
    sys_sec++;
    sys_msec = 0;
  }

  // выключим отображение
  DIG0_OFF;
  DIG1_OFF;
  DIG2_OFF;

  // занесем новые данные по сегментам
  GPIOA->ODR = GPIOA->ODR & _SYM_MASK[0];
  GPIOD->ODR = GPIOD->ODR & _SYM_MASK[1];
  GPIOC->ODR = GPIOC->ODR & _SYM_MASK[2];

  GPIOA->ODR = GPIOA->ODR | _SYM[LED[LED_CURR]][0];
  GPIOD->ODR = GPIOD->ODR | _SYM[LED[LED_CURR]][1];
  GPIOC->ODR = GPIOC->ODR | _SYM[LED[LED_CURR]][2];

  // включаем нужный разр€д
  if (LED_OFF[LED_CURR] == 0)
  {
    GPIOB->DDR = GPIOB->DDR | _DIG_MASK[LED_CURR][0];
    GPIOD->ODR = GPIOD->ODR & _DIG_MASK[LED_CURR][1];
  }

  if (LED_DP[LED_CURR] & 0x01) // точку включаем отдельно
     SEG_DP_ON;

  LED_CURR++;
  if (LED_CURR > 2)
    LED_CURR = 0;

}

// проверка, надо ли инициализировать значени€ по умолчанию в еепром контроллера
void ee_CheckDefaultValues()
{
   if (*eeToken != 0xAEEA)
   {
     FLASH_Unlock(FLASH_MEMTYPE_DATA);
     *eeToken = 0xAEEA;
     *eeTemp1 = DEFAULT_TEMP1_VALUE;
     *eeTempMode = DEFAULT_TEMP_MODE;
     FLASH_Lock(FLASH_MEMTYPE_DATA);
   }
}

void ee_ResetToDefaults()
{
    FLASH_Unlock(FLASH_MEMTYPE_DATA);
    *eeToken = 0x0000;
    FLASH_Lock(FLASH_MEMTYPE_DATA);
    ee_CheckDefaultValues();
}

void ee_Update_Temp1_Value(int16_t value)
{
  FLASH_Unlock(FLASH_MEMTYPE_DATA);
  *eeTemp1 = value;
  FLASH_Lock(FLASH_MEMTYPE_DATA);
}

void ee_Update_TempMode_Value(uint8_t value)
{
  FLASH_Unlock(FLASH_MEMTYPE_DATA);
  *eeTempMode = value;
  FLASH_Lock(FLASH_MEMTYPE_DATA);
}

void hardware_init(void)
{
  //инициализаци€ тактовой системы, HSI 16MHz
  CLK_DeInit();
  CLK_SYSCLKConfig(CLK_PRESCALER_HSIDIV1);
  CLK_ClockSwitchConfig(CLK_SWITCHMODE_AUTO, CLK_SOURCE_HSI, DISABLE, CLK_CURRENTCLOCKSTATE_DISABLE);;


  // таймер4 займетс€ распределением времени, одно переполнение за 1мс
  CLK_PeripheralClockConfig(CLK_PERIPHERAL_TIMER4,ENABLE);
  TIM4_TimeBaseInit(TIM4_PRESCALER_128, 125 - 1);
  TIM4_ClearFlag(TIM4_FLAG_UPDATE);
  TIM4_UpdateRequestConfig(TIM4_UPDATESOURCE_REGULAR);
  TIM4_ITConfig(TIM4_IT_UPDATE,ENABLE);
  TIM4_Cmd(ENABLE);

  //кнопки управлени€
  GPIO_Init(GPIOC,GPIO_PIN_3,GPIO_MODE_IN_PU_NO_IT); //SET
  GPIO_Init(GPIOC,GPIO_PIN_4,GPIO_MODE_IN_PU_NO_IT); //PLUS
  GPIO_Init(GPIOC,GPIO_PIN_5,GPIO_MODE_IN_PU_NO_IT); //MINUS

  // собственно, сегменты
  GPIO_Init(GPIOD,GPIO_PIN_1,GPIO_MODE_OUT_PP_LOW_FAST); //
  GPIO_Init(GPIOD,GPIO_PIN_2,GPIO_MODE_OUT_PP_LOW_FAST); //
  GPIO_Init(GPIOD,GPIO_PIN_3,GPIO_MODE_OUT_PP_LOW_FAST); //
  GPIO_Init(GPIOD,GPIO_PIN_5,GPIO_MODE_OUT_PP_LOW_FAST); //5

  GPIO_Init(GPIOA,GPIO_PIN_1,GPIO_MODE_OUT_PP_LOW_FAST); //2
  GPIO_Init(GPIOA,GPIO_PIN_2,GPIO_MODE_OUT_PP_LOW_FAST); //6

  GPIO_Init(GPIOC,GPIO_PIN_6,GPIO_MODE_OUT_PP_LOW_FAST); //16
  GPIO_Init(GPIOC,GPIO_PIN_7,GPIO_MODE_OUT_PP_LOW_FAST); //17


  //управление светодиодным индикатором. он у нас €вно с общим катодом
  GPIO_Init(GPIOB,GPIO_PIN_4,GPIO_MODE_IN_FL_NO_IT); //
  GPIO_Init(GPIOB,GPIO_PIN_5,GPIO_MODE_IN_FL_NO_IT); //
  GPIO_Init(GPIOD,GPIO_PIN_4,GPIO_MODE_OUT_PP_HIGH_FAST); //CH3
  GPIOB->ODR = GPIOB->ODR & 0xCF;

  GPIO_Init(GPIOD,GPIO_PIN_6,GPIO_MODE_IN_FL_NO_IT); //термодатчик
  GPIO_Init(GPIOA,GPIO_PIN_3,GPIO_MODE_OUT_PP_LOW_FAST); //реле
}

