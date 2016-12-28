#include "tracker_platform.h"
#include "tracker_routines.h"
#include "tracker_includes.h"
#include "tracker_global.h"

bool gpx_check_for_pause_file()
 {
   FILINFO f_inf;

   while(G_SPI_mutex == true)
    {}
   G_SPI_mutex = true;
   sd_spi_init();
   if(f_stat("paused.dat",&f_inf) != FR_OK)
    {
     sd_spi_uninit();
     G_SPI_mutex = false;  
     return false;
    }

   sd_spi_uninit();
   G_SPI_mutex = false;
   return true;

 }

bool gpx_apply_pause_file()
 {
   GPS_settings_t GPS_settings;
   TCHAR pause_data[64];
   char *str = (char *)&pause_data;
   char gpx_filename[32];
   double gpx_distance;
   uint16_t gpx_hours;
   uint8_t  gpx_minutes;
   FIL file;

   while(G_SPI_mutex == true)
    {}
   G_SPI_mutex = true;
   sd_spi_init();

   if (f_open(&file, "paused.dat", FA_READ) == FR_OK)
    {
     f_gets((TCHAR *)&pause_data,64,&file);
     f_close(&file);
     sd_spi_uninit();
     G_SPI_mutex = false;

     strcpy(gpx_filename,strsep(&str," "));
     if(strlen(gpx_filename) < 5) return false;
     gpx_distance = atof(strsep(&str," "));
     if(gpx_distance == 0) return false;
     gpx_hours = atoi(strsep(&str," "));
     gpx_minutes = atoi(strsep(&str," "));
     if(gpx_minutes == 0) return false;

     G_current_gpx_filename = malloc(strlen(gpx_filename));
     strcpy(G_current_gpx_filename, gpx_filename);
     G_current_track_distance = gpx_distance;
     G_current_track_hours = gpx_hours;
     G_current_track_mins = gpx_minutes;

     G_logger_state = LOGGER_PAUSE;
     G_screen_mode = PAUSE;

     if(G_GPS_module_state == GPS_OFF)
      {
       nrf_gpio_pin_write(GPS_SWITCH,1);
       G_GPS_module_state = GPS_ON;
       nrf_delay_ms(1000);
       GPS_get_settings(&GPS_settings);
       GPS_setup(&GPS_settings);
      }

     return true;

    }
  sd_spi_uninit();
  G_SPI_mutex = false;
  return false;

 }

bool gpx_write_pause_file(char *gpx_filename, double track_len, uint8_t track_minutes, uint16_t track_hours)
 {
  char data[64];
  FIL file;
  UINT bw;
  uint16_t data_len;

  sprintf(data,"%s %f %d %d\n",gpx_filename, track_len, track_minutes, track_hours);

  while(G_SPI_mutex == true)
   {}
  G_SPI_mutex = true;
  sd_spi_init();
  if (f_open(&file, "paused.dat", FA_WRITE | FA_CREATE_NEW) == FR_OK)
   {
    data_len = strlen(data);
    f_write(&file, data, data_len, &bw);
    f_close(&file);
    sd_spi_uninit();
    G_SPI_mutex = false;
    if (bw != data_len)
     return false;
   }
  else
   {
    sd_spi_uninit();
    G_SPI_mutex = false;
    return false;
   }
  return true;
 }

bool gpx_remove_pause_file(void)
 {
   while(G_SPI_mutex == true)
    {}
   G_SPI_mutex = true;
   sd_spi_init();
   if( f_unlink("paused.dat") != 0)
    {
     sd_spi_uninit();
     G_SPI_mutex = false;  
     return false;
    }
   else
    {
     sd_spi_uninit();
     G_SPI_mutex = false;
     return true;
    }
 }


bool gpx_write_header(char *filename, char *track_name)
 {
     FIL file;
     UINT bw;
     uint16_t data_len;

     char gpx_header[512];
     char gpx_header_1[] = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<gpx version=\"1.0\" xmlns=\"http://www.topografix.com/GPX/1/1\"\nxmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"\nxsi:schemaLocation=\"http://www.topografix.com/GPX/1/1 http://www.topografix.com/GPX/1/1/gpx.xsd\">\n<trk><name>";
     char gpx_header_2[] = "</name>\n<trkseg>\n";

     bzero(gpx_header,512);

     while(G_SPI_mutex == true)
      {}
     G_SPI_mutex = true;

     sd_spi_init();
     if (f_open(&file, filename, FA_WRITE | FA_CREATE_NEW) == FR_OK)
      {
        //f_lseek(&file, f_size(&file));
        
        strcat(gpx_header,gpx_header_1);
        strcat(gpx_header,track_name);
        strcat(gpx_header,gpx_header_2);

        data_len = strlen(gpx_header);

        f_write(&file, gpx_header, data_len, &bw);     /* Write data to the file */
        f_close(&file);                                /* Close the file */
        sd_spi_uninit();
        G_SPI_mutex = false;
        if (bw != data_len)
          return false;
       }
     G_SPI_mutex = false;
     sd_spi_uninit();
     return true;
 }

bool gpx_write_footer(char *filename)
 {
    FIL file;
    sd_spi_init();
    UINT bw;
    uint16_t data_len;
    char gpx_footer[] ="</trkseg></trk></gpx>";

    while(G_SPI_mutex == true)
     {}
    G_SPI_mutex = true;
    sd_spi_init();

    if (f_open(&file, filename, FA_WRITE | FA_OPEN_APPEND) == FR_OK)
     {
       data_len = strlen(gpx_footer);
       f_write(&file, gpx_footer, data_len, &bw);     /* Write data to the file */
       f_close(&file);                                /* Close the file */
       sd_spi_uninit();
       G_SPI_mutex = false;
       if (bw != data_len)
          return false;
      }
    G_SPI_mutex = false;
    sd_spi_uninit();
    return true;
 }


bool gpx_append_position(nmea_gpgga_t *position, char *filename)
 {
    FIL file;
    sd_spi_init();
    UINT bw;
    uint16_t data_len;
    uint32_t minsec_long;
    uint32_t minsec_lat;
    char gpx_trkpt[128];
    struct tm *timeptr;
    degmin_position_t *degmin_long,*degmin_lat;
    
    degmin_long = &position->m_longitude;
    degmin_lat = &position->m_latitude;
    timeptr = &position->time;

    minsec_long = (degmin_long->minutes/60)*10000000;
    minsec_lat = (degmin_lat->minutes/60)*10000000;
    sprintf(gpx_trkpt,"<trkpt lat=\"%d.%07d\" lon=\"%d.%07d\"><ele>%d</ele><time>%d-%02d-%02dT%02d:%02d:%02dZ</time></trkpt>\n",degmin_lat->degrees,(int)minsec_lat,degmin_long->degrees,
                                                                                                                               (int)minsec_long,position->altitude,
                                                                                                                               G_GPS_year+2000,G_GPS_month,G_GPS_day,
                                                                                                                               timeptr->tm_hour,timeptr->tm_min,timeptr->tm_sec);
    while(G_SPI_mutex == true)
     {}
    G_SPI_mutex = true;
    sd_spi_init();

    if (f_open(&file, filename, FA_WRITE | FA_OPEN_APPEND) == FR_OK)
     {
       data_len = strlen(gpx_trkpt);
       f_write(&file, gpx_trkpt, data_len, &bw);     /* Write data to the file */
       f_close(&file);                               /* Close the file */
       sd_spi_uninit();
       G_SPI_mutex = false;
       if (bw != data_len)
          return false;
      }
    sd_spi_uninit();
    G_SPI_mutex = false;
    return true;
 }



