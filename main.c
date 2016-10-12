#include <stdio.h>
#include "boards.h"
#include "app_util_platform.h"
#include "nrf_drv_rtc.h"
#include "nrf_drv_clock.h"
#include "nrf_delay.h"
#include "bsp.h"
#include "app_error.h"
#include "app_timer.h"
#include "app_twi.h"
#include "si7021.h"

#include "compiler_abstraction.h"
#define NRF_LOG_MODULE_NAME "APP"
#include "nrf_log.h"
#include "nrf_log_ctrl.h"


#define MAX_PENDING_TRANSACTIONS    10

#define APP_TIMER_PRESCALER         0
#define APP_TIMER_OP_QUEUE_SIZE     2

static app_twi_t m_app_twi = APP_TWI_INSTANCE(0);

static nrf_drv_rtc_t const m_rtc = NRF_DRV_RTC_INSTANCE(0);

////////////////////////////////////////////////////////////////////////////////
// Buttons handling (by means of BSP).
//
static void bsp_event_handler(bsp_event_t event)
{
    switch (event)
    {
    case BSP_EVENT_KEY_0: // Button 1 pushed.
        LEDS_INVERT(BSP_LED_0_MASK);
        break;

    case BSP_EVENT_KEY_1: // Button 2 pushed.
        LEDS_INVERT(BSP_LED_1_MASK);
        break;

    default:
        break;
    }
}

static void bsp_config(void)
{
    uint32_t err_code;

    APP_TIMER_INIT(APP_TIMER_PRESCALER, APP_TIMER_OP_QUEUE_SIZE, NULL);

    err_code = bsp_init(BSP_INIT_BUTTONS,
                        APP_TIMER_TICKS(100, APP_TIMER_PRESCALER),
                        bsp_event_handler);
    APP_ERROR_CHECK(err_code);
}

// TWI (with transaction manager) initialization.
static void twi_config(void)
{
    uint32_t err_code;

    nrf_drv_twi_config_t const config = {
       .scl                = ARDUINO_SCL_PIN,
       .sda                = ARDUINO_SDA_PIN,
       .frequency          = NRF_TWI_FREQ_100K,
       .interrupt_priority = APP_IRQ_PRIORITY_HIGH,
       .clear_bus_init     = false
    };

    APP_TWI_INIT(&m_app_twi, &config, MAX_PENDING_TRANSACTIONS, err_code);
    APP_ERROR_CHECK(err_code);
}

static void read_all_cb(ret_code_t result, void * p_user_data)
{
    LEDS_OFF(BSP_LED_4_MASK);
    if (result != NRF_SUCCESS)
    {
        NRF_LOG_INFO("read_all_registers_cb - error: %d\r\n", (int)result);
        return;
    }

    int16_t temp = si7021getTemperature100();
    int16_t rh = si7021getHumidity100();

    NRF_LOG_INFO("Temp*100: %d\r\n", temp);
    NRF_LOG_INFO("Humi*100: %d\r\n", rh);
}


// RTC tick events generation.
static void rtc_handler(nrf_drv_rtc_int_type_t int_type)
{
    if (int_type == NRF_DRV_RTC_INT_TICK)
    {
        LEDS_INVERT(BSP_LED_2_MASK);

        // On each RTC tick (their frequency is set in "nrf_drv_config.h")
        // we read data from our sensor.

        // send measure request
        static app_twi_transaction_t const transaction =
        {
            .callback            = read_all_cb,
            .p_user_data         = NULL,
            .p_transfers         = SI7021_READ_ALL,
            .number_of_transfers = sizeof(SI7021_READ_ALL) / sizeof(SI7021_READ_ALL[0])
        };

        LEDS_ON(BSP_LED_4_MASK);
        APP_ERROR_CHECK(app_twi_schedule(&m_app_twi, &transaction));                
    }
}

static void rtc_config(void)
{
    uint32_t err_code;

    // Initialize RTC instance with default configuration.
    nrf_drv_rtc_config_t config = NRF_DRV_RTC_DEFAULT_CONFIG;
    config.prescaler = RTC_FREQ_TO_PRESCALER(10); //Set RTC frequency to 10Hz
    err_code = nrf_drv_rtc_init(&m_rtc, &config, rtc_handler);
    APP_ERROR_CHECK(err_code);

    // Enable tick event and interrupt.
    nrf_drv_rtc_tick_enable(&m_rtc, true);

    // Power on RTC instance.
    nrf_drv_rtc_enable(&m_rtc);
}


static void lfclk_config(void)
{
    uint32_t err_code;

    err_code = nrf_drv_clock_init();
    APP_ERROR_CHECK(err_code);

    nrf_drv_clock_lfclk_request(NULL);
}


int main(void)
{
    LEDS_CONFIGURE(LEDS_MASK);
  
    // Start internal LFCLK XTAL oscillator - it is needed by BSP to handle
    // buttons with the use of APP_TIMER and for ticks generation
    // (by RTC).
    lfclk_config();

    bsp_config();

    APP_ERROR_CHECK(NRF_LOG_INIT(NULL));

    NRF_LOG_INFO("si7021 example\r\n");
    NRF_LOG_FLUSH();

    twi_config();

    // initialize si7021
    APP_ERROR_CHECK(app_twi_perform(&m_app_twi, SI7021_RESET, 1, NULL));

    nrf_delay_ms(100);

    // read config
    APP_ERROR_CHECK(app_twi_perform(&m_app_twi, SI7021_CONFIG, 2, NULL));

    rtc_config();

    while (true)
    {
        __WFI();
        NRF_LOG_FLUSH();
    }
}


/** @} */
