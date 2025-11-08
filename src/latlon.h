#ifndef _LATLON_H
#define _LATLON_H
#include <cmath>

double latlon2meters(double lat1, double lon1, double lat2, double lon2) {
    double R = 6378.137; // Radius of earth in KM
    double dLat = lat2 * M_PI / 180.0 - lat1 * M_PI / 180.0;
    double dLon = lon2 * M_PI / 180.0 - lon1 * M_PI / 180.0;
    double a = sin(dLat/2.0) * sin(dLat/2.0) +
    cos(lat1 * M_PI / 180.0) * cos(lat2 * M_PI / 180.0) *
    sin(dLon/2.0) * sin(dLon/2.0);
    double c = 2.0 * atan2(sqrt(a), sqrt(1.0-a));
    double d = R * c;
    return d * 1000.0; // meters
}

#endif