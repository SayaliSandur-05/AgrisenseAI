#ifndef CROP_RANGES_H
#define CROP_RANGES_H

struct CropRange
{
    const char* crop;

    // All values are stored as integer(value * 100).
    int moistureMin;
    int moistureMax;

    int temperatureMin;
    int temperatureMax;

    int humidityMin;
    int humidityMax;

    int phMin;
    int phMax;
};

const CropRange* getCropRange(const char* crop);
const CropRange* getCropRangeByIndex(int index);
int getCropCount();

#endif
