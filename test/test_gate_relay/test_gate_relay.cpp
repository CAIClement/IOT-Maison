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

void test_trigger_ferme_le_contact() {
    GateRelay relay = makeRelay();
    relay.begin();
    g_now = 5000;  // au-dela de la garde de boot

    const bool accepted = relay.trigger();

    TEST_ASSERT_TRUE(accepted);
    TEST_ASSERT_TRUE(g_closed);
    TEST_ASSERT_TRUE(relay.state() == GateRelay::State::Pulsing);
    TEST_ASSERT_TRUE(relay.isBusy());
}

void test_contact_reste_ferme_avant_la_fin_de_l_impulsion() {
    GateRelay relay = makeRelay();
    relay.begin();
    g_now = 5000;
    relay.trigger();

    g_now = 5399;  // 399 ms ecoulees sur 400
    relay.update();

    TEST_ASSERT_TRUE(g_closed);
    TEST_ASSERT_TRUE(relay.state() == GateRelay::State::Pulsing);
}

void test_contact_s_ouvre_a_la_fin_de_l_impulsion() {
    GateRelay relay = makeRelay();
    relay.begin();
    g_now = 5000;
    relay.trigger();

    g_now = 5400;  // pile 400 ms
    relay.update();

    TEST_ASSERT_FALSE(g_closed);
    TEST_ASSERT_TRUE(relay.state() == GateRelay::State::Lockout);
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_begin_ouvre_le_contact);
    RUN_TEST(test_trigger_ferme_le_contact);
    RUN_TEST(test_contact_reste_ferme_avant_la_fin_de_l_impulsion);
    RUN_TEST(test_contact_s_ouvre_a_la_fin_de_l_impulsion);
    return UNITY_END();
}
