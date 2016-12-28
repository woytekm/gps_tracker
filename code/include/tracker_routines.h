#include "tracker_includes.h"

// serial module
void parse_UART_input(char *line_data);
void UART0_IRQHandler(void);
void UART_config(uint8_t rts_pin_number,
                          uint8_t txd_pin_number,
                          uint8_t cts_pin_number,
                          uint8_t rxd_pin_number,
                          uint32_t speed,
                          bool hwfc);

// GPS module
void GPS_parse_GPGGA(char *GPGGA_msg);
void GPS_parse_GPGSA(char *GPGSA_msg);
void parse_GPS_input(char *input_string);
void GPS_query_setup();
//void GPS_get_settings(GPS_settings_t *GPS_settings);
//void GPS_setup(GPS_settings_t *GPS_settings);
double coord_distance(double lat1, double lon1, double lat2, double lon2, char unit);

// UBX porotocol routines
void fletcherChecksum(unsigned char* buffer, int size, unsigned char* checkSumA, unsigned char* checkSumB);
UBXMsgBuffer createBuffer(int payloadSize);
extern void clearUBXMsgBuffer(const UBXMsgBuffer* buffer);
void completeMsg(UBXMsgBuffer* buffer, int payloadSize);
void initMsg(UBXMsg* msg, int payloadSize, UBXMessageClass msgClass, UBXMessageId msgId);
UBXMsgBuffer getCFG_RXM(UBXU1_t lpMode);
UBXMsgBuffer getCFG_RXM_POLL();
UBXMsgBuffer getCFG_PM2_POLL();
UBXMsgBuffer getCFG_PM2(UBXCFG_PM2Flags flags, 
                        UBXU4_t updatePeriod, 
                        UBXU4_t searchPeriod, 
                        UBXU4_t gridOffset, 
                        UBXU2_t onTime, 
                        UBXU2_t minAcqTime);

// interface
void buttons_init(void);

// display
void LCDdisplay(void);
void LCDdrawstring(uint8_t x, uint8_t y, char *c, uint8_t is_inverted, uint8_t font);
void LCDdrawchar5x5(uint8_t x, uint8_t y, char c, uint8_t inverted);
void LCDdrawchar5x7(uint8_t x, uint8_t y, char c, uint8_t inverted);
void LCDInit(uint8_t contrast);
void LCDclear(void);
void update_LCD(void);
void set_keymap_bar(char *key1, char *key2, char *key3, char *key4);

// utils
uint16_t htobe16(uint16_t x);
void timers_start(void);
void app_timer1_handler(void);
void sync_system_time_to_GPS(void);
void sync_system_date_to_GPS(void);
void clock_init(void);
bool yesno_dialog(char *message);
void error_dialog(char *message);
void err_led_blink(void);
FRESULT mount_sd(void);
void buzzer_signal(uint8_t s1_len, uint8_t s2_len);

// power
void adc_config(void);
void adc_timer_handler(void);
void check_battery_level();

// logger
void logger(void);
void logger_init(void);
bool gpx_write_header(char *filename, char *track_name);
bool gpx_write_footer(char *filename);
bool gpx_write_pause_file(char *gpx_filename, double track_len, uint8_t track_minutes, uint16_t track_hours);
bool gpx_remove_pause_file(void);
bool gpx_check_for_pause_file();
bool gpx_apply_pause_file();

