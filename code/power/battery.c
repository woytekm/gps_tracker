#include "tracker_includes.h"
#include "tracker_global.h"
#include "tracker_routines.h"
#include "tracker_platform.h"


void adc_config(void)
{
  const nrf_adc_config_t nrf_adc_config = NRF_ADC_CONFIG_DEFAULT;

  nrf_adc_configure((nrf_adc_config_t *)&nrf_adc_config);
  nrf_adc_input_select(NRF_ADC_CONFIG_INPUT_3);
  //nrf_adc_int_enable(ADC_INTENSET_END_Enabled << ADC_INTENSET_END_Pos);
  //NVIC_SetPriority(ADC_IRQn, NRF_APP_PRIORITY_HIGH);
  //NVIC_EnableIRQ(ADC_IRQn);
}

void adc_timer_handler(void)
{
 // with current ADC settings (2x2.2M ohm voltage divider, 100nF filtering cap, default ADC config):
 // 591 = 4.18V - battery fully charged
 // 555 = 3.92V - battery 75%
 // 519 =       - battery 50%
 // 483 =       - battery 25%
 // 447 =       - battery 0%
 // 432 = 3.03V  - battery empty
 //
 nrf_adc_conversion_event_clean();
 nrf_adc_start();
 nrf_delay_ms(2); 
 G_battery_level = nrf_adc_result_get();
#ifdef SEGGER_RTT_DEBUG
 SEGGER_RTT_printf(0, "RTT DEBUG: battery level: %d\n", G_battery_level);
#endif
}

void check_battery_level(void)
 {
   if((G_battery_level < 430) && (G_battery_level > 0))
    {
     if(G_logger_state == LOGGER_RUN)
      {
        G_screen_mode = PAUSE;
        G_logger_state = LOGGER_PAUSE;
        gpx_write_pause_file(G_current_gpx_filename, G_current_track_distance, G_current_track_hours, G_current_track_mins);
      }
     error_dialog(" BATTERY LOW ");
     buzzer_signal(18,0);
     LCDclear();
     nrf_gpio_pin_write(LCD_BKL,0);
     nrf_gpio_pin_write(GPS_SWITCH,0);
     nrf_gpio_pin_write(GPRS_SWITCH,0);
     NRF_POWER->SYSTEMOFF = POWER_SYSTEMOFF_SYSTEMOFF_Enter << POWER_SYSTEMOFF_SYSTEMOFF_Pos;
    }
 }
