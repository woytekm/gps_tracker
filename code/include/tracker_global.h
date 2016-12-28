/*
*
* GPS Tracker
* Global data structures
*
*/

/* SDK setup follows */

#define SPI_ENABLED 1

#define APP_GPIOTE_ENABLED 1

#ifndef CLOCK_ENABLED
#define CLOCK_ENABLED 1
#endif
#if  CLOCK_ENABLED
#ifndef CLOCK_CONFIG_XTAL_FREQ
#define CLOCK_CONFIG_XTAL_FREQ 255
#endif
#ifndef CLOCK_CONFIG_LF_SRC
#define CLOCK_CONFIG_LF_SRC 1
#endif
#ifndef CLOCK_CONFIG_IRQ_PRIORITY
#define CLOCK_CONFIG_IRQ_PRIORITY 3
#endif
#endif //CLOCK_ENABLED

#define APP_GPIOTE_ENABLED 1

#define APP_TIMER_ENABLED 1

#if  APP_TIMER_ENABLED
#ifndef APP_TIMER_WITH_PROFILER
#define APP_TIMER_WITH_PROFILER 0
#endif
#ifndef APP_TIMER_KEEPS_RTC_ACTIVE
#define APP_TIMER_KEEPS_RTC_ACTIVE 0
#endif
#endif //APP_TIMER_ENABLED

#define UART_ENABLED 1
#if  UART_ENABLED
#ifndef UART_DEFAULT_CONFIG_HWFC
#define UART_DEFAULT_CONFIG_HWFC 0
#endif
#ifndef UART_DEFAULT_CONFIG_PARITY
#define UART_DEFAULT_CONFIG_PARITY 0
#endif
#ifndef UART_DEFAULT_CONFIG_BAUDRATE
#define UART_DEFAULT_CONFIG_BAUDRATE 2576384  // this is 9600bps
#endif
#ifndef UART_DEFAULT_CONFIG_IRQ_PRIORITY
#define UART_DEFAULT_CONFIG_IRQ_PRIORITY 1
#endif
#ifndef UART0_CONFIG_USE_EASY_DMA
#define UART0_CONFIG_USE_EASY_DMA 1
#endif
#ifndef UART_EASY_DMA_SUPPORT
#define UART_EASY_DMA_SUPPORT 1
#endif
#ifndef UART_LEGACY_SUPPORT
#define UART_LEGACY_SUPPORT 1
#endif
#endif //UART_ENABLED

/* SDK setup end */

#include "tracker_includes.h"

#define UART_TX_BUF_SIZE 256
#define UART_RX_BUF_SIZE 1024
#define TEXT_INVERTED 1

// global tracker settings

#define TRACKER_SILENT          0
#define DAYLIGHT_SAVING         1
#define BACKLIGHT_TIMEOUT       10
#define DEFAULT_LOGGER_INTERVAL 6
#define FAST_LOGGER_INTERVAL    3
#define MIN_SATELLITES          4
#define MIN_V_HDOP              4

app_gpiote_user_id_t m_app_gpiote_my_id;

enum UART_WIRING { UART_TO_GPS, UART_TO_GPRS };
enum UBX_ACK_STATE { UBX_ACK, UBX_NAK, UBX_UNKNOWN };
enum SCREENS { STOP_GPS_STOP, STOP_GPS_RUN, PAUSE, RUN, OPTIONS, LOCKED };
enum GPS_STATE { GPS_ON, GPS_OFF };
enum KEYPRESS { K1, K2, K3, K4, NOKEY};
enum LOGGER_STATE { LOGGER_STOP, LOGGER_RUN, LOGGER_PAUSE };
enum LOGGER_SETUP { LOG_TO_USD, LOG_TO_GPRS };

uint8_t G_UART_input_buffer[UART_RX_BUF_SIZE];
char G_UART_new_data[256];  // single UART input command max length
uint16_t G_UART_buffer_pos;

uint8_t G_GPS_module_state;

uint8_t G_UART_wiring;

uint8_t G_keypad_unlock_timer;

#define KEYPAD_UNLOCKED_100  0
#define KEYPAD_UNLOCKED_75   1
#define KEYPAD_UNLOCKED_50   2
#define KEYPAD_UNLOCKED_25   3
#define KEYPAD_UNLOCKED_0    4

#define MAX_LOGGER_BUFFER 100

FATFS G_FatFs;

uint8_t G_usd_mount_status;
uint8_t G_logger_setup;
uint8_t G_logger_state;
uint8_t G_logger_interval;
uint8_t G_logger_usd_update_interval;
uint8_t G_logger_gprs_update_interval;
uint8_t G_logger_interval_counter;
uint8_t G_logger_usd_update_counter;

uint8_t G_screen_mode_save;
uint8_t G_keypad_lock_status;
uint8_t G_last_key;
bool G_screen_lock;
bool G_screen_mutex;
bool G_SPI_mutex;
uint8_t G_screen_mode;
bool G_GPS_cmd_sent;
bool G_receiving_UBX;
uint8_t G_last_UBX_ACK_state;
UBXMsgBuffer G_last_received_UBX_msg;
uint16_t G_received_UBX_packet_len;

uint8_t G_backlight_timeout;

uint16_t G_logged_positions;
uint8_t G_fixes;
bool G_time_synced;
bool G_date_synced;
struct tm G_system_time;

uint8_t G_GPS_year;
uint8_t G_GPS_month;
uint8_t G_GPS_day;

double G_current_speed;
double G_current_track_distance;
uint16_t G_current_track_hours;
uint8_t G_current_track_mins;
uint8_t G_current_track_sec;

uint8_t G_GPS_opMode;
uint8_t G_GPS_navMode;

uint32_t G_battery_level;

#define NMEA_TIME_FORMAT        "%H%M%S"
#define NMEA_TIME_FORMAT_LEN    6

#define NMEA_DATE_FORMAT        "%d%m%y"
#define NMEA_DATE_FORMAT_LEN    6

 typedef struct {
           double minutes;
           int degrees;
  } degmin_position_t;

  typedef struct {
          struct tm time;
          degmin_position_t m_longitude;
          degmin_position_t m_latitude;
          char nmea_longitude[12];
          char nmea_latitude[12];
          double HDOP;
          int n_satellites;
          int altitude;
          char altitude_unit;
 } nmea_gpgga_t;

nmea_gpgga_t G_current_position;
nmea_gpgga_t G_prev_position;

nmea_gpgga_t G_logger_buffer[MAX_LOGGER_BUFFER];
uint8_t G_logger_buffer_idx;

char *G_current_gpx_filename;
bool G_gpx_wrote_header;
bool G_gpx_wrote_footer;

typedef struct GPS_settings{
  UBXU1_t lp_mode;
  UBXCFG_PM2Flags pm_flags;
  UBXU4_t pm_update_period;
  UBXU4_t pm_search_period;
  UBXU4_t pm_grid_offset;
  UBXU2_t pm_on_time; 
  UBXU2_t pm_min_acq_time;
 } GPS_settings_t;

void GPS_get_settings(GPS_settings_t *GPS_settings);
void GPS_setup(GPS_settings_t *GPS_settings);
bool gpx_append_position(nmea_gpgga_t *position, char *filename);

