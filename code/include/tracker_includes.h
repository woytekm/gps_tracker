#define __USE_XOPEN

#include "ff.h"
#include "diskio.h"
#include "sdk_config.h"
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include "nrf_delay.h"
#include "nrf_gpio.h"
#include "nrf_adc.h"
#include "app_uart.h"
#include "app_util.h"
#include "app_error.h"
#include "app_timer.h"
#include "app_gpiote.h"
#include "SEGGER_RTT.h"
#include "ubxmessage.h"
#include <nrf_drv_gpiote.h>
#include <nrf_drv_clock.h>
#include <nrf_drv_spi.h>

