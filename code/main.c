/** 
 * GPS Tracker application for GPS Tracker project.
 * (c)2016 woytekm
 *
 */

#include "tracker_platform.h"
#include "tracker_routines.h"
#include "tracker_includes.h"
#include "tracker_global.h"

FRESULT mount_sd(void)
{
        uint8_t temp;
        for(int i=0; i<5; i++)
        {
                temp = f_mount(&G_FatFs, "", 1);     /* Give a work area to the default drive */
                if(temp == 0)   break;
        }
        #ifdef SEGGER_RTT_DEBUG
        SEGGER_RTT_printf(0, "RTT DEBUG: usd mount status: %d\n",temp);
        #endif
        sd_spi_uninit();
        return temp;
}

static void nosd_power_manage(void)
 {
  __WFE();  
  __SEV();
  __WFE();
 }

void flash_leds()
 {
   nrf_gpio_pin_write(FIX_LED_PIN,1);
   nrf_gpio_pin_write(ERR_LED_PIN,1);
   nrf_delay_ms(300);
   nrf_gpio_pin_write(FIX_LED_PIN,0);
   nrf_gpio_pin_write(ERR_LED_PIN,0);
   nrf_delay_ms(300);
 }

void board_init()
 {
#ifdef SEGGER_RTT_DEBUG
  SEGGER_RTT_printf(0, "RTT DEBUG: initializing hardware:");
#endif
  nrf_gpio_cfg_output(FIX_LED_PIN);
  nrf_gpio_cfg_output(ERR_LED_PIN);
  nrf_gpio_cfg_output(BUZZER_SWITCH);
  nrf_gpio_cfg_output(GPS_SWITCH);
  nrf_gpio_cfg_output(LCD_BKL);
  nrf_gpio_cfg_input(USD_CARD,NRF_GPIO_PIN_NOPULL);
  nrf_gpio_cfg_input(KEY1,NRF_GPIO_PIN_PULLUP);
  nrf_gpio_cfg_input(KEY2,NRF_GPIO_PIN_PULLUP);
  nrf_gpio_cfg_input(KEY3,NRF_GPIO_PIN_PULLUP);
  nrf_gpio_cfg_input(KEY4,NRF_GPIO_PIN_PULLUP);

  nrf_gpio_cfg_sense_input(KEY1, NRF_GPIO_PIN_PULLUP, NRF_GPIO_PIN_SENSE_LOW);
  nrf_gpio_cfg_sense_input(KEY2, NRF_GPIO_PIN_PULLUP, NRF_GPIO_PIN_SENSE_LOW);
  nrf_gpio_cfg_sense_input(KEY3, NRF_GPIO_PIN_PULLUP, NRF_GPIO_PIN_SENSE_LOW);
  nrf_gpio_cfg_sense_input(KEY4, NRF_GPIO_PIN_PULLUP, NRF_GPIO_PIN_SENSE_LOW);

  G_GPS_module_state = GPS_OFF;

  LCDInit(60);
  LCDclear();

  memset(&G_system_time, 0, sizeof (struct tm));

  G_system_time.tm_mday = 1;
  G_system_time.tm_mon  = 1;
  G_system_time.tm_year    = 0;

  G_screen_mutex = false;
  G_SPI_mutex = false;
  G_gpx_wrote_header = false;
  G_gpx_wrote_footer = true;

  G_time_synced = false;
  G_date_synced = false;

  flash_leds();
#ifdef SEGGER_RTT_DEBUG
  SEGGER_RTT_printf(0, " done.\n");
#endif
 }

/**
 * @brief Function for application main entry.
 */
int main(void)
 {

#ifdef SEGGER_RTT_DEBUG
   SEGGER_RTT_printf(0, "RTT DEBUG: Tracker app startup.\n");
#endif

   clock_init();
   board_init();
   buttons_init();

   adc_config();
   nrf_adc_start();

   logger_init();

   G_screen_mode = STOP_GPS_STOP;

   UART_config(0,GPS_TX_PIN,0,GPS_RX_PIN,UART_BAUDRATE_BAUDRATE_Baud9600,false);
   G_UART_wiring = UART_TO_GPS;

   G_usd_mount_status = mount_sd();

   if(G_usd_mount_status > 0)
     err_led_blink();

   if(gpx_check_for_pause_file())
    if(!gpx_apply_pause_file())   // pause file exists but it seems to be corrupted - remove it
     gpx_remove_pause_file();
    
   timers_start();

   nrf_gpio_pin_write(LCD_BKL,1);
   G_backlight_timeout = BACKLIGHT_TIMEOUT;

   buzzer_signal(4,0);

   while(true)
    {
     nosd_power_manage();
    }

 }


/** @} */
