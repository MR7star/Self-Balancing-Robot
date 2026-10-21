#include "stm32f4xx.h"
#include "hcsr04.h"

#define HCSR04_TIM2_PRESCALER  15U
#define HCSR04_TIMEOUT_US      30000U
#define HCSR04_TRIGGER_US      10U
#define HCSR04_SAMPLE_DELAY_US 60000U

/* Keil Watch 1 debug variables */
volatile uint32_t hcsr04_echo_time_us = 0U;
volatile uint32_t hcsr04_debug_start = 0U;
volatile uint32_t hcsr04_debug_end = 0U;
volatile uint32_t hcsr04_debug_status = HCSR04_OK;
volatile uint32_t hcsr04_echo_pin = 0U;
volatile uint32_t hcsr04_debug_stage = 0U;
volatile uint32_t hcsr04_timeout_count = 0U;

typedef enum
{
    STATE_IDLE = 0,
    STATE_TRIGGER_HIGH,
    STATE_WAIT_RISE,
    STATE_WAIT_FALL
} HCSR04_State;

static HCSR04_State state = STATE_IDLE;

static uint32_t state_start_us = 0U;
static uint32_t echo_start_us = 0U;
static uint32_t last_cycle_us = 0U;
static uint32_t last_distance_cm = 0U;
static uint8_t measurement_valid = 0U;

static void TIM2_Init(void)
{
    RCC->APB1ENR |= (1U << 0);

    TIM2->CR1 = 0U;
    TIM2->PSC = HCSR04_TIM2_PRESCALER;
    TIM2->ARR = 0xFFFFFFFFU;
    TIM2->CNT = 0U;
    TIM2->EGR = 1U;
    TIM2->SR = 0U;
    TIM2->CR1 |= 1U;
}

static void GPIO_Init(void)
{
    RCC->AHB1ENR |= (1U << 1);

    /* PB0: TRIG output */
    GPIOB->MODER &= ~(3U << 0);
    GPIOB->MODER |=  (1U << 0);
    GPIOB->OTYPER &= ~(1U << 0);
    GPIOB->PUPDR &= ~(3U << 0);

    /* PB1: ECHO input */
    GPIOB->MODER &= ~(3U << 2);
    GPIOB->PUPDR &= ~(3U << 2);

    /* TRIG LOW initially */
    GPIOB->BSRR = (1U << 16);
}

static uint32_t TimerNowUs(void)
{
    return TIM2->CNT;
}

static uint8_t EchoPinHigh(void)
{
    hcsr04_echo_pin = (GPIOB->IDR >> 1) & 1U;
    return (uint8_t)hcsr04_echo_pin;
}

static void MeasurementFailed(HCSR04_Status status,
                              uint32_t debug_stage)
{
    GPIOB->BSRR = (1U << 16);  /* Ensure TRIG is LOW */

    hcsr04_debug_status = (uint32_t)status;
    hcsr04_debug_stage = debug_stage;
    hcsr04_timeout_count++;

    measurement_valid = 0U;
    state = STATE_IDLE;
    last_cycle_us = TimerNowUs();
}

void HCSR04_Init(void)
{
    GPIO_Init();
    TIM2_Init();

    hcsr04_echo_time_us = 0U;
    hcsr04_debug_start = 0U;
    hcsr04_debug_end = 0U;
    hcsr04_debug_status = HCSR04_OK;
    hcsr04_echo_pin = 0U;
    hcsr04_debug_stage = 0U;
    hcsr04_timeout_count = 0U;

    state = STATE_IDLE;
    state_start_us = 0U;
    echo_start_us = 0U;
    last_cycle_us = TimerNowUs() - HCSR04_SAMPLE_DELAY_US;
    last_distance_cm = 0U;
    measurement_valid = 0U;
}

