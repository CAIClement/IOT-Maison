#pragma once

#include <stdint.h>

/**
 * Pilote d'un module relais a declenchement sur niveau haut.
 *
 * Ne connait ni Arduino, ni le WiFi, ni Blynk : la sortie et l'horloge sont
 * injectees, ce qui rend la machine a etats testable sur PC.
 *
 * write(true)  = contact ferme  (le portail recoit l'impulsion)
 * write(false) = contact ouvert (etat de repos)
 */
class GateRelay {
public:
    using WriteFn = void (*)(bool closed);
    using ClockFn = uint32_t (*)();

    enum class State { Idle, Pulsing, Lockout };

    struct Config {
        uint32_t pulseMs;      // duree de fermeture du contact
        uint32_t lockoutMs;    // delai mort apres l'impulsion
        uint32_t bootGuardMs;  // aucun declenchement avant cette date
    };

    GateRelay(WriteFn write, ClockFn clock, const Config& config);

    /** Met le contact au repos. A appeler une fois au demarrage. */
    void begin();

    /**
     * Demande une impulsion.
     * @return true si acceptee, false si refusee (occupe, verrou, garde de boot).
     */
    bool trigger();

    /** Fait avancer la machine a etats. A appeler a chaque tour de boucle. */
    void update();

    State state() const { return state_; }
    bool isBusy() const { return state_ != State::Idle; }

private:
    void enter(State next, bool closed);

    WriteFn write_;
    ClockFn clock_;
    Config config_;
    State state_;
    uint32_t stateEnteredAt_;
};
