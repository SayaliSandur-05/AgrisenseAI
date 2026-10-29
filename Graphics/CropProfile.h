#ifndef CROPPROFILE_H
#define CROPPROFILE_H

#include <cstring>
#include <cstdio>
#include <string>
#include <vector>

using namespace std;

struct CropProfile {
    string name;
    float idealTempMin, idealTempMax;
    float idealHumidityMin, idealHumidityMax;
    float idealSoilMoistureMin, idealSoilMoistureMax;
    string sowingSeason;
};

inline vector<CropProfile> cropProfiles;

inline const CropProfile* findCropProfile(const char* cropName) {
    if (!cropName || cropName[0] == '\0') return nullptr;
    for (size_t i = 0; i < cropProfiles.size(); ++i) {
        if (cropProfiles[i].name == cropName) return &cropProfiles[i];
    }
    return nullptr;
}
#endif