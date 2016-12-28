#include "tracker_platform.h"
#include "tracker_routines.h"
#include "tracker_includes.h"
#include "tracker_global.h"

typedef enum
{
    UART_OFF,        /**< app_uart state OFF, indicating CTS is low. */
    UART_READY,      /**< app_uart state ON, indicating CTS is high. */
    UART_ON,         /**< app_uart state TX, indicating UART is ongoing transmitting data. */
    UART_WAIT_CLOSE, /**< app_uart state WAIT CLOSE, indicating that CTS is low, but a byte is currently being transmitted on the line. */
} app_uart_states_t;

/** @brief State transition events for the app_uart state machine. */
typedef enum
{
    ON_CTS_HIGH,   /**< Event: CTS gone high. */
    ON_CTS_LOW,    /**< Event: CTS gone low. */
    ON_UART_PUT,   /**< Event: Application wants to transmit data. */
    ON_TX_READY,   /**< Event: Data has been transmitted on the uart and line is available. */
    ON_UART_CLOSE, /**< Event: The UART module are being stopped. */
} app_uart_state_event_t;

static uint8_t  m_tx_byte;                /**< TX Byte placeholder for next byte to transmit. */

static volatile app_uart_states_t m_current_state = UART_OFF; /**< State of the state machine. */

void parse_UART_input(char *line_data)
 {

#ifdef SEGGER_RTT_DEBUG    
  if(line_data[0] != 0xB5)
    SEGGER_RTT_printf(0, "RTT DEBUG: UART input: %s\n",line_data);  // print it if we can assume that it's a text line 
                                                                    // if first byte is 0xB% - this is binary UBX message
#endif

   if(G_UART_wiring == UART_TO_GPS)  // parse GPS module input
    {
     parse_GPS_input(line_data);
    }

  if(G_UART_wiring == UART_TO_GPRS) // parse GPRS module input
    {
    }

 }

 void UART_config(  uint8_t rts_pin_number,
                           uint8_t txd_pin_number,
                           uint8_t cts_pin_number,
                           uint8_t rxd_pin_number,
                           uint32_t speed,
                           bool hwfc)
 {
   nrf_gpio_cfg_output(txd_pin_number);
   nrf_gpio_cfg_input(rxd_pin_number, NRF_GPIO_PIN_NOPULL);

   NRF_UART0->PSELTXD = txd_pin_number;
   NRF_UART0->PSELRXD = rxd_pin_number;

   if (hwfc)
   {
     nrf_gpio_cfg_output(rts_pin_number);
     nrf_gpio_cfg_input(cts_pin_number, NRF_GPIO_PIN_NOPULL);
     NRF_UART0->PSELCTS = cts_pin_number;
     NRF_UART0->PSELRTS = rts_pin_number;
     NRF_UART0->CONFIG  = (UART_CONFIG_HWFC_Enabled << UART_CONFIG_HWFC_Pos);
   }

   NRF_UART0->BAUDRATE         = (speed << UART_BAUDRATE_BAUDRATE_Pos);
   NRF_UART0->ENABLE           = (UART_ENABLE_ENABLE_Enabled << UART_ENABLE_ENABLE_Pos);
   NRF_UART0->TASKS_STARTTX    = 1;
   NRF_UART0->TASKS_STARTRX    = 1;
   NRF_UART0->EVENTS_RXDRDY    = 0;

   m_current_state         = UART_READY; 

   NRF_UART0->INTENCLR = 0xffffffffUL;
   NRF_UART0->INTENSET = (UART_INTENSET_RXDRDY_Set << UART_INTENSET_RXDRDY_Pos) |
                         (UART_INTENSET_TXDRDY_Set << UART_INTENSET_TXDRDY_Pos) |
                         (UART_INTENSET_ERROR_Set << UART_INTENSET_ERROR_Pos);

   NVIC_ClearPendingIRQ(UART0_IRQn);
   NVIC_SetPriority(UART0_IRQn, 1);
   NVIC_EnableIRQ(UART0_IRQn);

 }

static void action_tx_send()
{
    if (m_current_state != UART_ON)
    {
        // Start the UART.
        NRF_UART0->TASKS_STARTTX = 1;
    }

    //SEGGER_RTT_printf(0, "RTT DEBUG: serial out: %X.\n",m_tx_byte);

    NRF_UART0->TXD  = m_tx_byte;
    m_current_state = UART_ON;
}

void action_tx_stop()
{
    //app_uart_evt_t app_uart_event;

    // No more bytes in FIFO, terminate transmission.
    NRF_UART0->TASKS_STOPTX = 1;
    m_current_state         = UART_READY;
    // Last byte from FIFO transmitted, notify the application.
    // Notify that new data is available if this was first byte put in the buffer.
    //app_uart_event.evt_type = APP_UART_TX_EMPTY;
}

