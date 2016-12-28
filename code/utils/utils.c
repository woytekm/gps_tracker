#include "tracker_includes.h"
#include "tracker_global.h"
#include "tracker_routines.h"
#include "tracker_platform.h"

uint16_t htobe16( uint16_t x ) 
 {
  uint8_t xs[ 2 ] = {
  (uint8_t)( ( x >> 8 ) & 0xFF ),
  (uint8_t)( ( x      ) & 0xFF ) };
  union {
    const uint8_t  *p8;
    const uint16_t *p16;
   } u;
  u.p8 = xs;
  return *u.p16;
 }

bool yesno_dialog(char *message)
 {
  bool answer;

  G_screen_mutex = true;

  LCDclear();
  set_keymap_bar("YES","   ","   ","NO "); 
  LCDdrawstring(0,1,"  QUESTION    ", true, 5); 
  LCDdrawstring(0,15,message, false, 5); 
  LCDdisplay();

  G_last_key = NOKEY;

  while((G_last_key != K1)||(G_last_key != K4))
   nrf_delay_ms(100); 

  if(G_last_key == K1) answer = true;
  else if(G_last_key == K4) answer = false;

  G_screen_mutex = false;

  return answer;
 }

void error_dialog(char *message)
 {
   G_screen_mutex = true;

   LCDclear();
   set_keymap_bar("   ","   ","   ","   ");
   LCDdrawstring(0,1,"    ERROR     ", true, 5);
   LCDdrawstring(0,15,message, false, 5);
   LCDdisplay();

   nrf_delay_ms(1000);

   G_screen_mutex = false;
 }

void buzzer_signal(uint8_t s1_len, uint8_t s2_len)
 {
   uint8_t i = 0;

   while(i < s1_len)
    {
      nrf_gpio_pin_write(BUZZER_SWITCH,1);
      nrf_delay_ms(10);
      nrf_gpio_pin_write(BUZZER_SWITCH,0);
      i++;
    }

   nrf_delay_ms(300);
   i = 0;

   while(i < s2_len)
     {
       nrf_gpio_pin_write(BUZZER_SWITCH,1);
       nrf_delay_ms(10);
       nrf_gpio_pin_write(BUZZER_SWITCH,0);
       i++;
     }
 }


