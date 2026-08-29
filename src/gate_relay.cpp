#include "gate_relay.h"

GateRelay::GateRelay(WriteFn write, ClockFn clock, const Config& config)
    : write_(write),
      clock_(clock),
      config_(config),
      state_(State::Idle),
      stateEnteredAt_(0) {}

void GateRelay::begin() {
    state_ = State::Idle;
    stateEnteredAt_ = clock_();
    write_(false);
}

bool GateRelay::trigger() {
    if (state_ != State::Idle) return false;
    // Garde de demarrage : on refuse toute commande tant que la carte vient de
    // booter. Redondant avec la resistance de tirage sur GPIO18, volontairement.
    // Se re-arme 3 s tous les ~49 jours au rollover de millis() : sans importance.
    if (clock_() < config_.bootGuardMs) return false;
    enter(State::Pulsing, true);
    return true;
}

void GateRelay::update() {
    const uint32_t elapsed = clock_() - stateEnteredAt_;

    switch (state_) {
    case State::Pulsing:
        if (elapsed >= config_.pulseMs) enter(State::Lockout, false);
        break;
    case State::Lockout:
        if (elapsed >= config_.lockoutMs) enter(State::Idle, false);
        break;
    case State::Idle:
        break;
    }
}

void GateRelay::enter(State next, bool closed) {
    state_ = next;
    stateEnteredAt_ = clock_();
    write_(closed);
}
