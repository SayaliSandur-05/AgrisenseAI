// =============================================================================
// CropProfile.h  --  AgriSense AI  --  PSOOP (data-driven design)
// -----------------------------------------------------------------------------
// One row per crop that the CSV picker can select.
// Ranges = CSV average +/- 15% so auto-filled values pass threshold check.
// Soil-moisture range is centered on rainfall/3 (the value the picker writes).
// =============================================================================
#ifndef CROPPROFILE_H
#define CROPPROFILE_H

#include <cstring>
#include <cstdio>

struct CropProfile {
    const char* name;
    float idealTempMin, idealTempMax;
    float idealHumidityMin, idealHumidityMax;
    float idealSoilMoistureMin, idealSoilMoistureMax;
    const char* sowingSeason;
};

static const int CROP_PROFILE_COUNT = 22;

static const CropProfile CROP_PROFILES[CROP_PROFILE_COUNT] = {
    // name          tMin   tMax   hMin   hMax   smMin  smMax  season
    { "rice",        20.1f, 27.3f, 70.0f, 94.6f, 66.9f, 90.5f, "Kharif" },
    { "maize",       19.0f, 25.8f, 55.3f, 74.9f, 23.9f, 32.4f, "Kharif" },
    { "chickpea",    16.1f, 21.7f, 14.4f, 19.4f, 22.7f, 30.7f, "Rabi"   },
    { "kidneybeans", 17.1f, 23.1f, 18.4f, 24.8f, 30.0f, 40.6f, "Kharif" },
    { "pigeonpeas",  23.5f, 31.9f, 41.0f, 55.4f, 42.4f, 57.3f, "Kharif" },
    { "mothbeans",   24.0f, 32.4f, 45.2f, 61.2f, 14.5f, 19.6f, "Kharif" },
    { "mungbean",    24.2f, 32.8f, 72.7f, 98.3f, 13.7f, 18.5f, "Kharif" },
    { "blackgram",   25.5f, 34.5f, 55.3f, 74.9f, 19.2f, 26.0f, "Kharif" },
    { "lentil",      20.8f, 28.2f, 55.1f, 74.5f, 13.0f, 17.5f, "Rabi"   },
    { "pomegranate", 18.5f, 25.1f, 76.6f,103.6f, 30.5f, 41.2f, "Kharif" },
    { "banana",      23.3f, 31.5f, 68.2f, 92.2f, 29.6f, 40.1f, "Kharif" },
    { "mango",       26.5f, 35.9f, 42.7f, 57.7f, 26.8f, 36.3f, "Kharif" },
    { "grapes",      20.3f, 27.5f, 69.6f, 94.2f, 19.7f, 26.7f, "Rabi"   },
    { "watermelon",  21.7f, 29.3f, 72.4f, 98.0f, 14.5f, 19.6f, "Zaid"   },
    { "muskmelon",   24.4f, 33.0f, 78.5f,106.1f,  7.1f,  9.5f, "Zaid"   },
    { "apple",       19.2f, 26.0f, 78.5f,106.1f, 32.0f, 43.2f, "Rabi"   },
    { "orange",      19.4f, 26.2f, 78.4f,106.0f, 31.3f, 42.4f, "Kharif" },
    { "papaya",      28.6f, 38.8f, 78.5f,106.3f, 40.4f, 54.7f, "Kharif" },
    { "coconut",     23.3f, 31.5f, 80.6f,109.0f, 49.8f, 67.4f, "Kharif" },
    { "cotton",      20.4f, 27.6f, 68.1f, 92.1f, 22.7f, 30.7f, "Kharif" },
    { "jute",        21.3f, 28.8f, 67.7f, 91.5f, 49.5f, 67.0f, "Kharif" },
    { "coffee",      21.7f, 29.3f, 50.1f, 67.7f, 44.8f, 60.6f, "Kharif" }
};

inline const CropProfile* findCropProfile(const char* cropName) {
    if (!cropName || cropName[0] == '\0') return NULL;
    for (int i = 0; i < CROP_PROFILE_COUNT; ++i)
        if (std::strcmp(CROP_PROFILES[i].name, cropName) == 0)
            return &CROP_PROFILES[i];
    return NULL;
}

#endif // CROPPROFILE_H