#include "tracker_global.h"
#include "tracker_includes.h"
#include "tracker_routines.h"
#include "tracker_platform.h"

void logger_init(void)
{
  G_logger_setup = LOG_TO_USD;
  G_logger_state = LOGGER_STOP;
  G_logger_interval = DEFAULT_LOGGER_INTERVAL;
  G_logger_usd_update_interval = 60;
  G_logger_gprs_update_interval = 0;
  G_logger_buffer_idx = 0;
  G_logger_interval_counter = 0;
  G_logger_usd_update_counter = 0;
}

void err_led_blink(void)
{
  nrf_gpio_pin_write(ERR_LED_PIN,1);
  nrf_delay_ms(200);
  nrf_gpio_pin_write(ERR_LED_PIN,0);
}


void logger(void)
{

 uint8_t i;
 double cur_long, cur_lat, prev_long, prev_lat;

 G_logger_interval_counter++;
 G_logger_usd_update_counter++;

 if((G_logger_state == LOGGER_RUN)&&(!G_gpx_wrote_header)) // new track
  {
   #ifdef SEGGER_RTT_DEBUG
   SEGGER_RTT_printf(0, "RTT DEBUG: track start - writing new gpx file\n");
   #endif

   G_usd_mount_status = mount_sd();
   if(G_usd_mount_status > 0)
    {
       err_led_blink();
       error_dialog(" SD CARD ERR ");
       G_logger_state = LOGGER_STOP;
       G_screen_mode = STOP_GPS_RUN;
       return;
     }

   G_current_gpx_filename = malloc(30);
   sprintf(G_current_gpx_filename,"%d-%02d-%02d@%02d-%02d-%02d.gpx",G_system_time.tm_year+2000,G_system_time.tm_mon,G_system_time.tm_mday,
                                                                    G_system_time.tm_hour,G_system_time.tm_min,G_system_time.tm_sec);
   //f_unlink(G_current_gpx_filename);
   //sprintf(G_current_gpx_filename,"newtrack.gpx");
   if(!gpx_write_header(G_current_gpx_filename,"Test track"))
    err_led_blink();

   G_gpx_wrote_header = true;
   G_gpx_wrote_footer = false;
  }
 else if((G_logger_state == LOGGER_STOP)&&(!G_gpx_wrote_footer)) // new track
  {
   #ifdef SEGGER_RTT_DEBUG
   SEGGER_RTT_printf(0, "RTT DEBUG: track stop - writing gpx footer\n");
   #endif
   if(!gpx_write_footer(G_current_gpx_filename))
    err_led_blink();
   free(G_current_gpx_filename);
   G_gpx_wrote_header = false;
   G_gpx_wrote_footer = true;
   G_current_track_distance = 0;
   G_logged_positions = 0;
   G_current_track_hours = 0;
   G_current_track_mins = 0;
   G_current_track_sec = 0;
  }

 // if position is of low quality - do not log it (this should be optional)
 if(((G_logger_interval_counter >= G_logger_interval)&&(G_logger_state == LOGGER_RUN)) && 
     (G_current_position.n_satellites > MIN_SATELLITES) && (G_current_position.HDOP < MIN_V_HDOP)) 
  {

   G_logged_positions++;

   if(G_logged_positions == 1)
    {
     memcpy(&G_prev_position,&G_current_position,sizeof(nmea_gpgga_t));
    }
   else
    {
     cur_long = G_current_position.m_longitude.degrees + (G_current_position.m_longitude.minutes/60);
     cur_lat = G_current_position.m_latitude.degrees + (G_current_position.m_latitude.minutes/60);
     prev_long = G_prev_position.m_longitude.degrees + (G_prev_position.m_longitude.minutes/60); 
     prev_lat = G_prev_position.m_latitude.degrees + (G_prev_position.m_latitude.minutes/60);
     G_current_track_distance +=  coord_distance(cur_lat, cur_long, prev_lat, prev_long, 'K');
     memcpy(&G_prev_position,&G_current_position,sizeof(nmea_gpgga_t));
    }
   

   memcpy((void *)&G_logger_buffer[G_logger_buffer_idx++],(void *)&G_current_position,sizeof(nmea_gpgga_t));
   G_logger_interval_counter = 0;
#ifdef SEGGER_RTT_DEBUG
   SEGGER_RTT_printf(0, "RTT DEBUG: logger: appending current position to buffer: %s, %s\n",G_logger_buffer[G_logger_buffer_idx-1].nmea_longitude, 
                                                                                            G_logger_buffer[G_logger_buffer_idx-1].nmea_latitude);
#endif

   if(G_current_speed > 4)
    G_logger_interval = FAST_LOGGER_INTERVAL;
   else
    G_logger_interval = DEFAULT_LOGGER_INTERVAL;

  }

 if(G_logger_usd_update_counter == G_logger_usd_update_interval)
  {

   for(i = 0; i < G_logger_buffer_idx; i++)
    if(!gpx_append_position(&G_logger_buffer[i], G_current_gpx_filename))
     err_led_blink();

   G_logger_buffer_idx = 0;
   G_logger_usd_update_counter = 0;
  }
 
}

