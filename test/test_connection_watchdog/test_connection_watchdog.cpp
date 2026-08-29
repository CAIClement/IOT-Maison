#include <unity.h>

#include "connection_watchdog.h"

namespace {
const uint32_t kTimeoutMs = 300000;  // 5 min
}

void setUp() {}
void tearDown() {}

void test_pas_de_redemarrage_tant_que_connecte() {
    ConnectionWatchdog wd(kTimeoutMs);
    wd.begin(0);

    wd.update(true, 1000000);

    TEST_ASSERT_FALSE(wd.shouldRestart());
}

void test_pas_de_redemarrage_avant_le_delai() {
    ConnectionWatchdog wd(kTimeoutMs);
    wd.begin(0);

    wd.update(false, 299999);

    TEST_ASSERT_FALSE(wd.shouldRestart());
}

void test_redemarrage_au_dela_du_delai() {
    ConnectionWatchdog wd(kTimeoutMs);
    wd.begin(0);

    wd.update(false, 300000);

    TEST_ASSERT_TRUE(wd.shouldRestart());
}

void test_une_reconnexion_rearme_le_watchdog() {
    ConnectionWatchdog wd(kTimeoutMs);
    wd.begin(0);
    wd.update(false, 200000);
    wd.update(true, 250000);  // reconnexion

    wd.update(false, 500000);  // 250 s seulement depuis la reconnexion

    TEST_ASSERT_FALSE(wd.shouldRestart());
}

void test_robuste_au_rollover_de_millis() {
    ConnectionWatchdog wd(kTimeoutMs);
    wd.begin(0xFFFF0000);

    wd.update(false, 0x0003FFFF);  // 0x4FFFF ms ecoulees = 327 679 ms > 300 000

    TEST_ASSERT_TRUE(wd.shouldRestart());
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_pas_de_redemarrage_tant_que_connecte);
    RUN_TEST(test_pas_de_redemarrage_avant_le_delai);
    RUN_TEST(test_redemarrage_au_dela_du_delai);
    RUN_TEST(test_une_reconnexion_rearme_le_watchdog);
    RUN_TEST(test_robuste_au_rollover_de_millis);
    return UNITY_END();
}
