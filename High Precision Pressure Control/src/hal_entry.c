#include "hal_data.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h> // Thêm assert.h

FSP_CPP_HEADER
void R_BSP_WarmStart(bsp_warm_start_event_t event);
FSP_CPP_FOOTER

fsp_err_t err = FSP_SUCCESS;
//uart
#define UART_BUFFER_SIZE  128
char g_uart_rx_buffer[UART_BUFFER_SIZE];
volatile bool g_uart_receive_complete = false;
volatile bool g_uart_transmit_complete = false;
//uart callback : xác định data đã gửi/nhận xong chưa
void uart_callback(uart_callback_args_t *p_args);
void uart_callback(uart_callback_args_t *p_args)
{
    if (p_args->event == UART_EVENT_RX_COMPLETE)
    {
        g_uart_receive_complete = true;
    }
    else if (p_args->event == UART_EVENT_TX_COMPLETE)
    {
        g_uart_transmit_complete = true;
    }
}

//timer: đếm thời gian từ lúc cấp nguồn trả về thời gian là milli giây với biến g_millis;
volatile uint32_t g_millis = 0;
void agt_callback(timer_callback_args_t *p_args);
void agt_callback(timer_callback_args_t *p_args)
{
    if (p_args->event == TIMER_EVENT_CYCLE_END)
    {
        g_millis++;
    }
}

//pressure: get data from pressure sensor, wika cpt6100
float lastpressure;
float pressure = 0;

#define EPSILON 0.00001
int frequency = 1;

float desired_frequency = 1.0;
float desired_period_msec;
#define PI 3.14159265358979323846
#define SIN_MAX_VALUE 30
//uint32_t simple_rand(int f);
//uint32_t simple_rand(int f)
//{
//    desired_period_msec = 1000.0 / f;
////    desired_frequency += 2.0;
////
////    /* Reset the frequency to 1Hz after reaching 10kHz */
////    if (desired_frequency > 10000.0)
////    {
////        desired_frequency = 1.0;
////    }
//
//    /* Delay for 0.1 second */
//    R_BSP_SoftwareDelay (10, BSP_DELAY_UNITS_MILLISECONDS);
//
//}
void callPressure(void);
void callPressure(void)
{
//    const char *callslave = "#*?\n";
//    R_SCI_UART_Write (&g_uart0_ctrl, (uint8_t*) callslave, strlen (callslave));

//    char message[UART_BUFFER_SIZE];
//    snprintf (message, UART_BUFFER_SIZE, "#*?\n");
//    g_uart_transmit_complete = false;
//    R_SCI_UART_Write (&g_uart1_ctrl, (uint8_t*) message, strlen (message));
//
//    g_uart_receive_complete = false;
//    R_SCI_UART_Read (&g_uart1_ctrl, (uint8_t*) g_uart_rx_buffer, UART_BUFFER_SIZE);
//    char datapressure[10] =
//    { 0 };
//    strncpy (datapressure, g_uart_rx_buffer + 4, 6);
//    datapressure[6] = '\0';
//    //pressure = strtof (datapressure, NULL);  // Convert string to float// Ensure null-terminated string
//    pressure = atof (datapressure);
    //random pressure
    //pressure = (((g_millis % 30 ) *3 + 54)/2)*1.21435524;
    pressure = pressure + 0.05;
    if (pressure > 20)
    {
        pressure = 1;
    }
    if (pressure < EPSILON)
    {
        pressure = lastpressure;
    }
    lastpressure = pressure;
}
//communication serial via usb PC
int status = 0;
uint32_t time_start;
int arraySize;
int count = 0;
const int maxNumbers = 2000;
float setPoint[2000];
int j = 0; //vị trí
void readSerial(void);
void readSerial()
{
    R_SCI_UART_Read (&g_uart0_ctrl, (uint8_t*) g_uart_rx_buffer, UART_BUFFER_SIZE);

    if (strncmp (g_uart_rx_buffer, "PAUSE", 5) == 0)
    {
        status = 0;
    }
    else if (strncmp (g_uart_rx_buffer, "CONTINUE", 8) == 0)
    {
        status = 1;
    }
    else if (strncmp (g_uart_rx_buffer, "STOP", 4) == 0)
    {
        status = 2;
    }
    else if (strncmp (g_uart_rx_buffer, "START", 5) == 0)
    {
        status = 1;
        time_start = g_millis;
        j = 0;
    }
    else if (strncmp (g_uart_rx_buffer, "HOME", 4) == 0)
    {
        status = 4;
    }
    else if (strncmp (g_uart_rx_buffer, "SETUP", 5) == 0)
    {
        status = 3;
    }
    else if (strncmp (g_uart_rx_buffer, "#", 1))
    {
        j = 0;
        arraySize = 0;
        int lastIndex = 0;
        int currentIndex = 0;
        while (currentIndex < (int) strlen (g_uart_rx_buffer) && arraySize < maxNumbers)
        {
            char *comma = strchr (&g_uart_rx_buffer[lastIndex], ',');
            if (comma != NULL)
            {
                currentIndex = comma - g_uart_rx_buffer;
            }
            else
            {
                currentIndex = (int) strlen (g_uart_rx_buffer);
            }
            char numberString[16];
            strncpy (numberString, &g_uart_rx_buffer[lastIndex], currentIndex - lastIndex);
            numberString[currentIndex - lastIndex] = '\0';
            setPoint[arraySize] = strtof (numberString, NULL);
            arraySize++;
            lastIndex = currentIndex + 1;
        }
    }
    // memset(g_uart_rx_buffer, 0, sizeof(g_uart_rx_buffer));
}
float setPointPressuse, last_setPointPressuse;

