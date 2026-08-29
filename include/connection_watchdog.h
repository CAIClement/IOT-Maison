#pragma once

#include <stdint.h>

/**
 * Surveille la duree de deconnexion et demande un redemarrage au-dela d'un
 * seuil. Compense les blocages de pile WiFi observes en limite de portee.
 *
 * Fonction pure du temps et d'un booleen : aucune dependance materielle.
 */
class ConnectionWatchdog {
public:
    explicit ConnectionWatchdog(uint32_t timeoutMs);

    /** Arme le watchdog. A appeler une fois au demarrage. */
    void begin(uint32_t nowMs);

    /** A appeler a chaque tour de boucle avec l'etat reel de la connexion. */
    void update(bool connected, uint32_t nowMs);

    bool shouldRestart() const { return expired_; }

private:
    uint32_t timeoutMs_;
    uint32_t lastConnectedAt_;
    bool expired_;
};
