#include <unity.h>
#include "state_machine.h"

void setUp(void) {}
void tearDown(void) {}

void test_init_to_normal_when_sensors_ready(void)
{
    TEST_ASSERT_EQUAL(STATE_NORMAL, sm_next_state(STATE_INIT, SM_EVT_SENSORS_READY));
}

void test_init_to_fault_on_sensor_fault(void)
{
    TEST_ASSERT_EQUAL(STATE_FAULT, sm_next_state(STATE_INIT, SM_EVT_SENSOR_FAULT));
}

void test_normal_to_warning_on_threshold(void)
{
    TEST_ASSERT_EQUAL(STATE_WARNING, sm_next_state(STATE_NORMAL, SM_EVT_THRESHOLD_EXCEEDED));
}

void test_normal_to_fault_on_sensor_fault(void)
{
    TEST_ASSERT_EQUAL(STATE_FAULT, sm_next_state(STATE_NORMAL, SM_EVT_SENSOR_FAULT));
}

void test_warning_to_normal_when_values_normal(void)
{
    TEST_ASSERT_EQUAL(STATE_NORMAL, sm_next_state(STATE_WARNING, SM_EVT_VALUES_NORMAL));
}

void test_warning_to_fault_on_sensor_fault(void)
{
    TEST_ASSERT_EQUAL(STATE_FAULT, sm_next_state(STATE_WARNING, SM_EVT_SENSOR_FAULT));
}

void test_fault_to_normal_on_recovery(void)
{
    TEST_ASSERT_EQUAL(STATE_NORMAL, sm_next_state(STATE_FAULT, SM_EVT_SENSOR_RECOVERED));
}

void test_invalid_events_are_ignored(void)
{
    TEST_ASSERT_EQUAL(STATE_INIT,    sm_next_state(STATE_INIT,    SM_EVT_THRESHOLD_EXCEEDED));
    TEST_ASSERT_EQUAL(STATE_NORMAL,  sm_next_state(STATE_NORMAL,  SM_EVT_SENSORS_READY));
    TEST_ASSERT_EQUAL(STATE_WARNING, sm_next_state(STATE_WARNING, SM_EVT_THRESHOLD_EXCEEDED));
    TEST_ASSERT_EQUAL(STATE_FAULT,   sm_next_state(STATE_FAULT,   SM_EVT_VALUES_NORMAL));
}

void test_state_names(void)
{
    TEST_ASSERT_EQUAL_STRING("INIT",    sm_state_name(STATE_INIT));
    TEST_ASSERT_EQUAL_STRING("NORMAL",  sm_state_name(STATE_NORMAL));
    TEST_ASSERT_EQUAL_STRING("WARNING", sm_state_name(STATE_WARNING));
    TEST_ASSERT_EQUAL_STRING("FAULT",   sm_state_name(STATE_FAULT));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_init_to_normal_when_sensors_ready);
    RUN_TEST(test_init_to_fault_on_sensor_fault);
    RUN_TEST(test_normal_to_warning_on_threshold);
    RUN_TEST(test_normal_to_fault_on_sensor_fault);
    RUN_TEST(test_warning_to_normal_when_values_normal);
    RUN_TEST(test_warning_to_fault_on_sensor_fault);
    RUN_TEST(test_fault_to_normal_on_recovery);
    RUN_TEST(test_invalid_events_are_ignored);
    RUN_TEST(test_state_names);
    return UNITY_END();
}