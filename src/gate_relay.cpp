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
