#ifndef INFCLASSR_GEOLOCATION_H
#define INFCLASSR_GEOLOCATION_H

#include "GeoLite2PP/GeoLite2PP.hpp"
#include <iostream>

class Geolocation {
private:
    GeoLite2PP::DB *db;
    int get_iso_numeric_code(GeoLite2PP::MStr& m);
public:
    Geolocation(const char* path_to_mmdb);
    ~Geolocation();
    int get_country_iso_numeric_code(std::string& ip);
};

#endif