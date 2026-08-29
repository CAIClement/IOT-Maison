#include <unity.h>

#include "status_led.h"

void setUp() {}
void tearDown() {}

void test_eteinte_hors_ligne() {
    TEST_ASSERT_FALSE(statusLedLevel(LinkState::Offline, 0));
    TEST_ASSERT_FALSE(statusLedLevel(LinkState::Offline, 750));
    TEST_ASSERT_FALSE(statusLedLevel(LinkState::Offline, 1234567));
}

void test_allumee_fixe_quand_tout_est_connecte() {
    TEST_ASSERT_TRUE(statusLedLevel(LinkState::Online, 0));
    TEST_ASSERT_TRUE(statusLedLevel(LinkState::Online, 750));
    TEST_ASSERT_TRUE(statusLedLevel(LinkState::Online, 1234567));
}

void test_clignote_quand_wifi_seul() {
    // Periode de 1 s : 500 ms allumee, 500 ms eteinte.
    TEST_ASSERT_TRUE(statusLedLevel(LinkState::WifiOnly, 0));
    TEST_ASSERT_TRUE(statusLedLevel(LinkState::WifiOnly, 499));
    TEST_ASSERT_FALSE(statusLedLevel(LinkState::WifiOnly, 500));
    TEST_ASSERT_FALSE(statusLedLevel(LinkState::WifiOnly, 999));
    TEST_ASSERT_TRUE(statusLedLevel(LinkState::WifiOnly, 1000));
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_eteinte_hors_ligne);
    RUN_TEST(test_allumee_fixe_quand_tout_est_connecte);
    RUN_TEST(test_clignote_quand_wifi_seul);
    return UNITY_END();
}
