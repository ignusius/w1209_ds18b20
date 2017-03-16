#ifndef __hardware_h
#define __hardware_h

#include "stm8s.h"

extern uint32_t sys_msec;
extern uint32_t sys_sec;
extern uint8_t LED[3];
extern uint8_t LED_DP[3];
extern uint8_t LED_OFF[3];

#define PWM_OUT_SET_LOW GPIOD->ODR &= ~GPIO_PIN_4

#define DIG0_ON GPIOD->ODR &= ~GPIO_PIN_4
#define DIG1_ON GPIOB->DDR |= GPIO_PIN_5
#define DIG2_ON GPIOB->DDR |= GPIO_PIN_4

#define DIG0_OFF GPIOD->ODR |= GPIO_PIN_4
#define DIG1_OFF GPIOB->DDR &= ~GPIO_PIN_5
#define DIG2_OFF GPIOB->DDR &= ~GPIO_PIN_4

#define SEG_A_ON GPIOD->ODR |= GPIO_PIN_5
#define SEG_B_ON GPIOA->ODR |= GPIO_PIN_2
#define SEG_C_ON GPIOC->ODR |= GPIO_PIN_7
#define SEG_D_ON GPIOD->ODR |= GPIO_PIN_3
#define SEG_E_ON GPIOD->ODR |= GPIO_PIN_1
#define SEG_F_ON GPIOA->ODR |= GPIO_PIN_1
#define SEG_G_ON GPIOC->ODR |= GPIO_PIN_6
#define SEG_DP_ON GPIOD->ODR |= GPIO_PIN_2

#define PIN_BUTTON_SET ((GPIOC->IDR & GPIO_PIN_3) == 0x00)
#define PIN_BUTTON_PLUS ((GPIOC->IDR & GPIO_PIN_4) == 0x00)
#define PIN_BUTTON_MINUS ((GPIOC->IDR & GPIO_PIN_5) == 0x00)

#define RELAY_ON GPIOA->ODR |= GPIO_PIN_3
#define RELAY_OFF GPIOA->ODR &= ~GPIO_PIN_3


// значение по умолчанию температуры для индикатора. в десятых долях градуса
#define DEFAULT_TEMP1_VALUE (int16_t)300
#define TEMP_MODE_ON_IF_LOWER 0
#define TEMP_MODE_ON_OVER 1
#define DEFAULT_TEMP_MODE TEMP_MODE_ON_IF_LOWER


extern int16_t *eeTemp1;
extern uint8_t *eeTempMode;


void hardware_init(void);
void ee_Update_Temp1_Value(int16_t value);
void ee_Update_TempMode_Value(uint8_t value);
void ee_CheckDefaultValues();


#endif