void HCSR04_Process(void)
{
    uint32_t now = TimerNowUs();
    uint32_t elapsed;

    /* Keep the latest ECHO level visible in Keil */
    EchoPinHigh();

    switch (state)
    {
        case STATE_IDLE:

            if ((uint32_t)(now - last_cycle_us) <
                HCSR04_SAMPLE_DELAY_US)
            {
                break;
            }

            /*
             * ECHO should be LOW before a new trigger.
             * Do not wait here if it is stuck HIGH.
             */
            if (hcsr04_echo_pin != 0U)
            {
                MeasurementFailed(
                    HCSR04_ECHO_STUCK_HIGH, 7U);
                break;
            }

            hcsr04_echo_time_us = 0U;
            hcsr04_debug_start = 0U;
            hcsr04_debug_end = 0U;
            hcsr04_debug_status = HCSR04_OK;

            GPIOB->BSRR = (1U << 0);  /* TRIG HIGH */

            state_start_us = now;
            last_cycle_us = now;
            state = STATE_TRIGGER_HIGH;
            hcsr04_debug_stage = 1U;
            break;

        case STATE_TRIGGER_HIGH:

            elapsed = (uint32_t)(now - state_start_us);

            if (elapsed >= HCSR04_TRIGGER_US)
            {
                GPIOB->BSRR = (1U << 16); /* TRIG LOW */

                state_start_us = now;
                state = STATE_WAIT_RISE;
                hcsr04_debug_stage = 2U;
            }
            break;

        case STATE_WAIT_RISE:

            if (hcsr04_echo_pin != 0U)
            {
                echo_start_us = now;
                hcsr04_debug_start = now;

                state_start_us = now;
                state = STATE_WAIT_FALL;
                hcsr04_debug_stage = 3U;
            }
            else if ((uint32_t)(now - state_start_us) >=
                     HCSR04_TIMEOUT_US)
            {
                MeasurementFailed(
                    HCSR04_ECHO_RISE_TIMEOUT, 5U);
            }
            break;

        case STATE_WAIT_FALL:

            if (hcsr04_echo_pin == 0U)
            {
                hcsr04_debug_end = now;

                hcsr04_echo_time_us =
                    (uint32_t)(now - echo_start_us);

                last_distance_cm =
                    (hcsr04_echo_time_us + 29U) / 58U;

                hcsr04_debug_status = HCSR04_OK;
                hcsr04_debug_stage = 4U;
                measurement_valid = 1U;

                state = STATE_IDLE;
                last_cycle_us = now;
            }
            else if ((uint32_t)(now - state_start_us) >=
                     HCSR04_TIMEOUT_US)
            {
                MeasurementFailed(
                    HCSR04_ECHO_FALL_TIMEOUT, 6U);
            }
            break;

        default:
            GPIOB->BSRR = (1U << 16);
            state = STATE_IDLE;
            measurement_valid = 0U;
            last_cycle_us = now;
            break;
    }
}

uint8_t HCSR04_GetDistance(volatile uint32_t *distance_cm)
{
    if (distance_cm == 0)
    {
        return 0U;
    }

    *distance_cm = last_distance_cm;
    return measurement_valid;
}

uint32_t HCSR04_GetEchoTimeUs(void)
{
    return hcsr04_echo_time_us;
}

HCSR04_Status HCSR04_GetStatus(void)
{
    return (HCSR04_Status)hcsr04_debug_status;
}