void setpoint(void);
void setpoint()
{
    uint32_t time_now = g_millis;
    uint32_t time_end_pressure;
    if (time_now - time_start > 1000 && status == 1)
    {
        //timer
        time_start = time_now;
        if (j < arraySize)
        {
            setPointPressuse = setPoint[j];
            j++;
            time_end_pressure = g_millis;
        }
        else
        {
            if (g_millis - time_end_pressure > 5000)
            {
                status = 0;
            }
        }
        if (setPointPressuse > 100)
        {
            setPointPressuse = 100;
        }
        last_setPointPressuse = setPointPressuse;
    }
}

#define GPT_EXAMPLE_MSEC_PER_SEC           (1000)
float GPT_EXAMPLE_DESIRED_PERIOD_MSEC;
#define GPT_EXAMPLE_DESIRED_DUTY_CYCLE_PERCENT    (50)
#define GPT_EXAMPLE_MAX_PERCENT                   (100)
int frequency;
void set_pul_speed(int f);
void set_pul_speed(int f)
{
    (void) R_GPT_Start (&g_timer1_ctrl);
    //simple_rand(f);
    //desired_period_msec = 1000.0/f;
    // Chuyển đơn vị của GPT_EXAMPLE_DESIRED_PERIOD_MSEC sang mili giây
    GPT_EXAMPLE_DESIRED_PERIOD_MSEC = 1000.0 / f;
    uint32_t pclkd_freq_hz = R_FSP_SystemClockHzGet (FSP_PRIV_CLOCK_PCLKD) >> g_timer1_cfg.source_div;
    // Tính toán period_counts
    uint32_t period_counts = (uint32_t) (((uint64_t) pclkd_freq_hz * GPT_EXAMPLE_DESIRED_PERIOD_MSEC)
            / GPT_EXAMPLE_MSEC_PER_SEC);
    /* Set the calculated period. */
    err = R_GPT_PeriodSet (&g_timer1_ctrl, period_counts);
    /* Get the current period setting. */
    timer_info_t info;
    (void) R_GPT_InfoGet (&g_timer1_ctrl, &info);
    uint32_t current_period_counts = info.period_counts;
    /* Calculate the desired duty cycle based on the current period. Note that if the period could be larger than
     * UINT32_MAX / 100, this calculation could overflow. A cast to uint64_t is used to prevent this. The cast is
     * not required for 16-bit timers. */
    uint32_t duty_cycle_counts = (uint32_t) (((uint64_t) current_period_counts * GPT_EXAMPLE_DESIRED_DUTY_CYCLE_PERCENT)
            / GPT_EXAMPLE_MAX_PERCENT);
    /* Set the calculated duty cycle. */
    err = R_GPT_DutyCycleSet (&g_timer1_ctrl, duty_cycle_counts, GPT_IO_PIN_GTIOCB);
    //assert(FSP_SUCCESS == err);
    R_BSP_SoftwareDelay (50, BSP_DELAY_UNITS_MILLISECONDS);  // Low for half the period
}

