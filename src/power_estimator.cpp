#include "power_estimator.h"
#include <Arduino.h>

static unsigned long _idle_ms = 0;
static unsigned long _sleep_ms = 0;
static unsigned long _cpu_ms  = 0;
static unsigned long _wifi_ms = 0;
static unsigned long _last_report = 0;
static unsigned long _report_interval = 5000;

// simple configurable currents (mA)
static constexpr float I_IDLE = 70.0f;
static constexpr float I_CPU  = 90.0f;
static constexpr float I_WIFI = 150.0f;
static constexpr float I_SLEEP = 15.0f;   // sleep current

void pe_begin(unsigned long reportIntervalMs) {
  _report_interval = reportIntervalMs;
  _last_report = millis();
  _idle_ms = _cpu_ms = _wifi_ms = _sleep_ms = 0;
}

void pe_addCpuMs(unsigned long ms)   { _cpu_ms += ms; }
void pe_addWifiMs(unsigned long ms)  { _wifi_ms += ms; }
void pe_addIdleMs(unsigned long ms)  { _idle_ms += ms; }
void pe_addSleepMs(unsigned long ms) { _sleep_ms += ms; }
void pe_subtractIdleMs(unsigned long ms) {
  if (ms == 0) return;
  if (_idle_ms > ms) _idle_ms -= ms;
  else _idle_ms = 0;
}

// call this from loop() frequently (it will print every _report_interval)
void pe_tickAndMaybePrint() {
  unsigned long now = millis();
  if ((now - _last_report) < _report_interval) return;
  _last_report = now;

  unsigned long total = _idle_ms + _cpu_ms + _wifi_ms + _sleep_ms;
  if (total == 0) {
    Serial.println("[Power] no activity logged in interval");
    return;
  }

  float avg_mA = (
    (float)_idle_ms  * I_IDLE +
    (float)_cpu_ms   * I_CPU  +
    (float)_wifi_ms  * I_WIFI +
    (float)_sleep_ms * I_SLEEP
  ) / (float) total;

  // estimated energy in mWh during the reporting interval:
  float interval_h = (float)_report_interval / 3600000.0f;
  float est_mWh = avg_mA * 3.3f * interval_h;

  Serial.printf("[Power] avg=%.2f mA  est=%.4f mWh\n",
                avg_mA, est_mWh);
  Serial.flush();  // Ensure data is sent before reset

  // reset counters for next interval
  _idle_ms = _cpu_ms = _wifi_ms = _sleep_ms = 0;
}
