AgriSense AI - COA Assembly Extension

Files:
  crop_sensors.s     Actual 80386-style Assembly functions.
  crop_ranges.h/cpp  Crop-specific ranges extracted from the uploaded dataset.
  crop_sensor_test.cpp Standalone C++ test program.

Assembly functions:
  check_moisture(value, min, max)
  check_temperature(value, min, max)
  check_humidity(value, min, max)
  check_ph(value, min, max)

Return values:
  0 = NORMAL
  1 = LOW
  2 = HIGH

Decimal values are scaled by 100 before being passed to Assembly.
Example: pH 6.50 becomes 650.

The ranges are the minimum/maximum observed values for each crop in
Crop_recommendation_with_moisture.csv. They are dataset-derived ranges,
not universal agronomic recommendations.

Build (same 32-bit toolchain used for the earlier moisture.s):
  as --32 -o crop_sensors.o crop_sensors.s
  g++ -m32 -std=c++17 -o crop_sensor_test.exe crop_sensor_test.cpp crop_ranges.cpp crop_sensors.o
  .\crop_sensor_test.exe

Example: rice values inside the dataset range should return NORMAL.
Try changing one value below its minimum or above its maximum to see LOW/HIGH.