void stop_pressure(void);
void stop_pressure()
{
    //stop
    frequency = 0;
    set_pul_speed (frequency);
    R_IOPORT_PinWrite (&g_ioport_ctrl, PUL, BSP_IO_LEVEL_LOW);
}
float frequencyMax = 2400;
float frequencyMin = 6;
int kp = 1000;
float error = 1;
void control_servo(void);
void control_servo()
{
    error = setPointPressuse - pressure;

    if (error > 0)
    {
        R_IOPORT_PinWrite (&g_ioport_ctrl, DIR, BSP_IO_LEVEL_LOW);
    }
    else
    {
        R_IOPORT_PinWrite (&g_ioport_ctrl, DIR, BSP_IO_LEVEL_HIGH);
    }
    error = fabsf (error);
    if (error > 2)
    {
        frequency = (int) (error * 200 + 12);
        set_pul_speed (frequency);
    }
    else
    {
        float P;
        P = error * (float) kp;
        frequency = (int)P;
        if (frequency > frequencyMax)
            frequency = (int) frequencyMax;
        if (frequency > frequencyMin)
            frequency = (int) frequencyMin;
        if (error <= 0.01)
        {
            stop_pressure ();
        }
        else
        {
            set_pul_speed (frequency);
        }
    }
}
uint32_t time_send = 0;
//uint32_t current_time = 0;
uint32_t last_time = 0;
int delta_T = 250; // millis second
void sent_data_to_PC(void);
void sent_data_to_PC()
{
    /*do không thể gửi định dang float, double trong hàm snprintf nên áp suất được gửi lên pc sẽ là mili bar
     * Định dạng dữ liệu: [pressure,status,time]
     * với pressure đơn vị mBar, status là trạng thái hiện tại của hệ thống,
     * time là thời gian đếm từ lúc cấp nguồn cho hệ thống*/

    uint32_t current_time = g_millis;
    if (current_time - last_time >= delta_T)
    {
        last_time = current_time;
        char message[UART_BUFFER_SIZE];

        snprintf (message, UART_BUFFER_SIZE,
                  "Data send to PC: %d mbar, status %d , %lu,f: %d Hz, error: %d, set: %d mBar \n",
                  (int) (pressure * 1000), status, current_time, (int) frequency, (int) (error * 1000),
                  (int) (setPointPressuse * 1000));
        g_uart_transmit_complete = false;
        R_SCI_UART_Write (&g_uart0_ctrl, (uint8_t*) message, strlen (message));

        while (!g_uart_transmit_complete)
        {
            /* Optional: add a small delay to avoid busy-waiting
             * chờ gửi lên PC xong*/
        }
    }
}
bool status_home = false;
bool status_setup = false;
bsp_io_level_t button1_status, button2_status;
void goHome();
void goHome()
{
    if (status_home == true)
    {
        R_IOPORT_PinRead (&g_ioport_ctrl, SW1, &button1_status);
        if (button1_status == BSP_IO_LEVEL_LOW)
        {
            while (status_home == true)
            {
                R_IOPORT_PinWrite (&g_ioport_ctrl, DIR, BSP_IO_LEVEL_HIGH);
                R_IOPORT_PinWrite (&g_ioport_ctrl, VAL1, BSP_IO_LEVEL_HIGH);
                R_IOPORT_PinWrite (&g_ioport_ctrl, VAL2, BSP_IO_LEVEL_HIGH);
                R_BSP_SoftwareDelay (20, BSP_DELAY_UNITS_MICROSECONDS);
                set_pul_speed (800);
                R_IOPORT_PinRead (&g_ioport_ctrl, SW1, &button1_status);
                if (button1_status == BSP_IO_LEVEL_HIGH)
                {
                    uint32_t start_time_reset;
                    start_time_reset = g_millis;
                    stop_pressure ();
                    R_BSP_SoftwareDelay (1000, BSP_DELAY_UNITS_MILLISECONDS);
                    while (1)
                    {
                        R_IOPORT_PinWrite (&g_ioport_ctrl, DIR, BSP_IO_LEVEL_LOW);
                        R_BSP_SoftwareDelay (20, BSP_DELAY_UNITS_MICROSECONDS);
                        set_pul_speed (800);
                        if (button1_status == BSP_IO_LEVEL_LOW || g_millis - start_time_reset > 5000)
                        {
                            stop_pressure ();
                            R_IOPORT_PinWrite (&g_ioport_ctrl, VAL1, BSP_IO_LEVEL_LOW);
                            R_IOPORT_PinWrite (&g_ioport_ctrl, VAL2, BSP_IO_LEVEL_LOW);
                            callPressure ();
                            setPointPressuse = pressure;
                            R_BSP_SoftwareDelay (500, BSP_DELAY_UNITS_MILLISECONDS);
                            break;
                        }
                        status_home = false;
                        break;
                    }
                }
            }
        }
    }
}
void setup_pump();
void setup_pump()
{
    if (status_setup == true)
    {
        R_IOPORT_PinWrite (&g_ioport_ctrl, VAL1, BSP_IO_LEVEL_HIGH);
        R_IOPORT_PinWrite (&g_ioport_ctrl, VAL2, BSP_IO_LEVEL_HIGH);
        R_IOPORT_PinWrite (&g_ioport_ctrl, PUMP1, BSP_IO_LEVEL_HIGH);
        uint32_t start_pump1 = g_millis;
        while (1)
        {
            callPressure ();
            if (g_millis - start_pump1 > 300000 || pressure > 3)
            {
                R_IOPORT_PinWrite (&g_ioport_ctrl, VAL1, BSP_IO_LEVEL_LOW);
                R_IOPORT_PinWrite (&g_ioport_ctrl, VAL2, BSP_IO_LEVEL_LOW);
                R_IOPORT_PinWrite (&g_ioport_ctrl, PUMP1, BSP_IO_LEVEL_LOW);
                break;
            }
        }
    }
}
/*RULE:
 * SW1<-----<-----<--PISTON-->----->----->SW2
 * SW1 = LOW PRESSURE
 * SW2 = HIGH PRESSURE - WARNING
 * Wiring SINK - normal state = LOW
 * */
