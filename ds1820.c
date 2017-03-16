#include "ds1820.h"

//#define ONE_WIRE_IN PORTBbits.RB5
//#define ONE_WIRE_OUT LATBbits.LATB5
//#define ONE_WIRE_TRIS TRISBbits.TRISB5

#define delay1mks asm("nop"); asm("nop"); asm("nop"); asm("nop")

void delay_us(uint16 period)
{
	 uint16 i;
	 for (i = 0; i < period; i++)
	 {
		nop(); nop(); nop(); nop();
                nop(); nop(); nop(); nop();
                nop(); nop(); nop(); nop();
                nop(); nop(); nop(); nop();
	 }
}

void onewire_reset()
{

 ONE_WIRE_OUT();
 delay_us(400 ); // pull 1-wire low for reset pulse
 ONE_WIRE_IN();
 delay_us( 400 ); // wait-out remaining initialisation window.
}

/*********************** onewire_write() ********************************/
/*This function writes a byte to the sensor.*/
/* */
/*Parameters: byte - the byte to be written to the 1-wire */
/*Returns: */
/*********************************************************************/

void onewire_write(uint8 data)
{
 uint8 count;
 uint8 databuf;
 databuf = data;

 for (count=0; count<8; ++count)
 {
   __disable_interrupt();
   ONE_WIRE_OUT();
    delay_us(2);  // pull 1-wire low to initiate write time-slot.
    if (databuf & 0x01)
    {
      ONE_WIRE_IN();
    }
   __enable_interrupt();
   databuf = databuf >> 1;
  delay_us( 60 ); // wait until end of write slot.
  ONE_WIRE_IN(); // set 1-wire high again,
  delay_us( 2 ); // for more than 1us minimum.
 }
}


uint8 onewire_read()
{
 uint8 count, data;
 data = 0;
 for (count=0; count<8; ++count)
 {
   __disable_interrupt();
   ONE_WIRE_OUT();
   delay_us(2); // pull 1-wire low to initiate read time-slot.
   ONE_WIRE_IN(); // now let 1-wire float high,
 	// let device state stabilise,
   delay_us(6);
   if (ONE_WIRE_READ())
     data = data | 0x80;
   __enable_interrupt();
   if (count < 7)
   {
   	data = (data >> 1) & 0x7f;
   }
  delay_us( 80 ); // wait until end of read slot.  вообще-то в оригинале здесь 120мкс, но процедурка моя не выдерживает строго 1мкс
 }

 return( data );
}

/*

float ds1820_read()
{
 int8 busy=0;

 uint8 temp1, temp2;
 uint16 temp3;
 float result;

 onewire_reset();
 onewire_write(0xCC);
 onewire_write(0x44);

 while (busy == 0)
  busy = onewire_read();

 onewire_reset();
 onewire_write(0xCC);
 onewire_write(0xBE);
 temp1 = onewire_read();   // D2
 temp2 = onewire_read();  // 01

 temp3 = (uint16) temp2 * (uint16)0x0100L + (uint16) temp1;
 //result = (float) temp3 / 2.0;   //Calculation for DS18S20 with 0.5 deg C resolution
 result = (float)temp3 / 16.0; //Calculation for DS18B20 with 0.1 deg C resolution
 result = temp3/16.0;

 return(result);
}

*/

void ds1820_set9bit(void)
{
	onewire_reset();
	onewire_write(0xCC);
	onewire_write(0x4E);
	onewire_write(0x00);
	onewire_write(0x00);
	onewire_write(0x1F);
}

void ds1820_startconversion(void)
{
	onewire_reset();
	onewire_write(0xCC);
	onewire_write(0x44);
}

//***************************************************************************
// CRC8
// Для серийного номера вызывать 8 раз
// Для данных вызвать 9 раз
// Если в результате crc == 0, то чтение успешно
//***************************************************************************
uint8_t crc8 (uint8_t data, uint8_t crc)
#define CRC8INIT   0x00
#define CRC8POLY   0x18              //0X18 = X^8+X^5+X^4+X^0
{
	uint8_t bit_counter;
	uint8_t feedback_bit;
	bit_counter = 8;
	do
	{
		feedback_bit = (crc ^ data) & 0x01;
		if ( feedback_bit == 0x01 ) crc = crc ^ CRC8POLY;
		crc = (crc >> 1) & 0x7F;
		if ( feedback_bit == 0x01 ) crc = crc | 0x80;
		data = data >> 1;
		bit_counter--;
	}  while (bit_counter > 0);
	return crc;
}



int16 ds1820_readtemp(void)
{
	uint8 temp1, temp2, t3;
	int16 temp3;

	onewire_reset();
	onewire_write(0xCC);
	onewire_write(0xBE);

        uint8_t crc = 0;
        uint8_t crc_buf[9];
        for (t3 = 0; t3 < 9; t3++)
        {
          //__disable_interrupt();
          crc_buf[t3] = onewire_read();
          //__enable_interrupt();
        }


        for (t3 = 0; t3 < 9; t3++)
          crc = crc8(crc_buf[t3], crc);

        if (crc == 0)
          temp3 = (int16_t) ((uint16_t)(crc_buf[1] * 256) + (uint16_t)crc_buf[0]);
        else
          temp3 = 0;
	//temp1 = onewire_read();   // D2
	//temp2 = onewire_read();  // 01
	//t3 = onewire_read();
	//t3 = onewire_read();
	//t3 = onewire_read();

	//temp3 = (uint16) temp2 * (uint16)0x0100L + (uint16) temp1;
 	//temp3 = temp3 / 16; // целочисленное деление на 16
	return temp3;
}
