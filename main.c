#include "hardware.h"
#include "ds1820.h"

#include "string.h"
#include "ltkrn.h"

#define OS_THREAD_STACK 80
#define MAIN_THREAD_STACK 80

#define button_thread_stack (void*)(KRN_STACKFRAME - 1 * MAIN_THREAD_STACK)
#define temp_thread_stack (void*)(KRN_STACKFRAME - 2 * MAIN_THREAD_STACK)
#define display_thread_stack (void*)(KRN_STACKFRAME - 3 * MAIN_THREAD_STACK)
#define user_thread_stack (void*)(KRN_STACKFRAME - 4 * MAIN_THREAD_STACK)

static krn_thread thr_temp, thr_display, thr_button, thr_user;

#define DISPLAY_MODE_TEMP 0
#define DISPLAY_MODE_TEMP_SET 1
#define DISPLAY_MODE_FUNC 2
#define DISPLAY_MODIFIER_NONE 0
#define DISPLAY_MODIFIER_BLINK 1

uint8_t display_mode = DISPLAY_MODE_TEMP;
uint8_t display_modifier = DISPLAY_MODIFIER_NONE, display_submode = 0;
uint8_t display_blink_dig[3] = {0x01, 0x01, 0x01}; // значения по умолчанию мерцать всеми
uint16_t display_blink_cnt = 0, display_blink_value = 20;

int16_t ds_temp = 0;
uint8_t setup_mode = 0;

void display_value(int16_t value);

static NO_REG_SAVE void temp_thread_func (void)
{
  while(1)
  {
    ds1820_startconversion();
    krn_sleep(KRN_FREQ);  // ждем одну секунду
    ds_temp = ds1820_readtemp()/10;
    ds_temp = (ds_temp * 10)/16;

      if (ds_temp <= *eeTemp1-*eeTempMode)
        if (ds_temp !=0){
          RELAY_ON;
        }
        else {
         //
          //
        }
      if (ds_temp >= *eeTemp1)
        RELAY_OFF;
    }
    /*if (*eeTempMode == TEMP_MODE_ON_OVER)
    {
      if (ds_temp >= *eeTemp1)
        RELAY_ON;
      else
        RELAY_OFF;
    }*/
    krn_sleep(KRN_FREQ);
  }



#define BUTTON_HOLD_TICK 30

#define BUTTON_SET 0x01
#define BUTTON_SET_MASK 0xfe
#define BUTTON_PLUS 0x02
#define BUTTON_PLUS_MASK 0xfd
#define BUTTON_MINUS 0x04
#define BUTTON_MINUS_MASK 0xfb


typedef struct
{
  uint8_t JustPressed; // только что нажатые кнопки
  uint8_t JustReleased; // только что отпущеные кнопки
  uint8_t Pressed;  // нажатые в текущий момент кнопки
  uint8_t Hold;  // удерживаемые в текущий момент кнопки
  uint8_t count_btn1; // счетчики удержания кнопок 1 и 2
  uint8_t count_btn2;
  uint8_t count_btn3;

} TBUTTONS;



volatile TBUTTONS buttons;

