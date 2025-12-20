#ifndef POWER_ESTIMATOR_H
#define POWER_ESTIMATOR_H

#include <Arduino.h>

void pe_begin(unsigned long reportIntervalMs = 5000);
void pe_addCpuMs(unsigned long ms);
void pe_addWifiMs(unsigned long ms);
void pe_addIdleMs(unsigned long ms);
void pe_addSleepMs(unsigned long ms);
void pe_subtractIdleMs(unsigned long ms);

void pe_tickAndMaybePrint(); // call regularly from loop()

#endif // POWER_ESTIMATOR_H
