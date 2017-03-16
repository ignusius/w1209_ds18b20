#ifndef	_DS18B20
#define	_DS18B20


#include "stm8s.h"
#define uint8 u8
#define int16 s16
#define uint16 u16

#define ONE_WIRE_IN()  GPIOD->CR1 |= GPIO_PIN_6;GPIOD->DDR &= ~GPIO_PIN_6
#define ONE_WIRE_READ() (GPIOD->IDR & GPIO_PIN_6)
#define ONE_WIRE_OUT() GPIOD->CR1 &= ~GPIO_PIN_6;GPIOD->DDR |= GPIO_PIN_6

extern int16 ds1820_readtemp(void);
extern void ds1820_startconversion(void);
extern void ds1820_set9bit(void);
//extern float ds1820_read(void);
//int8 onewire_read(void);
//void onewire_write(int8 data);
//void onewire_reset(void);
void delay_us(uint16 period);
#endif

