#include "tracker_global.h"
#include "tracker_includes.h"
#include "tracker_routines.h"
#include "tracker_platform.h"

void GPS_LED_handler(void)
 {
   static uint8_t cnt;
   if((G_GPS_navMode == 2) || (G_GPS_navMode == 3))
    cnt++;
   else
    cnt = 0;

   if(cnt == 0)
    nrf_gpio_pin_write(FIX_LED_PIN,0);
   else if(cnt > 50)
     {
      cnt = 1;
      nrf_gpio_pin_write(FIX_LED_PIN,0);
     }
   else if(cnt == 48)
     {
      nrf_gpio_pin_write(FIX_LED_PIN,1);
     }
 }

void general_housekeeping(void)
{
 #ifdef SEGGER_RTT_DEBUG
 SEGGER_RTT_printf(0, "RTT DEBUG: housekeeping\n");
 #endif

 if((G_screen_mode == LOCKED) && (G_keypad_lock_status != KEYPAD_UNLOCKED_0))
  G_keypad_unlock_timer++;

 if(G_keypad_unlock_timer > 5)
  {
    G_keypad_lock_status = KEYPAD_UNLOCKED_0;
    G_keypad_unlock_timer = 0;
  }

 if(G_backlight_timeout > 0)
  {
    G_backlight_timeout--;
    if(G_backlight_timeout == 0)
     {nrf_gpio_pin_write(LCD_BKL,0);}
  }
  
 logger();
 check_battery_level();

}


void sync_system_time_to_GPS(void)
 {
  G_system_time.tm_sec = G_current_position.time.tm_sec;
  G_system_time.tm_min = G_current_position.time.tm_min;
  G_system_time.tm_hour = G_current_position.time.tm_hour;
 }

void sync_system_date_to_GPS(void)
 {
  G_system_time.tm_year = G_GPS_year;
  G_system_time.tm_mon = G_GPS_month;
  G_system_time.tm_mday = G_GPS_day;
 }

void start_timer_1(void)
{	
  APP_TIMER_DEF(timer_id1);	
  #define TIMER1_TICK APP_TIMER_TICKS(50, 0)

  app_timer_create(&timer_id1, APP_TIMER_MODE_REPEATED, (app_timer_timeout_handler_t)GPS_LED_handler);
  app_timer_start(timer_id1, TIMER1_TICK, NULL);
}


void start_timer_2(void)
{
  APP_TIMER_DEF(timer_id2);
  #define TIMER2_TICK APP_TIMER_TICKS(5000, 0)

  app_timer_create(&timer_id2, APP_TIMER_MODE_REPEATED, (app_timer_timeout_handler_t)adc_timer_handler);
  app_timer_start(timer_id2, TIMER2_TICK, NULL);
}


void start_timer_3(void)
{
   APP_TIMER_DEF(timer_id3);
   #define TIMER3_TICK APP_TIMER_TICKS(1500, 0)

   app_timer_create(&timer_id3, APP_TIMER_MODE_REPEATED, (app_timer_timeout_handler_t)update_LCD);
   app_timer_start(timer_id3, TIMER3_TICK, NULL);
}

void start_timer_4(void)
 {
    APP_TIMER_DEF(timer_id4);
    #define TIMER4_TICK APP_TIMER_TICKS(1200, 0)

    app_timer_create(&timer_id4, APP_TIMER_MODE_REPEATED, (app_timer_timeout_handler_t)general_housekeeping);
    app_timer_start(timer_id4, TIMER4_TICK, NULL);
 }

void RTC0_IRQHandler(void)
{
  // NORDIC: CLEAR TASK AS QUICKLY AS POSSIBLE
  NRF_RTC0->TASKS_CLEAR = 1;
  // This handler will be run after wakeup from system ON (RTC wakeup)
  if(NRF_RTC0->EVENTS_COMPARE[0])
   {
     NRF_RTC0->EVENTS_COMPARE[0] = 0;

     #ifdef SEGGER_RTT_DEBUG
     SEGGER_RTT_printf(0, "RTC interrupt: %02d:%02d:%02d\n",G_system_time.tm_hour,G_system_time.tm_min,G_system_time.tm_sec);
     #endif

     if(G_system_time.tm_sec >= 59)
      {
       G_system_time.tm_min++;
       G_system_time.tm_sec = 0;
       if(G_system_time.tm_min == 59)
        {
         G_system_time.tm_hour++;
         G_system_time.tm_min = 0;
         if(G_system_time.tm_hour == 23)
           {
            G_system_time.tm_hour = 0;
            G_system_time.tm_mday++;
           }
        }
       }

     G_system_time.tm_sec++;   

     if(G_logger_state == LOGGER_RUN)
      {
       if(G_current_track_mins == 59)
        {
         G_current_track_hours++;
         G_current_track_mins = 0;
        }
       G_current_track_sec++;
       if(G_current_track_sec == 59)
        {
         G_current_track_mins++;
         G_current_track_sec = 0;
        }
       }
   }
}


void clock_init()
{
    NRF_CLOCK->LFCLKSRC            = (CLOCK_LFCLKSRC_SRC_Xtal << CLOCK_LFCLKSRC_SRC_Pos);
    NRF_CLOCK->EVENTS_LFCLKSTARTED = 0;
    NRF_CLOCK->TASKS_LFCLKSTART    = 1;
    while (NRF_CLOCK->EVENTS_LFCLKSTARTED == 0); // Wait for clock to start

    NRF_RTC0->PRESCALER = 0;
    NRF_RTC0->EVTENSET = RTC_EVTEN_COMPARE0_Msk; 
    NRF_RTC0->INTENSET = RTC_INTENSET_COMPARE0_Msk; 
    // NORDIC: Count to 32767, and not 32768
    NRF_RTC0->CC[0] = 1*32767;

    NVIC_EnableIRQ(RTC0_IRQn);
    // NORDIC: SET IRQ PRIORITY
    NVIC_SetPriority(RTC0_IRQn, 0);

    NRF_RTC0->TASKS_START = 1;
}	
		
void timers_start(void)
{
  nrf_drv_clock_init();
  nrf_drv_clock_lfclk_request(NULL);

  #define APP_TIMER_PRESCALER 0

  APP_TIMER_INIT(APP_TIMER_PRESCALER, 4, NULL);

  start_timer_1();
  start_timer_2();
  start_timer_3();
  start_timer_4();
}


