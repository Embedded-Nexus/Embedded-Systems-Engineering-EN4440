#include "request_sim.h"
#include "debug_utils.h"

//  Define the global instance
RequestSIM requestSim;
unsigned long pollingInterval = 5000;  // default 30 seconds

// Optional helper implementation
void RequestSIM::clear() {
    for (int i = 0; i < NUM_REGISTERS; i++) {
        read[i] = false;
        write[i] = false;
        writeData[i] = 0;
    }
}

// ✅ New function to print current config
void printGlobalRequestSim() {
    DEBUG_PRINTLN("[GlobalConfig] 📋 Current RequestSIM State:");

    for (int i = 0; i < NUM_REGISTERS; i++) {
        DEBUG_PRINTF("  Reg[%02d] => READ:%d  WRITE:%d  VALUE:%u\n",
                     i, requestSim.read[i], requestSim.write[i], requestSim.writeData[i]);
    }

    DEBUG_PRINTLN("[GlobalConfig] -----------------------------");
}
