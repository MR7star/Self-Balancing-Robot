
#include "stm32f4xx.h"
#include "ldr.h"

#define LDR_DARK_THRESHOLD  2000U

#define LED1_PIN  5U
#define LED2_PIN  6U

void LDR_Init(void)
{
    /* Enable GPIOA and GPIOB clocks */
    RCC->AHB1ENR |= (1U << 0) | (1U << 1);

    /* Enable ADC1 clock */
    RCC->APB2ENR |= (1U << 8);

    /* PA0: analog input for LDR AO */
    GPIOA->MODER &= ~(3U << 0);
    GPIOA->MODER |=  (3U << 0);
    GPIOA->PUPDR &= ~(3U << 0);

    /* PB5 and PB6: GPIO outputs for yellow LEDs */
    GPIOB->MODER &= ~((3U << (LED1_PIN * 2U)) |
                      (3U << (LED2_PIN * 2U)));

    GPIOB->MODER |=  ((1U << (LED1_PIN * 2U)) |
                      (1U << (LED2_PIN * 2U)));

    /* Push-pull output */
    GPIOB->OTYPER &= ~((1U << LED1_PIN) |
                       (1U << LED2_PIN));

    /* No pull-up or pull-down */
    GPIOB->PUPDR &= ~((3U << (LED1_PIN * 2U)) |
                      (3U << (LED2_PIN * 2U)));

    /* Initially turn both LEDs OFF */
    GPIOB->BSRR = (1U << (LED1_PIN + 16U)) |
                  (1U << (LED2_PIN + 16U));

    /* ADC prescaler: PCLK2 / 2 */
    ADC->CCR &= ~(3U << 16);

    /* ADC1: single conversion, channel 0 */
    ADC1->CR1 = 0U;
    ADC1->CR2 = 0U;
    ADC1->SQR1 = 0U;
    ADC1->SQR3 = 0U;

    /* Channel 0 sample time */
    ADC1->SMPR2 &= ~(7U << 0);
    ADC1->SMPR2 |=  (5U << 0);

    /* Enable ADC1 */
    ADC1->CR2 |= (1U << 0);
}

uint16_t LDR_ReadRaw(void)
{
    /* Select ADC channel 0 (PA0) */
    ADC1->SQR3 = 0U;

    /* Start conversion */
    ADC1->CR2 |= (1U << 30);

    /* Wait until conversion completes */
    while (!(ADC1->SR & (1U << 1)))
    {
    }

    return (uint16_t)(ADC1->DR & 0x0FFFU);
}

uint8_t LDR_IsDarkValue(uint16_t value)
{
    /* Assumes ADC reading decreases in darkness */
    return (value > LDR_DARK_THRESHOLD) ? 1U : 0U;
}

uint8_t LDR_IsDark(void)
{
    return LDR_IsDarkValue(LDR_ReadRaw());
}

void LDR_UpdateLEDsFromValue(uint16_t value)
{
    if (LDR_IsDarkValue(value))
    {
        /* Dark: turn both LEDs ON */
        GPIOB->BSRR = (1U << LED1_PIN) |
                      (1U << LED2_PIN);
    }
    else
    {
        /* Bright: turn both LEDs OFF */
        GPIOB->BSRR = (1U << (LED1_PIN + 16U)) |
                      (1U << (LED2_PIN + 16U));
    }
}

void LDR_UpdateLEDs(void)
{
    LDR_UpdateLEDsFromValue(LDR_ReadRaw());
}
