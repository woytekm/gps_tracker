#include "tracker_includes.h"
#include "tracker_global.h"
#include "tracker_routines.h"
#include "tracker_platform.h"


#define FONT5x5 5
#define FONT5x7 7

void set_keymap_bar(char *key1, char *key2, char *key3, char *key4)
 {
   if(strlen(key1))
     LCDdrawstring(0,40,key1,TEXT_INVERTED,FONT5x5);
   if(strlen(key2))
     LCDdrawstring(22,40,key2,TEXT_INVERTED,FONT5x5);
   if(strlen(key3))
     LCDdrawstring(44,40,key3,TEXT_INVERTED,FONT5x5);
   if(strlen(key4))
     LCDdrawstring(65,40,key4,TEXT_INVERTED,FONT5x5);
 }

void update_LCD()
 {
   char line0[14];
   char line1[14];
   char line2[14];
   char line3[14];
   char line4a[9];
   char line4b[5];
   uint32_t minsec;
   char batt_ind;
   char state_ind;
   char h_quality;
   char sdcard;

   #ifdef SEGGER_RTT_DEBUG
   SEGGER_RTT_printf(0, "RTT DEBUG: update_LCD: screen_mutex:%d, screen_mode:%d, GPS_module_state:%d\n",G_screen_mutex,G_screen_mode,G_GPS_module_state);
   #endif

   if(G_screen_mutex)  
    return;

   LCDclear();

   switch (G_screen_mode) {

    case STOP_GPS_STOP:
      set_keymap_bar("NEW","GPS","OPT","LCK");
      break;
    case STOP_GPS_RUN:
      set_keymap_bar("NEW","STP","OPT","LCK");
      break;
    case RUN:
      set_keymap_bar("END","PSE","   ","LCK");
      break;
    case PAUSE:
      set_keymap_bar("END","RES","   ","LCK");
      break;
    case OPTIONS:
      set_keymap_bar(" UP","DWN","SET","ESC");
      break;
    case LOCKED:
      switch (G_keypad_lock_status) {
       case KEYPAD_UNLOCKED_0:
         set_keymap_bar(" 1 "," 2 "," 4 "," 3 ");
         break;
       case KEYPAD_UNLOCKED_25:
         set_keymap_bar("   "," 2 "," 4 "," 3 ");
         break;
       case KEYPAD_UNLOCKED_50:
         set_keymap_bar("   ","   "," 4 "," 3 ");
         break;
       case KEYPAD_UNLOCKED_75:
         set_keymap_bar("   ","   "," 4 ","   ");
         break;
      }

   }
  
   batt_ind = 0x0;

   if(G_battery_level >= 580)
    batt_ind = 0xFA;
   else if((G_battery_level < 580) && (G_battery_level >= 555))
    batt_ind = 0xFA;
   else if((G_battery_level < 555) && (G_battery_level >= 519))
    batt_ind = 0xF9;
   else if((G_battery_level < 519) && (G_battery_level >= 483))
    batt_ind = 0xF8;
   else if(G_battery_level < 483) 
    batt_ind = 0xF7;

   SEGGER_RTT_printf(0, "RTT DEBUG: bat_ind: %d\n",batt_ind);

   if(G_logger_state == LOGGER_RUN)
    state_ind = 0xf6;
   else if(G_logger_state == LOGGER_PAUSE)
    state_ind = 0xf4;
   else
    state_ind = 0xf5;

   if(G_current_position.HDOP < 3)
     h_quality='+';
   else
     h_quality=' ';

   if(nrf_gpio_pin_read(USD_CARD))
    sdcard = 0xf3;
   else
    sdcard = ' ';

   if(((G_GPS_navMode == 2) || (G_GPS_navMode == 3)) && (G_GPS_module_state == GPS_ON))
    {
      sprintf(line0," FIX %02d:%02d %c%c%c",G_system_time.tm_hour,G_system_time.tm_min,state_ind,sdcard,batt_ind);
      sprintf(line3,"ALT:%04dM %02dS",G_current_position.altitude,G_current_position.n_satellites);

      LCDdrawstring(0,1,line0,true,FONT5x5);

      minsec = (G_current_position.m_longitude.minutes/60)*10000000;
      sprintf(line1,"%d.%07d N%c",G_current_position.m_longitude.degrees,(int)minsec,h_quality);
 
      minsec = (G_current_position.m_latitude.minutes/60)*10000000;
      sprintf(line2,"%d.%07d W%c",G_current_position.m_latitude.degrees,(int)minsec,h_quality);
   
    }
   else
    {
      strcpy(line1,"             ");
      strcpy(line2,"             ");
      strcpy(line3,"             ");

      if(G_GPS_module_state == GPS_OFF)
       sprintf(line0,"!GPS %02d:%02d %c%c%c",G_system_time.tm_hour,G_system_time.tm_min,state_ind,sdcard,batt_ind);
      else
       sprintf(line0,"!FIX %02d:%02d %c%c%c",G_system_time.tm_hour,G_system_time.tm_min,state_ind,sdcard,batt_ind);
      LCDdrawstring(0,1,line0,true,FONT5x5);
    }

   LCDdrawstring(5,9,line1,false,FONT5x5);
   LCDdrawstring(5,16,line2,false,FONT5x5);
   LCDdrawstring(5,23,line3,false,FONT5x5);

   if((G_logger_state == LOGGER_RUN) || (G_logger_state == LOGGER_PAUSE))
    {
      sprintf(line4a,"%03.1fK ",G_current_track_distance);
      sprintf(line4b,"%02d:%02d",G_current_track_hours, G_current_track_mins);
      LCDdrawstring(5,30,line4a,false,FONT5x5);
      LCDdrawstring(54,30,line4b,false,FONT5x5);
    }

   LCDdisplay();

 }
