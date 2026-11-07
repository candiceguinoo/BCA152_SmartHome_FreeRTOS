#include <unity.h>
#include "debounce.h"

static debounce_t d;

void setUp(void)
{
    debounce_init(&d, 50, 0);
}
void tearDown(void) {}

void test_stable_press_is_accepted_once(void)
{
    bool pressed = false;
    // button held low from t=0 to t=200 ms, sampled every 10 ms
    for (uint32_t t = 0; t <= 200; t += 10) {
        if (debounce_update(&d, 0, t)) pressed = true;
    }
    TEST_ASSERT_TRUE(pressed);
}

void test_press_reported_only_one_time(void)
{
    int count = 0;
    for (uint32_t t = 0; t <= 500; t += 10) {
        if (debounce_update(&d, 0, t)) count++;
    }
    TEST_ASSERT_EQUAL(1, count);
}

void test_short_glitch_is_ignored(void)
{
    int count = 0;
    debounce_update(&d, 1, 0);
    // 20 ms low pulse, shorter than the 50 ms debounce
    if (debounce_update(&d, 0, 10)) count++;
    if (debounce_update(&d, 0, 20)) count++;
    if (debounce_update(&d, 1, 30)) count++;
    for (uint32_t t = 40; t <= 200; t += 10) {
        if (debounce_update(&d, 1, t)) count++;
    }
    TEST_ASSERT_EQUAL(0, count);
}

void test_bouncing_contacts_give_one_press(void)
{
    int count = 0;
    // contacts bounce low/high/low/high, then settle low
    int samples[] = {0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    uint32_t t = 0;
    for (unsigned i = 0; i < sizeof(samples) / sizeof(samples[0]); i++) {
        if (debounce_update(&d, samples[i], t)) count++;
        t += 10;
    }
    TEST_ASSERT_EQUAL(1, count);
}

void test_release_does_not_report_press(void)
{
    int count = 0;
    for (uint32_t t = 0; t <= 200; t += 10) {
        if (debounce_update(&d, 0, t)) count++;
    }
    // release and hold released
    for (uint32_t t = 210; t <= 500; t += 10) {
        if (debounce_update(&d, 1, t)) count++;
    }
    TEST_ASSERT_EQUAL(1, count);   // only the press, not the release
}

void test_second_press_after_release_is_detected(void)
{
    int count = 0;
    uint32_t t = 0;
    for (; t <= 200; t += 10) if (debounce_update(&d, 0, t)) count++;
    for (; t <= 400; t += 10) if (debounce_update(&d, 1, t)) count++;
    for (; t <= 600; t += 10) if (debounce_update(&d, 0, t)) count++;
    TEST_ASSERT_EQUAL(2, count);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_stable_press_is_accepted_once);
    RUN_TEST(test_press_reported_only_one_time);
    RUN_TEST(test_short_glitch_is_ignored);
    RUN_TEST(test_bouncing_contacts_give_one_press);
    RUN_TEST(test_release_does_not_report_press);
    RUN_TEST(test_second_press_after_release_is_detected);
    UNITY_END();
    return 0;
}