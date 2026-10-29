// Abstract base class demonstrating inheritance + runtime polymorphism.
// Each concrete sensor overrides read() and unit().

#ifndef SENSOR_H
#define SENSOR_H

#include <cstdio>
#include <string>
#include <vector>

using namespace std;

//Abstract base class
class Sensor {
protected:
    string name;
    float  value;

public:
    Sensor(const string& n) : name(n), value(0.0f) {}
    virtual ~Sensor() {}                      // virtual destructor

    //runtime polymorphisum: concrete subclasses override these methods
    virtual float read() const = 0;
    virtual const char* unit() const = 0;

    string getName() const { return name; }
    float  getValue() const { return value; }
    void   setValue(float v) { value = v; }
};

//Concrete sensor: temperature
class TemperatureSensor : public Sensor {
public:
    TemperatureSensor() : Sensor("Temperature") {}
    float read() const override { return value; }
    const char* unit() const override { return "C"; }
};

// Concrete sensor: humidity
class HumiditySensor : public Sensor {
public:
    HumiditySensor() : Sensor("Humidity") {}
    float read() const override { return value; }
    const char* unit() const override { return "%"; }
};

// Concrete sensor: soil moisture
class SoilMoistureSensor : public Sensor {
public:
    SoilMoistureSensor() : Sensor("Soil Moisture") {}
    float read() const override { return value; }
    const char* unit() const override { return "%"; }
};

#endif // SENSOR_H