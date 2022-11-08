#include "gps_utils.hh"
#include <math.h>

#define SQ(a) ((a) * (a))

const float kEarthEquatorialRadius = 6378137; // [m] Earth's equatorial radius.
const float kEarthPolarRadius = 6356752; // [m] Earth's polar radius.
const float kEarthMeanRadius = 6372797.560856f; // [m] Earth's quadratic mean radius for WS-84.

/**
 * @brief Returns the radius from the center of the earth to a point on the surface of the earth
 * at a given latitude, in radians, assuming that the earth is an ellipsoid shape.
 * @param[in] lat Latitude, in radians.
 * @retval Radius in meters.
 */
float EarthEllipsoidRadius(float lat) {
    // sqr((sq(kEarthEquatorialRadius*kEarthEquatorialRadius*cos(lat))+sq(b*b*sin(lat)))/(sq(kEarthEquatorialRadius*cos(lat))+sq(b*sin(lat))));
    return sqrt(
        (
            SQ(SQ(kEarthEquatorialRadius) * cos(lat)) + 
            SQ(SQ(kEarthPolarRadius) * sin(lat))
        ) / 
        (
            SQ(kEarthEquatorialRadius*cos(lat)) + 
            SQ(kEarthPolarRadius*sin(lat))
        )
    );
}

/**
 * @brief Converts a value from degrees to radians.
 * @param[in] degrees Parameter to convert, in degrees.
 * @retval Result in radians.
 */
float DegreesToRadians(float degrees) {
    return degrees * M_PI / 180.0f;
}

/**
 * @brief Calculates the straight line distance through the earth for two lat/long coordinate sets.
 * @param[in] lat_a Latitude of coordinate A, in degrees. + for N, - for S.
 * @param[in] lon_a Longitude of coordinate A, in degrees. + for E, - for W.
 * @param[in] lat_b Latitude of coordinate B, in degrees. + for N, - for S.
 * @param[in] lon_b Longitude of coordinate B, in degrees. + for E, - for W.
 * @retval Straight line distance between coordinates A and B.
 */
float CalculateStraightLineDistance(float lat_a, float lon_a, float lat_b, float lon_b) {
    // Adapted from this stackoverflow answer, which uses an oblate spheroid assumption but only calculates
    // straight-line distances: https://stackoverflow.com/a/49916544/4625627.
    float lat_a_rad = DegreesToRadians(lat_a);
    float lon_a_rad = DegreesToRadians(lon_a);
    float lat_b_rad = DegreesToRadians(lat_b);
    float lon_b_rad = DegreesToRadians(lon_b);

    float radius_a = EarthEllipsoidRadius(lat_a_rad);
    float x_a = radius_a * cos(lat_a_rad) * cos(lon_a_rad);
    float y_a = radius_a * cos(lat_a_rad) * sin(lon_a_rad);
    float z_a = radius_a * sin(lat_a_rad);

    float radius_b = EarthEllipsoidRadius(lat_b_rad);
    float x_b = radius_b * cos(lat_b_rad) * cos(lon_b_rad);
    float y_b = radius_b * cos(lat_b_rad) * sin(lon_b_rad);
    float z_b = radius_b * sin(lat_b_rad);

    return sqrt(SQ(x_a-x_b) + SQ(y_a-y_b) + SQ(z_a-z_b)); // TODO: make this return surface distance
}

/**
 * @brief Calculates the haversine of an angle, in radians.
 * @param[in] theta Angle in radians.
 * @retval Haversine of theta.
 */
float hav(float theta) {
    return SQ(sin(theta * 0.5f));
}

/**
 * @brief Calculates the great circle distance along the surface of the earth between two lat/long coordinate sets.
 * @param[in] lat_a Latitude of coordinate A, in degrees. + for N, - for S.
 * @param[in] lon_a Longitude of coordinate A, in degrees. + for E, - for W.
 * @param[in] lat_b Latitude of coordinate B, in degrees. + for N, - for S.
 * @param[in] lon_b Longitude of coordinate B, in degrees. + for E, - for W.
 * @retval Great circle distance between points A and B.
 */
float CalculateGeoidalDistance(float lat_a, float lon_a, float lat_b, float lon_b) {
    float lat_a_rad = DegreesToRadians(lat_a);
    float lon_a_rad = DegreesToRadians(lon_a);
    float lat_b_rad = DegreesToRadians(lat_b);
    float lon_b_rad = DegreesToRadians(lon_b);

    return 2*kEarthMeanRadius * 
        asin(
            sqrt(
                hav(lat_b_rad-lat_a_rad) +
                (1 - hav(lat_a_rad-lat_b_rad) - hav(lat_a_rad+lat_b_rad)) * hav(lon_b_rad - lon_a_rad)
            )
        );
}
