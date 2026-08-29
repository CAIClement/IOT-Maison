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

void test_trigger_refuse_pendant_l_impulsion() {
    GateRelay relay = makeRelay();
    relay.begin();
    g_now = 5000;
    relay.trigger();
    const int writesAvant = g_writeCount;

    g_now = 5200;
    const bool accepted = relay.trigger();

    TEST_ASSERT_FALSE(accepted);
    TEST_ASSERT_EQUAL(writesAvant, g_writeCount);
    TEST_ASSERT_TRUE(relay.state() == GateRelay::State::Pulsing);
}

void test_trigger_refuse_pendant_le_verrou() {
    GateRelay relay = makeRelay();
    relay.begin();
    g_now = 5000;
    relay.trigger();
    g_now = 5400;
    relay.update();  // -> Lockout

    g_now = 6000;
    const bool accepted = relay.trigger();

    TEST_ASSERT_FALSE(accepted);
    TEST_ASSERT_FALSE(g_closed);
    TEST_ASSERT_TRUE(relay.state() == GateRelay::State::Lockout);
}

void test_retour_au_repos_a_la_fin_du_verrou() {
    GateRelay relay = makeRelay();
    relay.begin();
    g_now = 5000;
    relay.trigger();
    g_now = 5400;
    relay.update();  // -> Lockout

    g_now = 7399;  // 1999 ms de verrou ecoulees sur 2000
    relay.update();
    TEST_ASSERT_TRUE(relay.state() == GateRelay::State::Lockout);

    g_now = 7400;  // 2000 ms de verrou ecoulees
    relay.update();

    TEST_ASSERT_TRUE(relay.state() == GateRelay::State::Idle);
    TEST_ASSERT_FALSE(g_closed);
}

void test_trigger_accepte_apres_un_cycle_complet() {
    GateRelay relay = makeRelay();
    relay.begin();
    g_now = 5000;
    relay.trigger();
    g_now = 5400;
    relay.update();
    g_now = 7400;
    relay.update();  // -> Idle

    const bool accepted = relay.trigger();

    TEST_ASSERT_TRUE(accepted);
    TEST_ASSERT_TRUE(g_closed);
    TEST_ASSERT_TRUE(relay.state() == GateRelay::State::Pulsing);
}

void test_trigger_refuse_pendant_la_garde_de_boot() {
    GateRelay relay = makeRelay();
    relay.begin();
    g_now = 2999;  // garde de boot = 3000 ms

    const bool accepted = relay.trigger();

    TEST_ASSERT_FALSE(accepted);
    TEST_ASSERT_FALSE(g_closed);
    TEST_ASSERT_TRUE(relay.state() == GateRelay::State::Idle);
}

void test_trigger_accepte_a_la_fin_de_la_garde_de_boot() {
    GateRelay relay = makeRelay();
    relay.begin();
    g_now = 3000;

    const bool accepted = relay.trigger();

    TEST_ASSERT_TRUE(accepted);
    TEST_ASSERT_TRUE(g_closed);
}

// millis() repasse a 0 apres environ 49 jours. Les comparaisons de duree
// doivent utiliser une soustraction non signee, jamais "date_de_fin <= now".
void test_impulsion_robuste_au_rollover_de_millis() {
    GateRelay relay = makeRelay();
    relay.begin();
    g_now = 0xFFFFFF00;  // 256 ms avant le rollover
    relay.trigger();
    TEST_ASSERT_TRUE(g_closed);

    g_now = 0xFFFFFF50;  // 80 ms seulement apres le declenchement
    relay.update();
    TEST_ASSERT_TRUE(g_closed);
    TEST_ASSERT_TRUE(relay.state() == GateRelay::State::Pulsing);

    g_now = 0x00000090;  // 256 + 144 = 400 ms plus tard, apres le rollover
    relay.update();

    TEST_ASSERT_FALSE(g_closed);
    TEST_ASSERT_TRUE(relay.state() == GateRelay::State::Lockout);
}

// update() peut etre appele en retard si la boucle principale bloque. Le contact
// reste alors ferme plus longtemps que pulseMs -- c'est une borne basse, pas une
// borne haute. Mais le verrou qui suit doit durer 2000 ms pleines A PARTIR de
// l'appel tardif, jamais moins.
void test_verrou_complet_apres_un_update_tardif() {
    GateRelay relay = makeRelay();
    relay.begin();
    g_now = 5000;
    relay.trigger();

    g_now = 20000;  // 15 s de retard : la boucle a bloque
    relay.update();
    TEST_ASSERT_FALSE(g_closed);
    TEST_ASSERT_TRUE(relay.state() == GateRelay::State::Lockout);

    g_now = 21999;
    relay.update();
    TEST_ASSERT_TRUE(relay.state() == GateRelay::State::Lockout);

    g_now = 22000;
    relay.update();
    TEST_ASSERT_TRUE(relay.state() == GateRelay::State::Idle);
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_begin_ouvre_le_contact);
    RUN_TEST(test_trigger_ferme_le_contact);
    RUN_TEST(test_contact_reste_ferme_avant_la_fin_de_l_impulsion);
    RUN_TEST(test_contact_s_ouvre_a_la_fin_de_l_impulsion);
    RUN_TEST(test_trigger_refuse_pendant_l_impulsion);
    RUN_TEST(test_trigger_refuse_pendant_le_verrou);
    RUN_TEST(test_retour_au_repos_a_la_fin_du_verrou);
    RUN_TEST(test_trigger_accepte_apres_un_cycle_complet);
    RUN_TEST(test_trigger_refuse_pendant_la_garde_de_boot);
    RUN_TEST(test_trigger_accepte_a_la_fin_de_la_garde_de_boot);
    RUN_TEST(test_impulsion_robuste_au_rollover_de_millis);
    RUN_TEST(test_verrou_complet_apres_un_update_tardif);
    return UNITY_END();
}
