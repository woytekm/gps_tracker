#include "tracker_includes.h"
#include "tracker_platform.h"
#include "tracker_routines.h"
#include "tracker_global.h"


/*
* distance calculation from: http://www.geodatasource.com/developers/c
*/

#define pi 3.14159265358979323846
	
double deg2rad(double deg) {
  return (deg * pi / 180);
}
double rad2deg(double rad) {
  return (rad * 180 / pi);
}

double coord_distance(double lat1, double lon1, double lat2, double lon2, char unit) {
  double theta, dist;
  theta = lon1 - lon2;
  dist = sin(deg2rad(lat1)) * sin(deg2rad(lat2)) + cos(deg2rad(lat1)) * cos(deg2rad(lat2)) * cos(deg2rad(theta));
  dist = acos(dist);
  dist = rad2deg(dist);
  dist = dist * 60 * 1.1515;
  switch(unit) {
    case 'M':
      break;
    case 'K':
      dist = dist * 1.609344;
      break;
    case 'N':
      dist = dist * 0.8684;
      break;
  }
  return (dist);
}


/*
* following routines come from libnmea (https://github.com/jacketizer/libnmea/blob/master/src/parsers/parse.c)
*/

int
nmea_position_parse(char *s, degmin_position_t *pos)
{
  char *cursor;

  pos->degrees = 0;
  pos->minutes = 0;

  if (s == NULL || *s == '\0') {
	return -1;
   }

  /* decimal minutes */
  if (NULL == (cursor = strchr(s, '.'))) {
	return -1;
   }

  /* minutes starts 2 digits before dot */
  cursor -= 2;
  pos->minutes = atof(cursor);
  *cursor = '\0';

  /* integer degrees */
  cursor = s;
  pos->degrees = atoi(cursor);

  return 0;
}

int
nmea_time_parse(char *s, struct tm *time)
{
    
   char hour[3];
   char min[3];
   char sec[3];

   if (s == NULL || *s == '\0' || (strlen(s) < 6) ) {
	return -1;
    }

  memset(time, 0, sizeof (struct tm));

  strncpy(hour,s,2);
  strncpy(min,s+2,2);
  strncpy(sec,s+4,2);
  sec[2] = 0x0;

  time->tm_hour = atoi(hour);
  time->tm_min = atoi(min);
  time->tm_sec = atoi(sec);
  time->tm_year = G_GPS_year;
  time->tm_mon = G_GPS_month;
  time->tm_mday = G_GPS_day;

  return 0;
}

int nmea_date_parse(char *s)
 {

   char day[3];
   char month[3];
   char year[3];

   if (s == NULL || *s == '\0' || (strlen(s) < 6) ) {
        return -1;
    }

  strncpy(day,s,2);
  strncpy(month,s+2,2);
  strncpy(year,s+4,2);

  G_GPS_day = atoi(day);
  G_GPS_month = atoi(month);
  G_GPS_year = atoi(year);

  return 0;
 }


/*
* libnmea parts end
*/


void GPS_parse_GPGGA(char *GPGGA_msg)
 {
   #ifdef SEGGER_RTT_DEBUG
   char debug[256];
   #endif
   char *delimiter = ",";
   char *strsep_helper;

   if(G_GPS_navMode > 1) // GPS fix is present
    {
     if(G_fixes < 12)
       G_fixes++;
     strsep(&GPGGA_msg, delimiter);   // field: $GPGGA
     nmea_time_parse(strsep(&GPGGA_msg, delimiter),&G_current_position.time);         // field: receiver time
     strsep_helper = strsep(&GPGGA_msg, delimiter);
     strcpy(G_current_position.nmea_latitude,strsep_helper);
     nmea_position_parse(strsep_helper,&G_current_position.m_latitude);      // field: GPS latitude
     strsep(&GPGGA_msg, delimiter);                // field: N
     strsep_helper = strsep(&GPGGA_msg, delimiter);
     strcpy(G_current_position.nmea_longitude,strsep_helper);
     nmea_position_parse(strsep_helper,&G_current_position.m_longitude);     // field: GPS longitude
     strsep(&GPGGA_msg, delimiter);                // field: E
     strsep(&GPGGA_msg, delimiter);                // field: location quality
     G_current_position.n_satellites = atoi(strsep(&GPGGA_msg, delimiter));                // field: number of satellites used
     G_current_position.HDOP = atof(strsep(&GPGGA_msg, delimiter));                // field: horizontal dilution of precision
     G_current_position.altitude = atoi(strsep(&GPGGA_msg, delimiter));      // field: GPS altitude above sea level

#ifdef SEGGER_RTT_DEBUG
     sprintf(debug,"RTT DEBUG: GPS location:  TIME, %d %f N, %d %f E, %dm ALT, NMEA: %s, %s, HDOP: %f, speed: %f\n", G_current_position.m_longitude.degrees, 
                                                                                  G_current_position.m_longitude.minutes,
                                                                                  G_current_position.m_latitude.degrees, 
                                                                                  G_current_position.m_latitude.minutes,
                                                                                  G_current_position.altitude,
                                                                                  G_current_position.nmea_latitude,
                                                                                  G_current_position.nmea_longitude,
                                                                                  G_current_position.HDOP,
                                                                                  G_current_speed
                                                                                  );
     SEGGER_RTT_printf(0,debug);
#endif
     if((!G_time_synced) && (G_fixes > 10))  // sync time after 5 good GPS fixes
      {
        sync_system_time_to_GPS();
        G_time_synced = true;
      }
    }
 }