static void action_tx_ready()
 {
     action_tx_stop();
 }

 static void on_uart_put(void)
 {
     if (m_current_state == UART_READY)
     {
         action_tx_send();
     }
 }

static void on_tx_ready(void)
 {
     switch (m_current_state)
     {
         case UART_ON:
         case UART_READY:
             action_tx_ready();
             break;

         default:
             // Nothing to do.
             break;
     }
 }


static void on_uart_event(app_uart_state_event_t event)
 {
     switch (event)
     {

         case ON_TX_READY:
             on_tx_ready();
             break;

         case ON_UART_PUT:
             on_uart_put();
             break;

         default:
             // All valid events are handled above.
             break;
     }
 }

uint32_t app_uart_put(uint8_t byte)
  {
     uint32_t err_code = NRF_SUCCESS;
     uint16_t delay_counter = 0;

     while(m_current_state != UART_READY)
      {
       nrf_delay_ms(1);
       delay_counter++;
       if(delay_counter > 300)
        return 4;
      }

     m_tx_byte = byte;
     on_uart_event(ON_UART_PUT);

     //SEGGER_RTT_printf(0, "RTT DEBUG: app_uart_put: %X, err: err_code: %d\n", byte, err_code);

     return err_code;
  }

 void UART0_IRQHandler(void)
   {
     uint8_t rx_byte;
 
    // Handle reception
     if ((NRF_UART0->EVENTS_RXDRDY != 0) && (NRF_UART0->INTENSET & UART_INTENSET_RXDRDY_Msk))
      {
       //app_uart_evt_t app_uart_event;

       // Clear UART RX event flag
       NRF_UART0->EVENTS_RXDRDY  = 0;
       rx_byte                 = (uint8_t)NRF_UART0->RXD;

       //app_uart_event.evt_type   = APP_UART_DATA;
       //app_uart_event.data.value = m_rx_byte;
       //m_event_handler(&app_uart_event);

       G_UART_input_buffer[G_UART_buffer_pos] = rx_byte;

       if((G_UART_buffer_pos == 0) && (rx_byte == 0xB5)) // receiving UBX packet
        {
         G_receiving_UBX = true;
         G_received_UBX_packet_len = 0;
        }
       else if((G_UART_buffer_pos == 0) && (rx_byte != 0xB5))
        {
         G_receiving_UBX = false;
         G_received_UBX_packet_len = 0;
        }

       if(G_receiving_UBX && (G_UART_buffer_pos == 4))
        G_received_UBX_packet_len = G_UART_input_buffer[G_UART_buffer_pos];
       if(G_receiving_UBX && (G_UART_buffer_pos == 5))
        G_received_UBX_packet_len |= G_UART_input_buffer[G_UART_buffer_pos] << 8;

       G_UART_buffer_pos++;

       if(G_UART_buffer_pos > (UART_RX_BUF_SIZE - 1))
         G_UART_buffer_pos = 0;  // overflow, wrap buffer

       if((G_UART_input_buffer[G_UART_buffer_pos - 1] == '\n') && (!G_receiving_UBX))  // new line of data from UART
        {
          memcpy(&G_UART_new_data,&G_UART_input_buffer,G_UART_buffer_pos);
          G_UART_new_data[G_UART_buffer_pos - 1] = 0x0;
          parse_UART_input((char *)&G_UART_new_data);
          G_UART_buffer_pos = 0;
        }
       else if(G_receiving_UBX && (G_UART_buffer_pos == (G_received_UBX_packet_len + 8)))
       {
         memcpy(&G_UART_new_data,&G_UART_input_buffer,G_UART_buffer_pos);
         parse_UART_input((char *)&G_UART_new_data);
         G_UART_buffer_pos = 0;
       }

      }

     if ((NRF_UART0->EVENTS_ERROR != 0) && (NRF_UART0->INTENSET & UART_INTENSET_ERROR_Msk))
      {
        uint32_t       error_source;
        //app_uart_evt_t app_uart_event;

        // Clear UART ERROR event flag.
        NRF_UART0->EVENTS_ERROR = 0;

        // Clear error source.
        error_source        = NRF_UART0->ERRORSRC;
        NRF_UART0->ERRORSRC = error_source;

       //app_uart_event.evt_type                 = APP_UART_COMMUNICATION_ERROR;
       //app_uart_event.data.error_communication = error_source;
       //m_event_handler(&app_uart_event);
        nrf_gpio_pin_write(ERR_LED_PIN,1);
        nrf_delay_ms(20);
        nrf_gpio_pin_write(ERR_LED_PIN,0);

      }

     // Handle transmission.
     if ((NRF_UART0->EVENTS_TXDRDY != 0) && (NRF_UART0->INTENSET & UART_INTENSET_TXDRDY_Msk))
     {
         // Clear UART TX event flag.
         NRF_UART0->EVENTS_TXDRDY = 0;
         on_uart_event(ON_TX_READY);
     }


  }

