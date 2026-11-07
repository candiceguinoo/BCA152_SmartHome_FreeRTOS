#include <unity.h>
#include "light_map.h"

void setUp(void) {}
void tearDown(void) {}

void test_zero_maps_to_zero(void)
{
    TEST_ASSERT_EQUAL(0, light_map_percent(0, 0));
}

void test_full_scale_maps_to_100(void)
{
    TEST_ASSERT_EQUAL(100, light_map_percent(4095, 0));
}

void test_mid_value(void)
{
    TEST_ASSERT_EQUAL(50, light_map_percent(2048, 0));
}

void test_real_ambient_value(void)
{
    // from my hardware log: raw=847 gave intensity 20
    TEST_ASSERT_EQUAL(20, light_map_percent(847, 0));
}

void test_inverted_flips_result(void)
{
    TEST_ASSERT_EQUAL(100, light_map_percent(0, 1));
    TEST_ASSERT_EQUAL(0,   light_map_percent(4095, 1));
}

void test_out_of_range_is_clamped(void)
{
    TEST_ASSERT_EQUAL(0,   light_map_percent(-5, 0));
    TEST_ASSERT_EQUAL(100, light_map_percent(5000, 0));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_zero_maps_to_zero);
    RUN_TEST(test_full_scale_maps_to_100);
    RUN_TEST(test_mid_value);
    RUN_TEST(test_real_ambient_value);
    RUN_TEST(test_inverted_flips_result);
    RUN_TEST(test_out_of_range_is_clamped);
    return UNITY_END();
}