void GPS_parse_GPRMC(char *GPRMC_msg)
 {
  char *delimiter = ",";

  strsep(&GPRMC_msg, delimiter); // field: $GPRMC
  strsep(&GPRMC_msg,delimiter);      // field: time
  strsep(&GPRMC_msg,delimiter);      // field: status
  strsep(&GPRMC_msg,delimiter);      // field: lat
  strsep(&GPRMC_msg,delimiter);      // field: N/S indicator
  strsep(&GPRMC_msg,delimiter);      // field: long
  strsep(&GPRMC_msg,delimiter);      // field: E/W indicator
  G_current_speed = atof(strsep(&GPRMC_msg,delimiter)); // current speed in knots
  strsep(&GPRMC_msg,delimiter);      // course over ground (unused)
  nmea_date_parse(strsep(&GPRMC_msg,delimiter));

  if((!G_date_synced) && (G_fixes > 10))
   {
    sync_system_date_to_GPS();
    G_date_synced = true;
   }
 }

void GPS_parse_GPGSA(char *GPGSA_msg)
 {
   char *delimiter = ",";

   uint8_t navMode, opMode;

   strsep(&GPGSA_msg, delimiter);     // field: $GPGSA
   opMode = (atoi(strsep(&GPGSA_msg,delimiter)));  // field: operation mode
   navMode = (atoi(strsep(&GPGSA_msg,delimiter))); // field: navigation mode

   if(G_GPS_opMode != opMode)
    {}

   if(G_GPS_navMode != navMode)
      G_GPS_navMode = navMode;

 }

void parse_GPS_input(char *input_string)
 {
   char *wrk_str;
   char *token;
   char *delimiter = ",";
   uint16_t UBX_len;
   #ifdef SEGGER_RTT_DEBUG
   uint8_t i;
   #endif

   wrk_str = strdup(input_string);

   token = strtok(wrk_str, delimiter);

   if( (strcmp(token,"$GPGGA")) == 0)
    {
      GPS_parse_GPGGA(input_string);
    }

   if( (strcmp(token,"$GPGSA")) == 0)
    {
      GPS_parse_GPGSA(input_string);
    }

   if( (strcmp(token,"$GPRMC")) == 0)
    {
     GPS_parse_GPRMC(input_string);
    }

   if((input_string[0] == 0xB5) && (input_string[1] == 0x62))  // this is UBX message
    {

     UBX_len = input_string[4];
     UBX_len |= input_string[5] << 8;
     UBX_len += 8; // count header and trailer in

     if((input_string[2] == 0x05) && (input_string[3] == 0x01))
      {
#ifdef SEGGER_RTT_DEBUG
       SEGGER_RTT_printf(0, "RTT DEBUG: UBX ACK-ACK\n");
#endif
       G_last_UBX_ACK_state = UBX_ACK;
      }
     else if((input_string[2] == 0x05) && (input_string[3] == 0x00))
      {
#ifdef SEGGER_RTT_DEBUG
        SEGGER_RTT_printf(0, "RTT DEBUG: UBX ACK-NAK\n");
#endif
        G_last_UBX_ACK_state = UBX_NAK;
      }
     else if(G_GPS_cmd_sent)  
      {
#ifdef SEGGER_RTT_DEBUG
       i = 0; 
       SEGGER_RTT_printf(0, "RTT DEBUG: UBX reply [ ");
       while(i < UBX_len)
        SEGGER_RTT_printf(0, "%X ",input_string[i++]);
       SEGGER_RTT_printf(0, " ] len: %d\n", UBX_len);
#endif

       if(G_last_received_UBX_msg.data != NULL)
        free(G_last_received_UBX_msg.data);

       G_last_received_UBX_msg.size = UBX_len;
       G_last_received_UBX_msg.data = malloc(UBX_len);
       memcpy(G_last_received_UBX_msg.data,input_string,UBX_len);

       G_GPS_cmd_sent = false;
      }
#ifdef SEGGER_RTT_DEBUG
     else
      {
        SEGGER_RTT_printf(0, "RTT DEBUG: UBX data without previous command [");
        i = 0;
        while(i < UBX_len)
         SEGGER_RTT_printf(0, "%X ",input_string[i++]);
        SEGGER_RTT_printf(0, " ] len: %d\n", UBX_len);
      }
#endif
    }

   free(wrk_str);

 }


