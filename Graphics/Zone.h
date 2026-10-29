#ifndef ZONE_H
#define ZONE_H

#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>
using namespace std;
#include "CropProfile.h"
#include "ZoneHistory.h"
#include "COA.h"
#include "Sensor.h"

enum AlertLevel { ALERT_OK = 0, ALERT_WARNING = 1, ALERT_DANGER = 2 };

class Zone {
private:
    int  zoneId;
    char fields[6][32];
    ZoneHistory history;
    TemperatureSensor  tempSensor;
    HumiditySensor     humSensor;
    SoilMoistureSensor soilSensor;
    AlertLevel currentAlert;
    char       alertMessage[128];
    COAEngine coa;
    vector<COAEngine::Step> lastTrace;
    string                  lastCheckDescription;

    float parseField(int idx) const {
        const char* s = fields[idx];
        if (!s || s[0] == '\0') return -999.0f;
        char* end = NULL;
        float v = (float)strtod(s, &end);
        if (end == s) return -999.0f;
        return v;
    }

public:
    float doorAngle;
    int season;
    char market[4][64];

    Zone(int id = 0) : zoneId(id), currentAlert(ALERT_OK), doorAngle(0.0f), season(1) {
        for (int i = 0; i < 6; ++i) fields[i][0] = '\0';
        alertMessage[0] = '\0';
        memset(market, 0, sizeof(market));
    }

    int getId() const { return zoneId; }
    char*       getField(int idx)       { return fields[idx]; }
    const char* getField(int idx) const { return fields[idx]; }
    
    float getNumericField(int idx) {
        if (idx < 0 || idx > 5) return 0.0f;
        return parseField(idx);
    }
    void setNumericField(int idx, float val) {
        if (idx < 0 || idx > 5) return;
        snprintf(fields[idx], 32, "%.1f", val);
    }

    ZoneHistory&       getHistory()       { return history; }
    const ZoneHistory& getHistory() const { return history; }
    AlertLevel  getAlertLevel()   const { return currentAlert; }
    const char* getAlertMessage() const { return alertMessage; }
    const vector<COAEngine::Step>& getCOATrace() const { return lastTrace; }
    const string& getCOACheckName() const { return lastCheckDescription; }

    void checkThresholds() {
        currentAlert    = ALERT_OK;
        alertMessage[0] = '\0';
        lastTrace.clear();
        lastCheckDescription.clear();

        if (fields[0][0] == '\0') {
            snprintf(alertMessage, sizeof(alertMessage), "No crop set for this zone");
            return;
        }
        const CropProfile* profile = findCropProfile(fields[0]);
        if (!profile) {
            currentAlert = ALERT_WARNING;
            snprintf(alertMessage, sizeof(alertMessage), "Unknown crop '%s'", fields[0]);
            return;
        }

        float t  = parseField(1);
        float h  = parseField(2);
        float sm = parseField(4);

        bool anyValue = (t > -900.0f) || (h > -900.0f) || (sm > -900.0f);
        if (!anyValue) {
            snprintf(alertMessage, sizeof(alertMessage), "Enter sensor values for %s", profile->name.c_str());
            return;
        }

        tempSensor.setValue(t  > -900.0f ? t  : 0.0f);
        humSensor.setValue(h   > -900.0f ? h  : 0.0f);
        soilSensor.setValue(sm > -900.0f ? sm : 0.0f);

        if (sm > -900.0f) {
            int res = coa.runSoilMoistureCheck(sm, profile->idealSoilMoistureMin, profile->idealSoilMoistureMax);
            lastTrace = coa.trace;
            lastCheckDescription = "Soil Moisture";
            if (res == 2) { currentAlert = ALERT_DANGER; snprintf(alertMessage, sizeof(alertMessage), "Soil moisture LOW (%s)", profile->name.c_str()); return; }
            if (res == 1) { currentAlert = ALERT_WARNING; snprintf(alertMessage, sizeof(alertMessage), "Soil moisture HIGH (%s)", profile->name.c_str()); return; }
        }
        if (t > -900.0f) {
            int res = coa.runTemperatureCheck(t, profile->idealTempMin, profile->idealTempMax);
            lastTrace = coa.trace;
            lastCheckDescription = "Temperature";
            if (res == 2) { currentAlert = ALERT_DANGER; snprintf(alertMessage, sizeof(alertMessage), "Temp out of range (%s)", profile->name.c_str()); return; }
        }
        if (h > -900.0f && (h < profile->idealHumidityMin || h > profile->idealHumidityMax)) {
            currentAlert = ALERT_WARNING;
            snprintf(alertMessage, sizeof(alertMessage), "Humidity out of range (%s)", profile->name.c_str());
            return;
        }
        snprintf(alertMessage, sizeof(alertMessage), "Normal for %s", profile->name.c_str());
    }
};
#endif