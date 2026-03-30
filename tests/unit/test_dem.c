#include "unity.h"
#include "Dem.h"
#include "Dem_EventConfig.h"
#include <string.h>

void setUp(void)    { Dem_Init(); }
void tearDown(void) { Dem_ClearDTC(DEM_DTC_GROUP_ALL); }

/* Test 1: DEM initializes correctly */
void test_Dem_Init_SetsInitializedState(void)
{
    Dem_EventStatusType status = 0xFFU;
    Std_ReturnType ret = Dem_GetEventStatus(RAIL_EVT_BRAKE_SENSORLOSS, &status);
    TEST_ASSERT_EQUAL(E_OK, ret);
    TEST_ASSERT_EQUAL(0x00U, status);
}

/* Test 2: SetEventStatus FAILED sets testFailed bit */
void test_Dem_SetEventStatus_Failed_SetsTFBit(void)
{
    Dem_EventStatusType status = 0U;
    Dem_SetEventStatus(RAIL_EVT_BRAKE_SENSORLOSS, DEM_EVENT_STATUS_FAILED);
    Dem_GetEventStatus(RAIL_EVT_BRAKE_SENSORLOSS, &status);
    TEST_ASSERT_BITS(DEM_UDS_STATUS_TF, DEM_UDS_STATUS_TF, status);
}

/* Test 3: SetEventStatus PASSED clears testFailed bit */
void test_Dem_SetEventStatus_Passed_ClearsTFBit(void)
{
    Dem_EventStatusType status = 0U;
    Dem_SetEventStatus(RAIL_EVT_BRAKE_SENSORLOSS, DEM_EVENT_STATUS_FAILED);
    Dem_SetEventStatus(RAIL_EVT_BRAKE_SENSORLOSS, DEM_EVENT_STATUS_PASSED);
    Dem_GetEventStatus(RAIL_EVT_BRAKE_SENSORLOSS, &status);
    TEST_ASSERT_BITS(DEM_UDS_STATUS_TF, 0U, status);
}

/* Test 4: Invalid EventId returns E_NOT_OK */
void test_Dem_SetEventStatus_InvalidId_ReturnsError(void)
{
    Std_ReturnType ret = Dem_SetEventStatus(0x00U, DEM_EVENT_STATUS_FAILED);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
}

/* Test 5: ClearDTC clears all events */
void test_Dem_ClearDTC_All_ClearsAllEvents(void)
{
    Dem_EventStatusType status = 0U;
    uint16_t count = 0U;
    Dem_FilterType filter;

    Dem_SetEventStatus(RAIL_EVT_BRAKE_SENSORLOSS,  DEM_EVENT_STATUS_FAILED);
    Dem_SetEventStatus(RAIL_EVT_DOOR_LOCKFAIL,     DEM_EVENT_STATUS_FAILED);
    Dem_SetEventStatus(RAIL_EVT_MOTOR_OVERCURRENT, DEM_EVENT_STATUS_FAILED);

    Dem_ClearDTC(DEM_DTC_GROUP_ALL);

    Dem_SetDTCFilter(DEM_UDS_STATUS_TF, &filter);
    Dem_GetNumberOfFilteredDTC(&filter, &count);
    TEST_ASSERT_EQUAL(0U, count);

    Dem_GetEventStatus(RAIL_EVT_BRAKE_SENSORLOSS, &status);
    TEST_ASSERT_EQUAL(0x00U, status);
}

/* Test 6: DTC filter returns correct count */
void test_Dem_Filter_ReturnsCorrectDTCCount(void)
{
    uint16_t count = 0U;
    Dem_FilterType filter;

    Dem_SetEventStatus(RAIL_EVT_BRAKE_SENSORLOSS,  DEM_EVENT_STATUS_FAILED);
    Dem_SetEventStatus(RAIL_EVT_DOOR_LOCKFAIL,     DEM_EVENT_STATUS_FAILED);
    Dem_SetEventStatus(RAIL_EVT_MOTOR_OVERCURRENT, DEM_EVENT_STATUS_FAILED);

    Dem_SetDTCFilter(DEM_UDS_STATUS_TF, &filter);
    Dem_GetNumberOfFilteredDTC(&filter, &count);
    TEST_ASSERT_EQUAL(3U, count);
}

