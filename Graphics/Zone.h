// =============================================================================
// Zone.h  --  AgriSense AI  --  PSOOP
// -----------------------------------------------------------------------------
// A Zone owns:
//   - its six editable text fields (crop type / temp / humidity / growth /
//     soil moisture / light intensity)
//   - its own ZoneHistory linked list  (from Programming Lab)
//   - checkThresholds(): compares its live numeric fields against the row
//     of CROP_PROFILES that matches the zone's crop. This is the data-driven
//     alternative to subclassing -- a single class, driven by a table.
// =============================================================================
#ifndef ZONE_H
#define ZONE_H

#include <cstring>
#include <cstdio>
#include <cstdlib>

#include "CropProfile.h"
#include "ZoneHistory.h"

// Traffic-light alert state used by the sidebar banner.
enum AlertLevel {
    ALERT_OK      = 0,   // green
    ALERT_WARNING = 1,   // amber
    ALERT_DANGER  = 2    // red
};

class Zone {
private:
    int  zoneId;

    // Six editable text fields, keyed by index for the generic UI.
    // 0=Crop Type  1=Temperature  2=Humidity
    // 3=Growth Rate  4=Soil Moisture  5=Light Intensity
    char fields[6][32];

    ZoneHistory history;

    // Latest computed alert state (refreshed once per frame)
    AlertLevel currentAlert;
    char       alertMessage[128];

    // Parse a field as a float. Returns -999 if the field is empty/not numeric.
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

    // ---- generic field access (used by the dashboard / keyboard) ----
    char*       getField(int idx)       { return fields[idx]; }
    const char* getField(int idx) const { return fields[idx]; }

    // Convenience: raw text of the crop type
    const char* getCropType() const { return fields[0]; }

    // ---- history ----
    ZoneHistory&       getHistory()       { return history; }
    const ZoneHistory& getHistory() const { return history; }

    // ---- alert state ----
    AlertLevel  getAlertLevel()   const { return currentAlert; }
    const char* getAlertMessage() const { return alertMessage; }

    // -------------------------------------------------------------------------
    // THE PSOOP CORE: checkThresholds()
    // -------------------------------------------------------------------------
    // Data-driven threshold check. One function, no per-crop subclasses.
    //  1. If the zone has no crop, stay silent.
    //  2. Look up the matching CropProfile row (table-driven dispatch).
    //  3. Compare soil moisture first (primary demo trigger), then temp, then
    //     humidity, and set alert level + human-readable message accordingly.
    // -------------------------------------------------------------------------
    void checkThresholds() {
        currentAlert    = ALERT_OK;
        alertMessage[0] = '\0';

        // 1. No crop typed yet
        if (fields[0][0] == '\0') {
            std::snprintf(alertMessage, sizeof(alertMessage),
                          "No crop set for this zone");
            return;
        }

        // 2. Table lookup
        const CropProfile* profile = findCropProfile(fields[0]);
        if (!profile) {
            currentAlert = ALERT_WARNING;
            std::snprintf(alertMessage, sizeof(alertMessage),
                          "Unknown crop '%s'", fields[0]);
            return;
        }

        float t  = parseField(1);   // Temperature
        float h  = parseField(2);   // Humidity
        float sm = parseField(4);   // Soil Moisture

        bool anyValue = (t > -900.0f) || (h > -900.0f) || (sm > -900.0f);
        if (!anyValue) {
            std::snprintf(alertMessage, sizeof(alertMessage),
                          "Enter sensor values for %s", profile->name);
            return;
        }

        // 3a. Soil moisture -- primary demo trigger
        if (sm > -900.0f && sm < profile->idealSoilMoistureMin) {
            currentAlert = ALERT_DANGER;
            std::snprintf(alertMessage, sizeof(alertMessage),
                          "Soil moisture LOW (%s)", profile->name);
            return;
        }
        if (sm > -900.0f && sm > profile->idealSoilMoistureMax) {
            currentAlert = ALERT_WARNING;
            std::snprintf(alertMessage, sizeof(alertMessage),
                          "Soil moisture HIGH (%s)", profile->name);
            return;
        }

        // 3b. Temperature
        if (t > -900.0f && (t < profile->idealTempMin || t > profile->idealTempMax)) {
            currentAlert = ALERT_DANGER;
            std::snprintf(alertMessage, sizeof(alertMessage),
                          "Temp out of range (%s)", profile->name);
            return;
        }

        // 3c. Humidity
        if (h > -900.0f && (h < profile->idealHumidityMin || h > profile->idealHumidityMax)) {
            currentAlert = ALERT_WARNING;
            std::snprintf(alertMessage, sizeof(alertMessage),
                          "Humidity out of range (%s)", profile->name);
            return;
        }

        // All good
        std::snprintf(alertMessage, sizeof(alertMessage),
                      "Normal for %s", profile->name);
    }
};

#endif // ZONE_H