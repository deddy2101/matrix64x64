#ifndef NOTIFIER_H
#define NOTIFIER_H

#include <Arduino.h>

/**
 * Notifier - Invio di notifiche a tutti i client connessi
 * (implementato dal trasporto, es. BleManager).
 * Da chiamare solo dal loop().
 */
class Notifier {
public:
    virtual ~Notifier() {}
    virtual void broadcast(const String& message) = 0;

    // Cambia il PIN statico di accesso (6 cifre). false = non supportato/non valido.
    virtual bool setPairingPin(const String& pin) { return false; }
};

#endif // NOTIFIER_H