static NO_REG_SAVE void button_thread_func (void)
{
  static uint8_t prev_state = 0x00;
  uint8_t _temp;

  buttons.count_btn1 = 0;
  buttons.count_btn2 = 0;
  buttons.Pressed = 0;
  buttons.JustPressed = 0;
  buttons.JustReleased = 0;

  for (;;)
  {
    krn_sleep(KRN_FREQ/50); // выждем примерно 20мс

    _temp = 0x00; // очистили статус

    if (PIN_BUTTON_SET)
      _temp |= 0x01;
    if (PIN_BUTTON_PLUS)
      _temp |= 0x02;
    if (PIN_BUTTON_MINUS)
      _temp |= 0x04;

    _temp ^= prev_state;	// xor на предыдущее состояние, узнаем что поменялось
    prev_state ^= _temp; // в prev появится текущее состояние пинов

    if (_temp) // подавление дребезга
    {
      continue;
    }
    _temp = prev_state ^ buttons.Pressed; // вычисляем маску изменения нажатия
    buttons.JustPressed |= (_temp & prev_state);
    if (_temp & buttons.Pressed) // проверяем только переход в ненажатое состояние
    {
      buttons.JustReleased |= _temp & buttons.Pressed;
    }

    buttons.Pressed = prev_state; // ставим что нажалось

    // проверка удержания кнопок, только кнопки 1 и 2.
    if (buttons.Pressed & 0x01)
    {
      buttons.count_btn1++;
      if (buttons.count_btn1 >= BUTTON_HOLD_TICK)
      {
        buttons.Hold |= 0x01; // ставим признак удержания
      }
    }
    else
    {
      buttons.count_btn1 = 0;
      buttons.Hold &= 0xfe; // снимаем флаг удержания, если был
    }
    if (buttons.Pressed & 0x02)
    {
      buttons.count_btn2++;
      if (buttons.count_btn2 >= BUTTON_HOLD_TICK)
      {
        buttons.Hold |= 0x02; // ставим признак удержания
      }

    }
    else
    {
      buttons.count_btn2 = 0;
      buttons.Hold &= 0xfd; // снимаем флаг удержания, если был
    }
    if (buttons.Pressed & 0x04)
    {
      buttons.count_btn3++;
      if (buttons.count_btn3 >= BUTTON_HOLD_TICK)
      {
        buttons.Hold |= 0x04; // ставим признак удержания
      }

    }
    else
    {
      buttons.count_btn3 = 0;
      buttons.Hold &= 0xfb; // снимаем флаг удержания, если был
    }

  }
}


uint16_t display_temp;
uint8_t display_func;

static NO_REG_SAVE void user_thread_func (void)
{
  while (1)
  {
    if (buttons.JustPressed & BUTTON_SET)
    {
      buttons.JustPressed &= BUTTON_SET_MASK;
      if (display_mode == DISPLAY_MODE_TEMP)
      {
        // переходим в режим настройки границы температуры
        display_modifier = DISPLAY_MODIFIER_BLINK;
        display_mode = DISPLAY_MODE_TEMP_SET;
        display_temp = *eeTemp1;
      }
    }

    if (display_mode == DISPLAY_MODE_TEMP_SET)
    {
      
      if (buttons.JustPressed & BUTTON_PLUS)
      {
        buttons.JustPressed &= BUTTON_PLUS_MASK;
        /*if (display_temp < 125)*/ display_temp++;
      }
      if (buttons.JustPressed & BUTTON_MINUS)
      {
        buttons.JustPressed &= BUTTON_MINUS_MASK;
        /*if (display_temp > 0)*/ display_temp--;
      }
      while (buttons.Hold & BUTTON_PLUS)
      {
        /*if (display_temp < 125)*/ display_temp += 10;
        krn_sleep(KRN_FREQ/2);
      }
      while (buttons.Hold & BUTTON_MINUS)
      {
        /*if (display_temp > 0)*/ display_temp -= 10;
        krn_sleep(KRN_FREQ/2);
      }
      if (buttons.JustPressed & BUTTON_SET)
      {
        buttons.JustPressed &= BUTTON_SET_MASK;
        display_mode = DISPLAY_MODE_TEMP;
        display_modifier = DISPLAY_MODIFIER_NONE;
        LED_OFF[0] = 0; LED_OFF[1] = 0; LED_OFF[2] = 0;
        ee_Update_Temp1_Value(display_temp);
      }
      if (buttons.Hold & BUTTON_SET)
      {
        buttons.JustPressed &= BUTTON_SET_MASK;
        display_mode = DISPLAY_MODE_FUNC;
        display_modifier = DISPLAY_MODIFIER_NONE;
        LED_OFF[0] = 0; LED_OFF[1] = 0; LED_OFF[2] = 0;
        LED_DP[1] = 0; LED[1] = 0x0a;
        display_func = *eeTempMode;
        LED[2] = display_func;
        //while (buttons.Hold & BUTTON_SET);
      }
    }
    if (display_mode == DISPLAY_MODE_FUNC)
    {
      buttons.JustPressed &= BUTTON_SET;
      while (!(buttons.JustPressed & BUTTON_SET))
      {
        LED[1] = 0x0a; LED[2] = display_func;
        if (buttons.JustPressed & BUTTON_PLUS)
        {
          buttons.JustPressed &= BUTTON_PLUS_MASK;
          if (display_func<9) display_func ++;
        }
        if (buttons.JustPressed & BUTTON_MINUS)
        {
          buttons.JustPressed &= BUTTON_MINUS_MASK;
          if (display_func>0) display_func --;
        }
        krn_sleep(1);
      }
      buttons.JustPressed &= BUTTON_SET_MASK;
      LED_DP[1] = 1;
      display_mode = DISPLAY_MODE_TEMP;
      display_modifier = DISPLAY_MODIFIER_NONE;
      LED_OFF[0] = 0; LED_OFF[1] = 0; LED_OFF[2] = 0;
      ee_Update_TempMode_Value(display_func);
    }
  }
}

