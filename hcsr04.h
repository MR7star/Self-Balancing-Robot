#ifndef HCSR04_H
#define HCSR04_H

#include <stdint.h>

typedef enum
{
    HCSR04_OK = 0,
    HCSR04_ECHO_RISE_TIMEOUT,
    HCSR04_ECHO_FALL_TIMEOUT,
    HCSR04_ECHO_STUCK_HIGH
} HCSR04_Status;

void HCSR04_Init(void);

/* Call frequently from the main loop; does not wait for an echo. */
void HCSR04_Process(void);

/* Returns 1 if the latest measurement is valid, otherwise 0. */
uint8_t HCSR04_GetDistance(volatile uint32_t *distance_cm);

uint32_t HCSR04_GetEchoTimeUs(void);
HCSR04_Status HCSR04_GetStatus(void);

#endif


/*
#ifndef HCSR04_H
#define HCSR04_H

#include <stdint.h>

typedef enum
{
    HCSR04_OK = 0,
    HCSR04_ECHO_RISE_TIMEOUT,
    HCSR04_ECHO_FALL_TIMEOUT
} HCSR04_Status;

void HCSR04_Init(void);

uint8_t HCSR04_ReadDistance(volatile uint32_t *distance_cm);

uint32_t HCSR04_GetEchoTimeUs(void);

HCSR04_Status HCSR04_GetStatus(void);

#endif
*/