/* Test 7: Counter debounce - PREFAILED not stored until threshold */
void test_Dem_Debounce_Counter_NotFailedUntilThreshold(void)
{
    uint16_t count = 0U;
    Dem_FilterType filter;

    /* Send PREFAILED once - should NOT reach FAILED yet */
    Dem_SetEventStatus(RAIL_EVT_BRAKE_SENSORLOSS, DEM_EVENT_STATUS_PREFAILED);

    Dem_SetDTCFilter(DEM_UDS_STATUS_TF, &filter);
    Dem_GetNumberOfFilteredDTC(&filter, &count);
    TEST_ASSERT_EQUAL(0U, count);
}

/* Test 8: Counter debounce - FAILED after threshold */
void test_Dem_Debounce_Counter_FailedAfterThreshold(void)
{
    uint16_t count = 0U;
    Dem_FilterType filter;
    uint8_t i;

    /* Send PREFAILED DEM_DEBOUNCE_COUNTER_THRESHOLD times */
    for (i = 0U; i < DEM_DEBOUNCE_COUNTER_THRESHOLD; i++)
        Dem_SetEventStatus(RAIL_EVT_BRAKE_SENSORLOSS, DEM_EVENT_STATUS_PREFAILED);

    Dem_SetDTCFilter(DEM_UDS_STATUS_TF, &filter);
    Dem_GetNumberOfFilteredDTC(&filter, &count);
    TEST_ASSERT_EQUAL(1U, count);
}

/* Test 9: GetNextFilteredDTC iterates correctly */
void test_Dem_GetNextFilteredDTC_IteratesAll(void)
{
    Dem_FilterType filter;
    Dem_DTCType    dtc;
    uint8_t        dtc_status;
    uint8_t        found = 0U;

    Dem_SetEventStatus(RAIL_EVT_BRAKE_SENSORLOSS, DEM_EVENT_STATUS_FAILED);
    Dem_SetEventStatus(RAIL_EVT_DOOR_LOCKFAIL,    DEM_EVENT_STATUS_FAILED);

    Dem_SetDTCFilter(DEM_UDS_STATUS_TF, &filter);
    while (Dem_GetNextFilteredDTC(&filter, &dtc, &dtc_status) == E_OK)
        found++;

    TEST_ASSERT_EQUAL(2U, found);
}

/* Test 10: NULL pointer returns E_NOT_OK */
void test_Dem_GetEventStatus_NullPtr_ReturnsError(void)
{
    Std_ReturnType ret = Dem_GetEventStatus(RAIL_EVT_BRAKE_SENSORLOSS, NULL_PTR);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_Dem_Init_SetsInitializedState);
    RUN_TEST(test_Dem_SetEventStatus_Failed_SetsTFBit);
    RUN_TEST(test_Dem_SetEventStatus_Passed_ClearsTFBit);
    RUN_TEST(test_Dem_SetEventStatus_InvalidId_ReturnsError);
    RUN_TEST(test_Dem_ClearDTC_All_ClearsAllEvents);
    RUN_TEST(test_Dem_Filter_ReturnsCorrectDTCCount);
    RUN_TEST(test_Dem_Debounce_Counter_NotFailedUntilThreshold);
    RUN_TEST(test_Dem_Debounce_Counter_FailedAfterThreshold);
    RUN_TEST(test_Dem_GetNextFilteredDTC_IteratesAll);
    RUN_TEST(test_Dem_GetEventStatus_NullPtr_ReturnsError);
    return UNITY_END();
}