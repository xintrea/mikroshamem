#ifndef INITDEVICE_H
#define INITDEVICE_H

int clockInit(void);
void portClockInit(void);
void addressBusInit(void);
void dataBusInit(void);
void systemPinsInit(void);
void debugLedInit(void);

#endif
