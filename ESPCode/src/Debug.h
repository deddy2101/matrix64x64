#ifndef DEBUG_H
#define DEBUG_H

#include <Arduino.h>

// ═══════════════════════════════════════════════════════════════
// DEBUG CONFIGURATION
// Commenta la riga sotto per DISABILITARE tutto il debug
// e risparmiare ~5-10KB di Flash
// ═══════════════════════════════════════════════════════════════
#define DEBUG_ENABLED

#ifdef DEBUG_ENABLED
  #define DEBUG_INIT(baud) Serial.begin(baud)
  #define DEBUG_PRINT(x) Serial.print(x)
  #define DEBUG_PRINTLN(x) Serial.println(x)
  #define DEBUG_PRINTF(...) Serial.printf(__VA_ARGS__)
#else
  #define DEBUG_INIT(baud)
  #define DEBUG_PRINT(x)
  #define DEBUG_PRINTLN(x)
  #define DEBUG_PRINTF(...)
#endif

#endif // DEBUG_H