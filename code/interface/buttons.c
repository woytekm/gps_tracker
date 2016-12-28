#include "tracker_includes.h"
#include "tracker_global.h"
#include "tracker_routines.h"
#include "tracker_platform.h"

static void button_handler(uint32_t event_pins_low_to_high, uint32_t event_pins_high_to_low)
{

    GPS_settings_t GPS_settings;
    G_last_key = NOKEY;

    nrf_gpio_pin_write(LCD_BKL,1);
    G_backlight_timeout = BACKLIGHT_TIMEOUT;

    if(event_pins_high_to_low & (1 << KEY4))
     G_last_key = K4;
    else if(event_pins_high_to_low & (1 << KEY3))
     G_last_key = K3;
    else if(event_pins_high_to_low & (1 << KEY2))
     G_last_key = K2;
    else if(event_pins_high_to_low & (1 << KEY1))
     G_last_key = K1;

    //if(G_screen_mutex = true)
    // return;

    switch(G_last_key)
        {
            case K1:
#ifdef SEGGER_RTT_DEBUG
                SEGGER_RTT_printf(0, "RTT DEBUG: key1\n");
#endif
                if((G_screen_mode == STOP_GPS_STOP) ||  (G_screen_mode == STOP_GPS_RUN))
                 {
                  if(G_GPS_module_state == GPS_OFF)
                   {
                    nrf_gpio_pin_write(GPS_SWITCH,1);
                    G_GPS_module_state = GPS_ON;
                    nrf_delay_ms(1000);
                    GPS_get_settings(&GPS_settings);
                    GPS_setup(&GPS_settings);
                   }
                  G_logger_state = LOGGER_RUN;
                  G_screen_mode = RUN;
                 }
                else if((G_screen_mode == RUN) || (G_screen_mode == PAUSE))
                 {
                  G_logger_state = LOGGER_STOP;
                  G_screen_mode = STOP_GPS_RUN;
                 }
                else if(G_screen_mode == LOCKED)
                 {
                  if(G_keypad_lock_status == KEYPAD_UNLOCKED_0)
                    G_keypad_lock_status = KEYPAD_UNLOCKED_25;
                 }
                break;
            case K2:
#ifdef SEGGER_RTT_DEBUG
                SEGGER_RTT_printf(0, "RTT DEBUG: key2\n");
#endif
                if((G_screen_mode == STOP_GPS_STOP) ||  (G_screen_mode == STOP_GPS_RUN))
                  {
                   if(G_GPS_module_state == GPS_ON)
                   {
                     nrf_gpio_pin_write(GPS_SWITCH,0);
                     G_GPS_module_state = GPS_OFF;
                     G_screen_mode = STOP_GPS_STOP;
                     G_GPS_navMode = 0;
                   }
                  else if(G_GPS_module_state == GPS_OFF)
                   {
                     nrf_gpio_pin_write(GPS_SWITCH,1);
                     G_GPS_module_state = GPS_ON;
                     G_screen_mode = STOP_GPS_RUN;
                     nrf_delay_ms(1000);
                     GPS_get_settings(&GPS_settings);
                     GPS_setup(&GPS_settings);
                   }
                 }
                else if(G_screen_mode == RUN)
                 {
                   G_screen_mode = PAUSE;
                   G_logger_state = LOGGER_PAUSE;
                   gpx_write_pause_file(G_current_gpx_filename, G_current_track_distance, G_current_track_hours, G_current_track_mins);
                 }
                else if(G_screen_mode == PAUSE)
                 {
                   G_screen_mode = RUN;
                   G_logger_state = LOGGER_RUN;
                   gpx_remove_pause_file();
                 }
                else  if(G_screen_mode == LOCKED)
                  {
                    if(G_keypad_lock_status == KEYPAD_UNLOCKED_25)
                    G_keypad_lock_status = KEYPAD_UNLOCKED_50;
                  }

                break;
            case K3:
#ifdef SEGGER_RTT_DEBUG
                SEGGER_RTT_printf(0, "RTT DEBUG: key3\n");
#endif
                if(G_screen_mode == LOCKED)
                 {
                   if(G_keypad_lock_status == KEYPAD_UNLOCKED_75)
                    {
                     G_keypad_unlock_timer = 0;
                     G_keypad_lock_status = KEYPAD_UNLOCKED_100;
                     G_screen_mode = G_screen_mode_save;
                    }
                 }
                break;
            case K4:
#ifdef SEGGER_RTT_DEBUG
                SEGGER_RTT_printf(0, "RTT DEBUG: key4\n");
#endif
                if((G_screen_mode == STOP_GPS_STOP) ||  (G_screen_mode == STOP_GPS_RUN) ||
                   (G_screen_mode == RUN) ||  (G_screen_mode == PAUSE))
                 {
                   G_screen_mode_save = G_screen_mode;
                   G_screen_mode = LOCKED;
                   G_keypad_lock_status = KEYPAD_UNLOCKED_0;
                 }
                else if(G_screen_mode == LOCKED)
                 { 
                  if(G_keypad_lock_status == KEYPAD_UNLOCKED_50)
                   G_keypad_lock_status = KEYPAD_UNLOCKED_75;
                 }
                break;
            default:
                break;
        }

  update_LCD();

}

void buttons_init(void)
{

   #define APP_GPIOTE_MAX_USERS  2

   APP_GPIOTE_INIT(APP_GPIOTE_MAX_USERS);
   app_gpiote_user_register(&m_app_gpiote_my_id,(1<<KEY1)|(1<<KEY2)|(1<<KEY3)|(1<<KEY4),(1<<KEY1)|(1<<KEY2)|(1<<KEY3)|(1<<KEY4), button_handler);
   //app_gpiote_user_register(&m_app_gpiote_my_id,0xFFFFFFFF,0xFFFFFFFF,button_handler);
   app_gpiote_user_enable(m_app_gpiote_my_id);
   NVIC_SetPriority(GPIOTE_IRQn, 3);
   NVIC_EnableIRQ(GPIOTE_IRQn);
}
