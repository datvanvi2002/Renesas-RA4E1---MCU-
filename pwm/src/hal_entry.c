#include "hal_data.h"
#include "r_ioport.h"
#include "r_agt.h"

/* Define the LED pin (example: P1_0) */
bsp_io_level_t readLevel;
//fsp_err_t status = 0;

FSP_CPP_HEADER
void R_BSP_WarmStart(bsp_warm_start_event_t event);
FSP_CPP_FOOTER

#define GPT_EXAMPLE_MSEC_PER_SEC           (1000)
float GPT_EXAMPLE_DESIRED_PERIOD_MSEC = 0.01;
#define GPT_EXAMPLE_DESIRED_DUTY_CYCLE_PERCENT    (50)
#define GPT_EXAMPLE_MAX_PERCENT                   (100)

fsp_err_t err = FSP_SUCCESS;
void hal_entry(void)
{
    /* Initialize the I/O port module */
    R_IOPORT_Open (&g_ioport_ctrl, &g_bsp_pin_cfg);

    /* Main loop */
    /* Initializes the module. */
    err = R_GPT_Open (&g_timer1_ctrl, &g_timer1_cfg);
    /* Handle any errors. This function should be defined by the user. */
    assert(FSP_SUCCESS == err);
    /* Start the timer. */
    (void) R_GPT_Start (&g_timer1_ctrl);
    int frequency = 1;
    float desired_frequency = 1.0;
    float desired_period_msec;
    while (1)
    { /* Calculate the desired period in milliseconds */
        desired_period_msec = 1000.0 / desired_frequency;

        uint32_t pclkd_freq_hz = R_FSP_SystemClockHzGet (FSP_PRIV_CLOCK_PCLKD) >> g_timer1_cfg.source_div;
        uint32_t period_counts =
                (uint32_t) (((uint64_t) pclkd_freq_hz * desired_period_msec) / GPT_EXAMPLE_MSEC_PER_SEC);

        /* Set the calculated period */
        err = R_GPT_PeriodSet (&g_timer1_ctrl, period_counts);
        assert(FSP_SUCCESS == err);

        /* Get the current period setting */
        timer_info_t info;
        (void) R_GPT_InfoGet (&g_timer1_ctrl, &info);
        uint32_t current_period_counts = info.period_counts;

        /* Calculate and set the duty cycle */
        uint32_t duty_cycle_counts = (uint32_t) (((uint64_t) current_period_counts
                * GPT_EXAMPLE_DESIRED_DUTY_CYCLE_PERCENT) / GPT_EXAMPLE_MAX_PERCENT);
        err = R_GPT_DutyCycleSet (&g_timer1_ctrl, duty_cycle_counts, GPT_IO_PIN_GTIOCB);
        assert(FSP_SUCCESS == err);

        /* Increase the frequency by 2Hz */
        desired_frequency += 2.0;

        /* Reset the frequency to 1Hz after reaching 10kHz */
        if (desired_frequency > 10000.0)
        {
            desired_frequency = 1.0;
        }

        /* Delay for 0.1 second */
        R_BSP_SoftwareDelay (100, BSP_DELAY_UNITS_MILLISECONDS);

    }

#if BSP_TZ_SECURE_BUILD
    /* Enter non-secure code */
    R_BSP_NonSecureEnter();
#endif
}

void R_BSP_WarmStart(bsp_warm_start_event_t event)
{
    if (BSP_WARM_START_RESET == event)
    {
#if BSP_FEATURE_FLASH_LP_VERSION != 0

        /* Enable reading from data flash. */
        R_FACI_LP->DFLCTL = 1U;

        /* Would normally have to wait tDSTOP(6us) for data flash recovery. Placing the enable here, before clock and
         * C runtime initialization, should negate the need for a delay since the initialization will typically take more than 6us. */
#endif
    }

    if (BSP_WARM_START_POST_C == event)
    {
        /* C runtime environment and system clocks are setup. */

        /* Configure pins. */
        R_IOPORT_Open (&IOPORT_CFG_CTRL, &IOPORT_CFG_NAME);

#if BSP_CFG_SDRAM_ENABLED

        /* Setup SDRAM and initialize it. Must configure pins first. */
        R_BSP_SdramInit(true);
#endif
    }
}

#if BSP_TZ_SECURE_BUILD

FSP_CPP_HEADER
BSP_CMSE_NONSECURE_ENTRY void template_nonsecure_callable();

/* Trustzone Secure Projects require at least one nonsecure callable function in order to build (Remove this if it is not required to build). */
BSP_CMSE_NONSECURE_ENTRY void template_nonsecure_callable()
{

}
FSP_CPP_FOOTER

#endif