bool limit_state = false;
void limit_sw();
void limit_sw()
{
    R_IOPORT_PinRead (&g_ioport_ctrl, SW1, &button1_status);
    R_IOPORT_PinRead (&g_ioport_ctrl, SW2, &button2_status);
    if (button1_status == BSP_IO_LEVEL_HIGH || button2_status == BSP_IO_LEVEL_HIGH)
    {
        limit_state = true;
        stop_pressure ();
        R_BSP_SoftwareDelay (1000, BSP_DELAY_UNITS_MILLISECONDS);
        R_IOPORT_PinRead (&g_ioport_ctrl, SW1, &button1_status);
        R_IOPORT_PinRead (&g_ioport_ctrl, SW2, &button2_status);
        if (button1_status == BSP_IO_LEVEL_HIGH)
        {
            while (1)
            {
                R_IOPORT_PinWrite (&g_ioport_ctrl, DIR, BSP_IO_LEVEL_HIGH);
                R_BSP_SoftwareDelay (20, BSP_DELAY_UNITS_MICROSECONDS);
                set_pul_speed (800);
                R_IOPORT_PinRead (&g_ioport_ctrl, SW1, &button1_status);
                if (button1_status == BSP_IO_LEVEL_HIGH)
                {
                    //stop bài đo
                    stop_pressure ();
                    callPressure ();
                    setPointPressuse = pressure;
                    R_BSP_SoftwareDelay (1000, BSP_DELAY_UNITS_MILLISECONDS);
                    break;
                }
            }
        }
        if (button2_status == BSP_IO_LEVEL_HIGH)
        {
            while (1)
            {
                R_IOPORT_PinWrite (&g_ioport_ctrl, DIR, BSP_IO_LEVEL_LOW);
                R_BSP_SoftwareDelay (20, BSP_DELAY_UNITS_MICROSECONDS);
                set_pul_speed (800);
                R_IOPORT_PinRead (&g_ioport_ctrl, SW2, &button2_status);
                if (button2_status == BSP_IO_LEVEL_HIGH)
                {
                    //stop bài đo không?
                    stop_pressure ();
                    callPressure ();
                    setPointPressuse = pressure;
                    R_BSP_SoftwareDelay (1000, BSP_DELAY_UNITS_MILLISECONDS);
                    break;
                }
            }
        }
    }
}

