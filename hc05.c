
#include "stm32f4xx.h"
#include "hc05.h"

/* Store the most recently received command */
static volatile HC05_Command current_command = HC05_STOP;

/* ================= TRANSMIT FUNCTIONS ================= */

void HC05_SendChar(char ch)
{
    /* Wait until transmit data register is empty */
    while (!(USART2->SR & (1U << 7)))
    {
    }

    USART2->DR = (uint8_t)ch;
}

void HC05_SendString(const char *str)
{
    while (*str != '\0')
    {
        HC05_SendChar(*str);
        str++;
    }

    /* Wait until the final character has been transmitted */
    while (!(USART2->SR & (1U << 6)))
    {
    }
}

/* ================= INITIALIZATION ================= */

void HC05_Init(void)
{
    /* Enable GPIOA and USART2 clocks */
    RCC->AHB1ENR |= (1U << 0);
    RCC->APB1ENR |= (1U << 17);

    /* PA2 = USART2_TX, PA3 = USART2_RX */
    GPIOA->MODER &= ~((3U << 4) | (3U << 6));
    GPIOA->MODER |=  ((2U << 4) | (2U << 6));

    /* Select alternate function AF7 */
    GPIOA->AFR[0] &= ~((0xFU << 8) | (0xFU << 12));
    GPIOA->AFR[0] |=  ((7U << 8) | (7U << 12));

    /* Configure push-pull and no pull resistors */
    GPIOA->OTYPER &= ~((1U << 2) | (1U << 3));
    GPIOA->PUPDR &= ~((3U << 4) | (3U << 6));

    /* Configure USART2 */
    USART2->CR1 = 0U;
    USART2->CR2 = 0U;
    USART2->CR3 = 0U;

    /* 9600 baud when USART2 clock is 16 MHz */
    USART2->BRR = 1667U;

    /* Enable receiver, transmitter and USART */
    USART2->CR1 = (1U << 2) |
                  (1U << 3) |
                  (1U << 13);

    current_command = HC05_STOP;

    /* Startup message */
    HC05_SendString("HC-05 ready\r\n");
}

/* ================= COMMAND PROCESSING ================= */

void HC05_Process(void)
{
    uint8_t received;

    /* Process all bytes currently available */
    while (USART2->SR & (1U << 5))
    {
        received = (uint8_t)USART2->DR;

        switch (received)
        {
            case 'F':
            case 'f':
                current_command = HC05_FORWARD;
                HC05_SendString("Forward command received\r\n");
                break;

            case 'B':
            case 'b':
                current_command = HC05_BACKWARD;
                HC05_SendString("Backward command received\r\n");
                break;

            case 'L':
            case 'l':
                current_command = HC05_LEFT;
                HC05_SendString("Left command received\r\n");
                break;

            case 'R':
            case 'r':
                current_command = HC05_RIGHT;
                HC05_SendString("Right command received\r\n");
                break;

            case 'S':
            case 's':
                current_command = HC05_STOP;
                HC05_SendString("Stop command received\r\n");
                break;

            case '\r':
            case '\n':
                /* Ignore terminal line endings */
                break;

            default:
                HC05_SendString("Unknown command\r\n");
                break;
        }
    }
}

/* ================= GET CURRENT COMMAND ================= */

HC05_Command HC05_GetCommand(void)
{
    return current_command;
}
