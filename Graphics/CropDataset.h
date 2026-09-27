// =============================================================================
// CropDataset.h  --  AgriSense AI  --  CSV-derived crop averages
// -----------------------------------------------------------------------------
// Averages computed from Crop_recommendation.csv (100 rows per crop label).
// This is the "Kaggle dataset integration" mentioned in the plan -- we use it
// to auto-fill the sensor fields when the user picks a crop.
// =============================================================================
#ifndef CROPDATASET_H
#define CROPDATASET_H

#include <cstring>

struct CropData {
    const char* name;
    float N, P, K;          // nitrogen / phosphorus / potassium (kg/ha)
    float temperature;      // avg ideal temperature (C)
    float humidity;         // avg ideal humidity (%)
    float ph;               // avg ideal soil pH
    float rainfall;         // avg ideal rainfall (mm)
};

static const int CROP_DATASET_COUNT = 22;

// Averaged values, one row per crop label.
static const CropData CROP_DATASET[CROP_DATASET_COUNT] = {
    // name           N      P       K      temp   humid   pH     rain
    { "rice",        80.0f,  48.0f,  40.0f, 23.7f, 82.3f,  6.40f, 236.0f },
    { "maize",       77.0f,  48.0f,  20.0f, 22.4f, 65.1f,  6.25f,  84.5f },
    { "chickpea",    40.0f,  67.0f,  80.0f, 18.9f, 16.9f,  7.30f,  80.1f },
    { "kidneybeans", 21.0f,  67.0f,  20.0f, 20.1f, 21.6f,  5.75f, 105.9f },
    { "pigeonpeas",  21.0f,  68.0f,  20.0f, 27.7f, 48.2f,  5.80f, 149.5f },
    { "mothbeans",   21.0f,  48.0f,  20.0f, 28.2f, 53.2f,  6.80f,  51.2f },
    { "mungbean",    21.0f,  47.0f,  20.0f, 28.5f, 85.5f,  6.70f,  48.4f },
    { "blackgram",   40.0f,  67.0f,  19.0f, 30.0f, 65.1f,  7.13f,  67.9f },
    { "lentil",      19.0f,  68.0f,  19.0f, 24.5f, 64.8f,  6.90f,  45.7f },
    { "pomegranate", 19.0f,  18.0f,  40.0f, 21.8f, 90.1f,  6.40f, 107.5f },
    { "banana",     100.0f,  82.0f,  50.0f, 27.4f, 80.2f,  6.00f, 104.6f },
    { "mango",       20.0f,  27.0f,  30.0f, 31.2f, 50.2f,  5.77f,  94.7f },
    { "grapes",      23.0f, 132.0f, 200.0f, 23.9f, 81.9f,  6.03f,  69.6f },
    { "watermelon",  99.0f,  17.0f,  50.0f, 25.5f, 85.2f,  6.50f,  51.0f },
    { "muskmelon",  100.0f,  18.0f,  50.0f, 28.7f, 92.3f,  6.36f,  24.9f },
    { "apple",       21.0f, 134.0f, 200.0f, 22.6f, 92.3f,  5.93f, 112.7f },
    { "orange",      20.0f,  17.0f,  10.0f, 22.8f, 92.2f,  6.95f, 110.5f },
    { "papaya",      50.0f,  59.0f,  50.0f, 33.7f, 92.4f,  6.74f, 142.6f },
    { "coconut",     22.0f,  17.0f,  31.0f, 27.4f, 94.8f,  5.98f, 175.7f },
    { "cotton",     118.0f,  46.0f,  20.0f, 24.0f, 80.1f,  6.92f,  80.1f },
    { "jute",        78.0f,  47.0f,  40.0f, 25.0f, 79.6f,  6.73f, 174.8f },
    { "coffee",     102.0f,  29.0f,  30.0f, 25.5f, 58.9f,  6.79f, 158.1f }
};

inline const CropData* findCropData(const char* name) {
    if (!name || !name[0]) return NULL;
    for (int i = 0; i < CROP_DATASET_COUNT; ++i)
        if (std::strcmp(CROP_DATASET[i].name, name) == 0) return &CROP_DATASET[i];
    return NULL;
}

#endif