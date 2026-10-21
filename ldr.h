
#ifndef LDR_H
#define LDR_H

#include <stdint.h>

void LDR_Init(void);
uint16_t LDR_ReadRaw(void);
uint8_t LDR_IsDark(void);
void LDR_UpdateLEDs(void);

/* Use an existing ADC reading without another conversion */
uint8_t LDR_IsDarkValue(uint16_t value);
void LDR_UpdateLEDsFromValue(uint16_t value);

#endif
