#include "debug_utils.h"

// Global flag to prevent concurrent Serial writes
bool _serial_busy = false;
