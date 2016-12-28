#include "tracker_includes.h"
#include "tracker_routines.h"
#include "tracker_error.h"
#include "tracker_global.h"

uint8_t GPS_send_UBX_stream(char *stream, uint16_t length)
 {
  uint16_t i;
  uint8_t UBX_response_timeout = 0;

  G_last_UBX_ACK_state = UBX_UNKNOWN;

  if(G_UART_wiring == UART_TO_GPS)
   {

    for(i = 0; i < length; i++)
     {
       app_uart_put(stream[i]);
     }

    G_GPS_cmd_sent = true;

    while((G_last_UBX_ACK_state == UBX_UNKNOWN) && (UBX_response_timeout < 100))
     {
      UBX_response_timeout++;
      nrf_delay_ms(10);
     }

    if(G_last_UBX_ACK_state != UBX_UNKNOWN)
      return G_last_UBX_ACK_state;
    else return TR_UBX_NAK_TIMEOUT;

   }
  else
   return TR_ERR_GPS_NOTCONN;

 }

void GPS_setup(GPS_settings_t *GPS_settings)
 {
  UBXMsgBuffer cmd_buffer;
#ifdef SEGGER_RTT_DEBUG
  uint8_t UBX_response;
#endif

  cmd_buffer = getCFG_RXM(GPS_settings->lp_mode);
#ifdef SEGGER_RTT_DEBUG
  UBX_response = GPS_send_UBX_stream(cmd_buffer.data,cmd_buffer.size);
#else
  GPS_send_UBX_stream(cmd_buffer.data,cmd_buffer.size);
#endif
  
  #ifdef SEGGER_RTT_DEBUG
  SEGGER_RTT_printf(0, "RTT DEBUG: received UBX response: %d\n",UBX_response);
  #endif

  clearUBXMsgBuffer(&cmd_buffer); 


  //cmd_buffer = getCFG_PM2(GPS_settings->pm_flags, GPS_settings->pm_update_period, GPS_settings->pm_search_period, 
  //                        GPS_settings->pm_grid_offset, GPS_settings->pm_on_time, GPS_settings->pm_min_acq_time);
  //G_last_UBX_ACK_state = UBX_UNKNOWN;
  //GPS_send_byte_stream(cmd_buffer.data,cmd_buffer.size);

  //while(G_last_UBX_ACK_state != UBX_UNKNOWN)
  // nrf_delay_ms(10);

  //clearUBXMsgBuffer(&cmd_buffer);


 }

void GPS_query_setup()
 {
  UBXMsgBuffer cmd_buffer;

  cmd_buffer = getCFG_RXM_POLL();
  G_last_UBX_ACK_state = UBX_UNKNOWN;
  GPS_send_UBX_stream(cmd_buffer.data,cmd_buffer.size);
  clearUBXMsgBuffer(&cmd_buffer);

  nrf_delay_ms(100);

  cmd_buffer = getCFG_PM2_POLL();
  G_last_UBX_ACK_state = UBX_UNKNOWN;
  GPS_send_UBX_stream(cmd_buffer.data,cmd_buffer.size);
  clearUBXMsgBuffer(&cmd_buffer);

 }

void GPS_get_settings(GPS_settings_t *settings)
{
  settings->lp_mode = 0; // low power
  //settings->pm_flags;
  //settings->pm_update_period;
  //settings->pm_search_period;
  //settings->pm_grid_offset;
  //settings->m_on_time;
  //settings->m_min_acq_time;
 }
