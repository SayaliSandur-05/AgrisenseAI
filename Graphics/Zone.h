// =============================================================================
// Zone.h  --  AgriSense AI  --  PSOOP
// -----------------------------------------------------------------------------
// Every threshold check delegates to real x86-64 assembly, called through
// COAEngine. The alert level is the CPU's return value, not a C++ if-else.
// =============================================================================
#ifndef ZONE_H
#define ZONE_H

#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

#include "CropProfile.h"
#include "ZoneHistory.h"
#include "COA.h"

enum AlertLevel {
    ALERT_OK      = 0,
    ALERT_WARNING = 1,
    ALERT_DANGER  = 2
};

class Zone {
private:
    int  zoneId;
    char fields[6][32];
    ZoneHistory history;

    AlertLevel currentAlert;
    char       alertMessage[128];

    COAEngine coa;
    std::vector<COAEngine::Step> lastTrace;
    std::string                  lastCheckDescription;

    float parseField(int idx) const {
        const char* s = fields[idx];
        if (!s || s[0] == '\0') return -999.0f;
        char* end = NULL;
        float v = (float)std::strtod(s, &end);
        if (end == s) return -999.0f;
        return v;
    }

public:
    Zone(int id = 0) : zoneId(id), currentAlert(ALERT_OK) {
        for (int i = 0; i < 6; ++i) fields[i][0] = '\0';
        alertMessage[0] = '\0';
    }

    int getId() const { return zoneId; }

    char*       getField(int idx)       { return fields[idx]; }
    const char* getField(int idx) const { return fields[idx]; }

    const char* getCropType() const { return fields[0]; }

    ZoneHistory&       getHistory()       { return history; }
    const ZoneHistory& getHistory() const { return history; }

    AlertLevel  getAlertLevel()   const { return currentAlert; }
    const char* getAlertMessage() const { return alertMessage; }

    const std::vector<COAEngine::Step>& getCOATrace() const { return lastTrace; }
    const std::string& getCOACheckName() const { return lastCheckDescription; }

    void checkThresholds() {
        currentAlert    = ALERT_OK;
        alertMessage[0] = '\0';
        lastTrace.clear();
        lastCheckDescription.clear();

        if (fields[0][0] == '\0') {
            std::snprintf(alertMessage, sizeof(alertMessage),
                          "No crop set for this zone");
            return;
        }

        const CropProfile* profile = findCropProfile(fields[0]);
        if (!profile) {
            currentAlert = ALERT_WARNING;
            std::snprintf(alertMessage, sizeof(alertMessage),
                          "Unknown crop '%s'", fields[0]);
            return;
        }

        float t  = parseField(1);
        float h  = parseField(2);
        float sm = parseField(4);

        bool anyValue = (t > -900.0f) || (h > -900.0f) || (sm > -900.0f);
        if (!anyValue) {
            std::snprintf(alertMessage, sizeof(alertMessage),
                          "Enter sensor values for %s", profile->name);
            return;
        }

        // ---- Soil moisture via real assembly ----
        if (sm > -900.0f) {
            int res = coa.runSoilMoistureCheck(sm,
                        profile->idealSoilMoistureMin,
                        profile->idealSoilMoistureMax);
            lastTrace = coa.trace;
            lastCheckDescription = "Soil Moisture";

            if (res == 2) {
                currentAlert = ALERT_DANGER;
                std::snprintf(alertMessage, sizeof(alertMessage),
                              "Soil moisture LOW (%s)", profile->name);
                return;
            }
            if (res == 1) {
                currentAlert = ALERT_WARNING;
                std::snprintf(alertMessage, sizeof(alertMessage),
                              "Soil moisture HIGH (%s)", profile->name);
                return;
            }
        }

        // ---- Temperature via real assembly ----
        if (t > -900.0f) {
            int res = coa.runTemperatureCheck(t,
                        profile->idealTempMin,
                        profile->idealTempMax);
            lastTrace = coa.trace;
            lastCheckDescription = "Temperature";

            if (res == 2) {
                currentAlert = ALERT_DANGER;
                std::snprintf(alertMessage, sizeof(alertMessage),
                              "Temp out of range (%s)", profile->name);
                return;
            }
        }

        // ---- Humidity (plain rule) ----
        if (h > -900.0f &&
            (h < profile->idealHumidityMin || h > profile->idealHumidityMax)) {
            currentAlert = ALERT_WARNING;
            std::snprintf(alertMessage, sizeof(alertMessage),
                          "Humidity out of range (%s)", profile->name);
            return;
        }

        std::snprintf(alertMessage, sizeof(alertMessage),
                      "Normal for %s", profile->name);
    }
};

#endif // ZONE_H