#include <unity.h>

#include "gate_relay.h"

namespace {

uint32_t g_now = 0;
bool g_closed = false;
int g_writeCount = 0;

uint32_t fakeClock() { return g_now; }

void fakeWrite(bool closed) {
    g_closed = closed;
    g_writeCount++;
}

// Memes valeurs que config.h, en dur ici pour que le test reste independant.
const GateRelay::Config kConfig{400, 2000, 3000};

GateRelay makeRelay() { return GateRelay(fakeWrite, fakeClock, kConfig); }

}  // namespace

void setUp() {
    g_now = 0;
    g_closed = true;  // valeur volontairement fausse : begin() doit la corriger
    g_writeCount = 0;
}

void tearDown() {}

void test_begin_ouvre_le_contact() {
    GateRelay relay = makeRelay();

    relay.begin();

    TEST_ASSERT_FALSE(g_closed);
    TEST_ASSERT_EQUAL(1, g_writeCount);
    TEST_ASSERT_TRUE(relay.state() == GateRelay::State::Idle);
    TEST_ASSERT_FALSE(relay.isBusy());
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_begin_ouvre_le_contact);
    return UNITY_END();
}