/*******************************************************************************************************************//**
 * main() is generated by the RA Configuration editor and is used to generate threads if an RTOS is used.  This function
 * is called by main() when no RTOS is used.
 **********************************************************************************************************************/
void hal_entry(void)
{
    /*open port*/
    R_IOPORT_Open (&g_ioport_ctrl, &g_bsp_pin_cfg);

    /*UART*/
    R_SCI_UART_Open (&g_uart0_ctrl, &g_uart0_cfg);
    R_SCI_UART_Open (&g_uart1_ctrl, &g_uart1_cfg);

    /* Set UART callback */
    uart_callback_args_t uart_callback_memory;
    R_SCI_UART_CallbackSet (&g_uart0_ctrl, uart_callback, NULL, &uart_callback_memory);

    /* Configure AGT timer to increment millis */
    timer_callback_args_t timer_callback_memory;
    R_AGT_Open (&g_timer0_ctrl, &g_timer0_cfg);
    R_AGT_CallbackSet (&g_timer0_ctrl, agt_callback, NULL, &timer_callback_memory);
    R_AGT_Start (&g_timer0_ctrl);

    /* Configure GPT timer to increment millis */
    err = R_GPT_Open (&g_timer1_ctrl, &g_timer1_cfg);
    /* Handle any errors. This function should be defined by the user. */
    assert(FSP_SUCCESS == err);
    /* Start the timer. */
    (void) R_GPT_Start (&g_timer1_ctrl);
    last_time = 0;

    while (1)
    {

        if (limit_state == true)
        {
            limit_sw ();
        }
        else
        {
            readSerial ();
            callPressure ();
            if(status == 1)
            {
                setpoint();
                control_servo ();
            }
            else if ( status == 0)
            {
               stop_pressure();
            }
            sent_data_to_PC ();
        }
        //R_BSP_SoftwareDelay (1, BSP_DELAY_UNITS_MILLISECONDS);
    }

    /* TODO: add your own code here */

#if BSP_TZ_SECURE_BUILD
    /* Enter non-secure code */
    R_BSP_NonSecureEnter();
#endif
}

/*******************************************************************************************************************//**
 * This function is called at various points during the startup process.  This implementation uses the event that is
 * called right before main() to set up the pins.
 *
 * @param[in]  event    Where at in the start up process the code is currently at
 **********************************************************************************************************************/
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
BSP_CMSE_NONSECURE_ENTRY void template_nonsecure_callable ();

/* Trustzone Secure Projects require at least one nonsecure callable function in order to build (Remove this if it is not required to build). */
BSP_CMSE_NONSECURE_ENTRY void template_nonsecure_callable ()
{

}
FSP_CPP_FOOTER

#endif