/*
#include "stm32f4xx.h"
#include "hcsr04.h"

#define HCSR04_TIM2_PRESCALER  15U
#define HCSR04_TIMEOUT_US      30000U
#define HCSR04_TRIGGER_US      10U
#define HCSR04_SAMPLE_DELAY_US 60000U

// Keil Watch 1 debug variables
volatile uint32_t hcsr04_echo_time_us = 0U;
volatile uint32_t hcsr04_debug_start = 0U;
volatile uint32_t hcsr04_debug_end = 0U;
volatile uint32_t hcsr04_debug_status = HCSR04_OK;
volatile uint32_t hcsr04_echo_pin = 0U;
volatile uint32_t hcsr04_debug_stage = 0U;
volatile uint32_t hcsr04_timeout_count = 0U;

static void TIM2_Init(void)
{
    RCC->APB1ENR |= (1U << 0);

    TIM2->CR1 = 0U;
    TIM2->PSC = HCSR04_TIM2_PRESCALER;
    TIM2->ARR = 0xFFFFFFFFU;
    TIM2->CNT = 0U;
    TIM2->EGR = 1U;
    TIM2->SR = 0U;
    TIM2->CR1 |= 1U;
}

static void GPIO_Init(void)
{
    RCC->AHB1ENR |= (1U << 1);

    // PB0: TRIG output
    GPIOB->MODER &= ~(3U << 0);
    GPIOB->MODER |=  (1U << 0);
    GPIOB->OTYPER &= ~(1U << 0);
    GPIOB->PUPDR &= ~(3U << 0);

    // PB1: ECHO input
    GPIOB->MODER &= ~(3U << 2);
    GPIOB->PUPDR &= ~(3U << 2);

    // TRIG LOW initially
    GPIOB->BSRR = (1U << 16);
}

static void DelayUs(uint32_t us)
{
    uint32_t start = TIM2->CNT;

    while ((uint32_t)(TIM2->CNT - start) < us)
    {
    }
}

static uint8_t WaitForEcho(uint8_t level, uint32_t timeout_us)
{
    uint32_t start = TIM2->CNT;

    while (1)
    {
        hcsr04_echo_pin = (GPIOB->IDR >> 1) & 1U;

        if (hcsr04_echo_pin == level)
        {
            return 1U;
        }

        if ((uint32_t)(TIM2->CNT - start) >= timeout_us)
        {
            hcsr04_timeout_count++;
            return 0U;
        }
    }
}

void HCSR04_Init(void)
{
    GPIO_Init();
    TIM2_Init();

    hcsr04_echo_time_us = 0U;
    hcsr04_debug_start = 0U;
    hcsr04_debug_end = 0U;
    hcsr04_debug_status = HCSR04_OK;
    hcsr04_echo_pin = 0U;
    hcsr04_debug_stage = 0U;
    hcsr04_timeout_count = 0U;
}

uint8_t HCSR04_ReadDistance(volatile uint32_t *distance_cm)
{
    uint32_t start;
    uint8_t success = 0U;

    if (distance_cm == 0)
    {
        return 0U;
    }

    *distance_cm = 0U;
    hcsr04_echo_time_us = 0U;
    hcsr04_debug_start = 0U;
    hcsr04_debug_end = 0U;
    hcsr04_debug_status = HCSR04_OK;
    hcsr04_debug_stage = 1U;

    // ECHO should be LOW before triggering
    if (!WaitForEcho(0U, HCSR04_TIMEOUT_US))
    {
        hcsr04_debug_status = HCSR04_ECHO_RISE_TIMEOUT;
        goto measurement_done;
    }

    // Send trigger pulse
    GPIOB->BSRR = (1U << 16);
    DelayUs(2U);

    GPIOB->BSRR = (1U << 0);
    DelayUs(HCSR04_TRIGGER_US);
    GPIOB->BSRR = (1U << 16);

    // Wait for ECHO rising edge
    hcsr04_debug_stage = 2U;

    if (!WaitForEcho(1U, HCSR04_TIMEOUT_US))
    {
        hcsr04_debug_status = HCSR04_ECHO_RISE_TIMEOUT;
        hcsr04_debug_stage = 5U;
        goto measurement_done;
    }

    // Capture start of ECHO HIGH
    hcsr04_debug_stage = 3U;
    start = TIM2->CNT;
    hcsr04_debug_start = start;

    // Wait for ECHO falling edge
    if (!WaitForEcho(0U, HCSR04_TIMEOUT_US))
    {
        hcsr04_debug_status = HCSR04_ECHO_FALL_TIMEOUT;
        hcsr04_debug_stage = 6U;
        goto measurement_done;
    }

    // Calculate pulse width and distance
    hcsr04_debug_end = TIM2->CNT;
    hcsr04_echo_time_us =
        (uint32_t)(hcsr04_debug_end - start);

    *distance_cm = (hcsr04_echo_time_us + 29U) / 58U;

    hcsr04_debug_status = HCSR04_OK;
    hcsr04_debug_stage = 4U;
    success = 1U;

measurement_done:
    // Wait before the next measurement, even after a failure
    DelayUs(HCSR04_SAMPLE_DELAY_US);

    return success;
}

uint32_t HCSR04_GetEchoTimeUs(void)
{
    return hcsr04_echo_time_us;
}

HCSR04_Status HCSR04_GetStatus(void)
{
    return (HCSR04_Status)hcsr04_debug_status;
}
*/