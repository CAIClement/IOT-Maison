#include "status_led.h"

namespace {
constexpr uint32_t kBlinkHalfPeriodMs = 500;
}

bool statusLedLevel(LinkState link, uint32_t nowMs) {
    switch (link) {
    case LinkState::Online:
        return true;
    case LinkState::WifiOnly:
        return (nowMs / kBlinkHalfPeriodMs) % 2 == 0;
    case LinkState::Offline:
    default:
        return false;
    }
}
