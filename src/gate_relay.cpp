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
    return false;
}

void GateRelay::update() {
}

void GateRelay::enter(State next, bool closed) {
    state_ = next;
    stateEnteredAt_ = clock_();
    write_(closed);
}
