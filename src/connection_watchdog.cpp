#include "connection_watchdog.h"

ConnectionWatchdog::ConnectionWatchdog(uint32_t timeoutMs)
    : timeoutMs_(timeoutMs), lastConnectedAt_(0), expired_(false) {}

void ConnectionWatchdog::begin(uint32_t nowMs) {
    lastConnectedAt_ = nowMs;
    expired_ = false;
}

void ConnectionWatchdog::update(bool connected, uint32_t nowMs) {
    if (connected) {
        lastConnectedAt_ = nowMs;
        expired_ = false;
        return;
    }
    // Soustraction non signee : robuste au rollover de millis().
    if (nowMs - lastConnectedAt_ >= timeoutMs_) {
        expired_ = true;
    }
}
