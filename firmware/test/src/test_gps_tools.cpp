#include "gtest/gtest.h"
#include "gps_utils.hh"

#define FLOAT_RELTOL 0.0005

TEST(GPSTools, CalculateGeoidalDistance) {
    // Cross checking against this haversine calcualtor: https://www.movable-type.co.uk/scripts/latlong.html
    // Start with small hops
    EXPECT_NEAR(CalculateGeoidalDistance(20.35f, -13.78f, 19.35f, -13.78f), 111200, 111200*FLOAT_RELTOL); // -lat only
    EXPECT_NEAR(CalculateGeoidalDistance(19.35f, -13.78f, 20.35f, -13.78f), 111200, 111200*FLOAT_RELTOL); // +lat only
    EXPECT_NEAR(CalculateGeoidalDistance(19.35f, -13.78f, 19.35f, -14.78f), 104900, 104900*FLOAT_RELTOL); // -long only
    EXPECT_NEAR(CalculateGeoidalDistance(19.35f, -13.78f, 19.35f, -12.78f), 104900, 104900*FLOAT_RELTOL); // +long only
    
    // Bigger hop
    EXPECT_NEAR(CalculateGeoidalDistance(20.35f, -13.78f, 320.2f, -50.99f), 7722e3f, 7722e3f*FLOAT_RELTOL);
}