
#include "stm32f4xx.h"
#include "hc05.h"
#include "hcsr04.h"
#include "ldr.h"

#define OBSTACLE_STOP_DISTANCE_CM  15U

/* HC-SR04 status */
volatile uint32_t obstacle_distance_cm = 0U;
volatile uint8_t distance_valid = 0U;
volatile uint8_t obstacle_detected = 1U;

/* HC-05 command */
volatile HC05_Command robot_command = HC05_STOP;

/* LDR status */
volatile uint16_t light_raw = 0U;
volatile uint8_t is_dark = 0U;

int main(void)
{
    HC05_Init();
    HCSR04_Init();
    LDR_Init();

    while (1)
    {
        /* Process Bluetooth commands */
        HC05_Process();
        robot_command = HC05_GetCommand();

        /* Advance non-blocking HC-SR04 measurement */
        HCSR04_Process();

        distance_valid =
            HCSR04_GetDistance(&obstacle_distance_cm);

        if (distance_valid)
        {
            obstacle_detected =
                (obstacle_distance_cm <=
                 OBSTACLE_STOP_DISTANCE_CM) ? 1U : 0U;
        }
        else
        {
            /* Invalid distance: do not assume path is clear */
            obstacle_detected = 1U;
        }

        /* Read LDR once and reuse the same reading */
        light_raw = LDR_ReadRaw();

        is_dark = LDR_IsDarkValue(light_raw);

        /* Control both yellow LEDs from this reading */
        LDR_UpdateLEDsFromValue(light_raw);
    }
}
