#ifndef VERSION_H
#define VERSION_H

// Firmware version
#define FIRMWARE_VERSION "1.5.1"
#define FIRMWARE_BUILD_NUMBER "19"  // Incrementa manualmente ad ogni build
#define FIRMWARE_BUILD_DATE __DATE__
#define FIRMWARE_BUILD_TIME __TIME__

// Stringa versione completa: "1.0.0 (build 1)"
#define FIRMWARE_FULL_VERSION FIRMWARE_VERSION " (build " FIRMWARE_BUILD_NUMBER ")"

#endif // VERSION_H
