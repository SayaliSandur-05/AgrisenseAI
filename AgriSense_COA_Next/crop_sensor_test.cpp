#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <string>

#include "crop_ranges.h"

extern "C" int check_moisture(int value, int minimum, int maximum);
extern "C" int check_temperature(int value, int minimum, int maximum);
extern "C" int check_humidity(int value, int minimum, int maximum);
extern "C" int check_ph(int value, int minimum, int maximum);

static int scale100(double value)
{
    return static_cast<int>(value * 100.0 + (value >= 0 ? 0.5 : -0.5));
}

static const char* statusText(int status)
{
    if (status == 1) return "LOW";
    if (status == 2) return "HIGH";
    return "NORMAL";
}

static void checkCrop(const CropRange& r,
                      double moisture,
                      double temperature,
                      double humidity,
                      double ph)
{
    int m = check_moisture(scale100(moisture), r.moistureMin, r.moistureMax);
    int t = check_temperature(scale100(temperature), r.temperatureMin, r.temperatureMax);
    int h = check_humidity(scale100(humidity), r.humidityMin, r.humidityMax);
    int p = check_ph(scale100(ph), r.phMin, r.phMax);

    std::cout << "\nCrop: " << r.crop << '\n';
    std::cout << "Moisture    : " << moisture << " -> " << statusText(m) << '\n';
    std::cout << "Temperature : " << temperature << " -> " << statusText(t) << '\n';
    std::cout << "Humidity    : " << humidity << " -> " << statusText(h) << '\n';
    std::cout << "pH          : " << ph << " -> " << statusText(p) << '\n';
}

int main()
{
    std::string crop;
    double moisture, temperature, humidity, ph;

    std::cout << "AgriSense AI - 80386 Assembly Sensor Checker\n";
    std::cout << "Enter crop name: ";
    std::cin >> crop;

    const CropRange* range = getCropRange(crop.c_str());
    if (range == 0)
    {
        std::cout << "Crop not found in dataset.\n";
        return 1;
    }

    std::cout << "Enter moisture: ";
    std::cin >> moisture;
    std::cout << "Enter temperature: ";
    std::cin >> temperature;
    std::cout << "Enter humidity: ";
    std::cin >> humidity;
    std::cout << "Enter pH: ";
    std::cin >> ph;

    checkCrop(*range, moisture, temperature, humidity, ph);
    return 0;
}
