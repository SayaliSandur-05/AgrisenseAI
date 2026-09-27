// =============================================================================
// CropProfile.h  --  AgriSense AI  --  PSOOP (data-driven design)
// -----------------------------------------------------------------------------
// WHY THIS FILE EXISTS
//   Different crops need different ideal conditions. We explicitly chose a
//   *single data-driven Crop class* over one-subclass-per-crop, because only
//   the numbers differ, not the behaviour. Adding a new crop = adding one row
//   to the table below. This is our PSOOP CO.2 talking point in the viva.
//
// NOTE: these ranges are PROVISIONAL for Review 2. They will be refined from
//       the Kaggle dataset before Review 3.
// =============================================================================
#ifndef CROPPROFILE_H
#define CROPPROFILE_H

#include <cstring>
#include <cstdio>

// One row of crop knowledge. Plain struct -- pure data, no behaviour.
struct CropProfile {
    const char* name;                 // must match the string typed in Crop Type
    float idealTempMin,     idealTempMax;       // degrees C
    float idealHumidityMin, idealHumidityMax;   // %
    float idealSoilMoistureMin, idealSoilMoistureMax; // %
    const char* sowingSeason;
};

// The whole table. 6 crops -- our likely demo set.
static const int CROP_PROFILE_COUNT = 6;

static const CropProfile CROP_PROFILES[CROP_PROFILE_COUNT] = {
    // name        tMin   tMax   hMin   hMax   smMin  smMax   season
    { "Tomato",    20.0f, 27.0f, 60.0f, 80.0f, 40.0f, 70.0f, "Kharif" },
    { "Chilli",    20.0f, 30.0f, 50.0f, 70.0f, 35.0f, 60.0f, "Kharif" },
    { "Capsicum",  18.0f, 25.0f, 55.0f, 75.0f, 40.0f, 65.0f, "Rabi"   },
    { "Cotton",    21.0f, 35.0f, 40.0f, 65.0f, 30.0f, 55.0f, "Kharif" },
    { "Peanut",    25.0f, 35.0f, 40.0f, 60.0f, 25.0f, 50.0f, "Kharif" },
    { "Beans",     18.0f, 27.0f, 50.0f, 70.0f, 35.0f, 65.0f, "Rabi"   }
};

// Look up a profile by crop name. Returns NULL if no match.
inline const CropProfile* findCropProfile(const char* cropName) {
    if (!cropName || cropName[0] == '\0') return NULL;
    for (int i = 0; i < CROP_PROFILE_COUNT; ++i) {
        if (std::strcmp(CROP_PROFILES[i].name, cropName) == 0)
            return &CROP_PROFILES[i];
    }
    return NULL;
}

#endif // CROPPROFILE_H