void display_value(int16_t value)
{
  // просто отображаем температуру с датчика
  LED_DP[1] = 0;
  int16_t temp = value;
  LED_DP[1] = 0;
  //temp = temp * 10;
  //temp = temp / 16;
  //if (temp > 1000) // переходим в режим только целые градусы, если temp > 100 реальных
  //{
    //temp = temp / 10;
   // LED_DP[1] = 0;
  //}
  

  
       //GPIOD->ODR |= GPIO_PIN_4;
       //GPIOD->ODR |= GPIO_PIN_5;
      
  
  LED[0] = (uint8_t) (temp / 100);
  temp = temp - ((int16_t)LED[0] * 100);
  LED[1] = (uint8_t) (temp / 10);
  temp = temp - ((int16_t)LED[1] * 10);
  LED[2] = (uint8_t) temp;
  
}

static NO_REG_SAVE void display_thread_func (void)
{
  while(1)
  {
    if (display_modifier == DISPLAY_MODIFIER_BLINK)
    {

      if (display_submode == 0) // индикация работает
      {
        if (display_blink_cnt == 0)
        {
          display_blink_cnt = display_blink_value;
          display_submode = 1;
          memcpy(&LED_OFF, &display_blink_dig, 3);
        }
        display_blink_cnt--;
      }
      if (display_submode == 1) // индикатор погашен.
      {
        if (display_blink_cnt == 0)
        {
          display_blink_cnt = display_blink_value;
          display_submode = 0;
          uint8_t i;
          for (i = 0; i < 3; i++)
            if (display_blink_dig[i])
              LED_OFF[i] = 0;
        }
        display_blink_cnt--;
      }
    }
    if (display_mode == DISPLAY_MODE_TEMP)
    {
      
      if(ds_temp==0){
       display_value(000);
        LED_OFF[0]=1;LED_OFF[1]=1;LED_OFF[2]=1;
        krn_sleep(20);
        LED_OFF[0]=0;LED_OFF[1]=0;LED_OFF[2]=0;
        krn_sleep(20);
      }
       if(ds_temp>126){
       display_value(888);
        LED_OFF[0]=1;LED_OFF[1]=1;LED_OFF[2]=1;
        krn_sleep(20);
        LED_OFF[0]=0;LED_OFF[1]=0;LED_OFF[2]=0;
        krn_sleep(20);
      }
      else{
        LED_OFF[0]=0;LED_OFF[1]=0;LED_OFF[2]=0;
      display_value(ds_temp);
      }
      
    }
    else if (display_mode == DISPLAY_MODE_TEMP_SET)
    {
      display_value(display_temp);
    }
    else
    {
      // пока дополнительный режим один - показ номера установочного режима
      LED[0] = setup_mode;
    }
    

    krn_sleep(1);
  }
}

void  wait_sec()
{
  uint32_t sec = sys_sec;
  while (sec == sys_sec);
}


int main( void )
{
  hardware_init();
  ee_CheckDefaultValues();

  __enable_interrupt();
  

  //LED[0] = 0;
  //LED[1] = 0;
  //LED[2] = 0;
    

  krn_tmp_stack();
  krn_thread_init();

  krn_thread_create(&thr_temp, (void*)temp_thread_func, (void*)1, 1, temp_thread_stack, MAIN_THREAD_STACK);
  krn_thread_create(&thr_display, (void*)display_thread_func, (void*)2, 2, display_thread_stack, MAIN_THREAD_STACK);
  krn_thread_create(&thr_button, (void*)button_thread_func, (void*)3, 3, button_thread_stack, MAIN_THREAD_STACK);
  krn_thread_create(&thr_user, (void*)user_thread_func, (void*)4, 4, user_thread_stack, MAIN_THREAD_STACK);
  //krn_mutex_init(&mutex_printf);
  //krn_timer_init();
  
  krn_run();

}
