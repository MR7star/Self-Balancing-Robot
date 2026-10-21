
#ifndef HC05_H
#define HC05_H

#include <stdint.h>

typedef enum
{
    HC05_STOP     = 'S',
    HC05_FORWARD  = 'F',
    HC05_BACKWARD = 'B',
    HC05_LEFT     = 'L',
    HC05_RIGHT    = 'R'
} HC05_Command;

void HC05_Init(void);
void HC05_Process(void);
HC05_Command HC05_GetCommand(void);

void HC05_SendChar(char ch);
void HC05_SendString(const char *str);

#endif
