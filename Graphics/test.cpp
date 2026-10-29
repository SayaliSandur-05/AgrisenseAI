// =============================================================================
// AgriSense AI  --  Final Complete Merge (CGL + PSOOP + PL + COA + CSV + DS)
// =============================================================================
#include <GL/glut.h>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <cmath>
#include <vector>
#include <algorithm>
#include <string>
#include <fstream>
#include <sstream>

#include "CropProfile.h"
#include "ZoneHistory.h"
#include "Zone.h"
#include "CropDataset.h"
#include "DataStructures.h" 

using namespace std;

// ---------------------------------------------------------------------------
// 1. FILE HANDLING & SORTING
// ---------------------------------------------------------------------------
void loadCropsFromCSV() {
    ifstream file("crops.csv");
    if (!file.is_open()) {
        ofstream newFile("crops.csv");
        newFile << "rice,20.1,27.3,70.0,94.6,66.9,90.5,Kharif\n";
        newFile << "maize,19.0,25.8,55.3,74.9,23.9,32.4,Kharif\n";
        newFile << "chickpea,16.1,21.7,14.4,19.4,22.7,30.7,Rabi\n";
        newFile << "banana,23.3,31.5,68.2,92.2,29.6,40.1,Kharif\n";
        newFile << "cotton,20.4,27.6,68.1,92.1,22.7,30.7,Kharif\n";
        newFile.close();
        file.open("crops.csv");
    }

    cropProfiles.clear();
    string line;
    while (getline(file, line)) {
        if (line.empty()) continue;
        stringstream ss(line);
        string item;
        CropProfile cp;
        getline(ss, cp.name, ',');
        getline(ss, item, ','); cp.idealTempMin = stof(item);
        getline(ss, item, ','); cp.idealTempMax = stof(item);
        getline(ss, item, ','); cp.idealHumidityMin = stof(item);
        getline(ss, item, ','); cp.idealHumidityMax = stof(item);
        getline(ss, item, ','); cp.idealSoilMoistureMin = stof(item);
        getline(ss, item, ','); cp.idealSoilMoistureMax = stof(item);
        getline(ss, cp.sowingSeason);
        cropProfiles.push_back(cp);
    }
    file.close();
    
    // LAB: Apply Bubble Sort to the crops list
    bubbleSortCrops(cropProfiles); 
}

void saveNewCropToCSV(const string& name, float t, float h, float sm, const string& season) {
    float tMin = t - 3.0f, tMax = t + 3.0f;
    float hMin = h - 10.0f, hMax = h + 10.0f;
    float smMin = sm - 10.0f, smMax = sm + 10.0f;

    CropProfile cp;
    cp.name = name; cp.idealTempMin = tMin; cp.idealTempMax = tMax;
    cp.idealHumidityMin = hMin; cp.idealHumidityMax = hMax;
    cp.idealSoilMoistureMin = smMin; cp.idealSoilMoistureMax = smMax;
    cp.sowingSeason = season;
    cropProfiles.push_back(cp);

    ofstream file("crops.csv", ios::app);
    if (file.is_open()) {
        file << name << "," << tMin << "," << tMax << "," << hMin << "," << hMax << "," << smMin << "," << smMax << "," << season << "\n";
        file.close();
    }
    bubbleSortCrops(cropProfiles);
}

void applyCropSelection(Zone& z, int index) {
    if (index < 0 || index >= (int)cropProfiles.size()) return;
    const CropProfile& cp = cropProfiles[index];
    snprintf(z.getField(0), 32, "%s", cp.name.c_str());
    z.setNumericField(1, (cp.idealTempMin + cp.idealTempMax) / 2.0f);
    z.setNumericField(2, (cp.idealHumidityMin + cp.idealHumidityMax) / 2.0f);
    z.setNumericField(4, (cp.idealSoilMoistureMin + cp.idealSoilMoistureMax) / 2.0f);
}

// ---------------------------------------------------------------------------
// 2. GLOBAL STATE & BACKEND
// ---------------------------------------------------------------------------
float rotateX = 0.0f, rotateY = 0.0f, zoom = 0.0f;
bool nightMode = false;
vector<Zone> zones; 

NavStack navHistory;      // STACK: For Navigation Back button
AlertQueue pendingAlerts; // QUEUE: For Alert resolution tasks

int zonePage = 0, selectedZone = 0;
const int ZONES_PER_PAGE = 3;

// Search Bar State for Crop Picker
string cropSearchQuery = "";
vector<CropProfile> filteredCrops;
bool editingSearch = false;

// Dashboard Navigation State
enum DashView { DASH_MENU, DASH_LIVE, DASH_CROP, DASH_SEASON, DASH_MARKET, DASH_HISTORY, DASH_CONTROLS, DASH_ALERTS };
bool zoneDashboardOpen = false;
DashView currentDashView = DASH_MENU;

int editingField = -1, editingMarketField = -1;
bool cropSavedMessage = false; 

bool cropPickerOpen = false;
int cropPickerScroll = 0, cropPickerMaxScroll = 0;

int winW = 1300, winH = 780;
const int headerH = 60, sidebarW = 340, footerH = 34;

void updateCropSearch() {
    filteredCrops.clear();
    string q = cropSearchQuery;
    transform(q.begin(), q.end(), q.begin(), ::tolower);
    
    for (const auto& cp : cropProfiles) {
        string n = cp.name;
        transform(n.begin(), n.end(), n.begin(), ::tolower);
        if (n.find(q) != string::npos) {
            filteredCrops.push_back(cp);
        }
    }
}

// ---------------------------------------------------------------------------
// 3. CGL: WEATHER PARTICLES
// ---------------------------------------------------------------------------
enum WeatherMode { WEATHER_CLEAR, WEATHER_RAIN, WEATHER_SMOKE, WEATHER_WIND, WEATHER_STORM };
WeatherMode weatherMode = WEATHER_CLEAR;
const char* weatherNames[] = { "Clear", "Rain", "Smoky/Foggy", "Heavy Wind", "Storm" };

float animationTime = 0.0f, windSway = 0.0f;
struct Particle { float x, y, z, speed, size; };
const int RAIN_COUNT = 420, SMOKE_COUNT = 48, WIND_COUNT = 90;
Particle rainParticles[RAIN_COUNT], smokeParticles[SMOKE_COUNT], windParticles[WIND_COUNT];

float randomRange(float low, float high) { return low + (high - low) * ((float)std::rand() / RAND_MAX); }

void initializeWeather() {
    std::srand(23);
    for (int i = 0; i < RAIN_COUNT; ++i) rainParticles[i] = {randomRange(-27, 27), randomRange(1, 18), randomRange(-18, 18), randomRange(0.22f, 0.48f), randomRange(0.25f, 0.55f)};
    for (int i = 0; i < SMOKE_COUNT; ++i) smokeParticles[i] = {randomRange(-25, 25), randomRange(0.4f, 10), randomRange(-16, 16), randomRange(0.012f, 0.035f), randomRange(0.35f, 1.0f)};
    for (int i = 0; i < WIND_COUNT; ++i) windParticles[i] = {randomRange(-26, 26), randomRange(0.5f, 14), randomRange(-17, 17), randomRange(0.25f, 0.65f), randomRange(0.45f, 1.25f)};
}
void changeWeather() { weatherMode = (WeatherMode)(((int)weatherMode + 1) % 5); glutPostRedisplay(); }

void weatherTimer(int) {
    animationTime += 0.033f;
    float strength = (weatherMode == WEATHER_STORM) ? 1.0f : (weatherMode == WEATHER_WIND) ? 0.8f : 0.18f;
    windSway = std::sin(animationTime * 2.4f) * strength * 0.18f;
    if (weatherMode == WEATHER_RAIN || weatherMode == WEATHER_STORM) {
        float drift = (weatherMode == WEATHER_STORM) ? 0.22f : 0.035f;
        for (int i = 0; i < RAIN_COUNT; ++i) {
            rainParticles[i].y -= rainParticles[i].speed * ((weatherMode == WEATHER_STORM) ? 1.65f : 1.0f);
            rainParticles[i].x += drift;
            if (rainParticles[i].y < 0.0f) { rainParticles[i].y = randomRange(12.0f, 19.0f); rainParticles[i].x = randomRange(-27.0f, 27.0f); }
            if (rainParticles[i].x > 29.0f) rainParticles[i].x = -29.0f;
        }
    }
    if (weatherMode == WEATHER_SMOKE) {
        for (int i = 0; i < SMOKE_COUNT; ++i) {
            smokeParticles[i].y += smokeParticles[i].speed;
            smokeParticles[i].x += std::sin(animationTime + i) * 0.012f;
            if (smokeParticles[i].y > 12.0f) { smokeParticles[i].y = 0.4f; smokeParticles[i].x = randomRange(-25.0f, 25.0f); }
        }
    }
    if (weatherMode == WEATHER_WIND || weatherMode == WEATHER_STORM) {
        for (int i = 0; i < WIND_COUNT; ++i) {
            windParticles[i].x -= windParticles[i].speed * ((weatherMode == WEATHER_STORM) ? 1.5f : 1.0f);
            if (windParticles[i].x < -29.0f) { windParticles[i].x = 29.0f; windParticles[i].y = randomRange(0.5f, 14.0f); }
        }
    }
    glutPostRedisplay(); glutTimerFunc(33, weatherTimer, 0);
}

// ---------------------------------------------------------------------------
// 4. CGL: 3D GEOMETRY
// ---------------------------------------------------------------------------
void drawCube(float x, float y, float z, float sx, float sy, float sz) { glPushMatrix(); glTranslatef(x, y, z); glScalef(sx, sy, sz); glutSolidCube(1.0); glPopMatrix(); }
void drawWeatherEffects() {
    if (weatherMode == WEATHER_RAIN || weatherMode == WEATHER_STORM) {
        glDisable(GL_LIGHTING); glColor4f(0.72f, 0.84f, 0.98f, 0.82f); glLineWidth(1.2f); glBegin(GL_LINES);
        for (int i = 0; i < RAIN_COUNT; ++i) {
            const Particle& p = rainParticles[i];
            glVertex3f(p.x, p.y, p.z); glVertex3f(p.x + (weatherMode==WEATHER_STORM?0.65f:0.12f), p.y - p.size * 1.7f, p.z);
        }
        glEnd(); glLineWidth(1.0f); glEnable(GL_LIGHTING);
    }
    if (weatherMode == WEATHER_SMOKE) {
        glDisable(GL_LIGHTING); glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); glDepthMask(GL_FALSE);
        for (int i = 0; i < SMOKE_COUNT; ++i) {
            const Particle& p = smokeParticles[i];
            glPushMatrix(); glTranslatef(p.x, p.y, p.z); glColor4f(0.66f, 0.68f, 0.69f, 0.10f); glutSolidSphere(p.size, 10, 8); glPopMatrix();
        }
        glDepthMask(GL_TRUE); glDisable(GL_BLEND); glEnable(GL_LIGHTING);
    }
    if (weatherMode == WEATHER_WIND || weatherMode == WEATHER_STORM) {
        glDisable(GL_LIGHTING); glColor3f(0.84f, 0.91f, 0.94f); glLineWidth(1.5f); glBegin(GL_LINES);
        for (int i = 0; i < WIND_COUNT; ++i) {
            const Particle& p = windParticles[i];
            glVertex3f(p.x, p.y, p.z); glVertex3f(p.x - p.size * 2.0f, p.y + p.size * 0.10f, p.z);
        }
        glEnd(); glLineWidth(1.0f); glEnable(GL_LIGHTING);
    }
}
void drawPlant(float x, float z) {
    glPushMatrix(); glTranslatef(x, 0.0f, z); glRotatef(windSway * 180.0f / 3.14159265f, 0.0f, 0.0f, 1.0f);
    glColor3f(0.12f, 0.50f, 0.08f); drawCube(0, 1.25f, 0, 0.12f, 1.4f, 0.12f);
    glColor3f(0.08f, 0.65f, 0.12f);
    glPushMatrix(); glTranslatef(-0.25f, 1.45f, 0); glutSolidSphere(0.25, 12, 12); glPopMatrix();
    glPushMatrix(); glTranslatef( 0.25f, 1.60f, 0); glutSolidSphere(0.25, 12, 12); glPopMatrix();
    glPushMatrix(); glTranslatef(0, 1.90f, 0); glutSolidSphere(0.30, 12, 12); glPopMatrix();
    glPopMatrix();
}
void drawCropBed(float x, float z) {
    glColor3f(0.36f, 0.20f, 0.08f); drawCube(x, 0.55f, z, 3.0f, 0.65f, 2.4f);
    for (int row = -1; row <= 1; row += 2) for (int col = -1; col <= 1; ++col) drawPlant(x + col * 0.9f, z + row * 0.55f);
}
void drawRoofBeam(float x, float z, float angle) { glPushMatrix(); glTranslatef(x, 6.5f, z); glRotatef(angle, 0, 0, 1); glScalef(6.16f, 0.18f, 0.18f); glutSolidCube(1.0); glPopMatrix(); }
void drawBresenhamLine(int x1, int y1, int x2, int y2, float z) {
    int dx = std::abs(x2 - x1), dy = std::abs(y2 - y1), sx = (x1 < x2) ? 1 : -1, sy = (y1 < y2) ? 1 : -1, error = dx - dy;
    glDisable(GL_LIGHTING); glBegin(GL_POINTS);
    for (;;) {
        glVertex3f(x1 / 10.0f, y1 / 10.0f, z);
        if (x1 == x2 && y1 == y2) break;
        int e2 = 2 * error;
        if (e2 > -dy) { error -= dy; x1 += sx; }
        if (e2 < dx) { error += dx; y1 += sy; }
    }
    glEnd(); glEnable(GL_LIGHTING);
}
void drawIrrigationLines() {
    glColor3f(0.10f, 0.45f, 0.85f); glPointSize(3.0f);
    drawBresenhamLine(-48, 12, -28, 12, -2.3f); drawBresenhamLine( 28, 12,  48, 12, -2.3f);
    drawBresenhamLine(-48, 12, -28, 12,  2.3f); drawBresenhamLine( 28, 12,  48, 12,  2.3f);
    glPointSize(1.0f);
}
void drawDoor(float doorAngle) {
    glColor3f(0.10f, 0.15f, 0.15f);
    drawCube(-1.50f, 2.85f, -4.10f, 0.22f, 5.7f, 0.22f); drawCube( 1.50f, 2.85f, -4.10f, 0.22f, 5.7f, 0.22f);
    drawCube(0, 5.70f, -4.10f, 3.0f, 0.22f, 0.22f); drawCube(0, 0.12f, -4.10f, 3.0f, 0.18f, 0.22f);
    glPushMatrix(); glTranslatef(-1.38f, 0, -4.05f); glRotatef(doorAngle, 0, 1, 0); glTranslatef(1.38f, 2.85f, 0);
    glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); glColor4f(0.20f, 0.55f, 0.72f, 0.50f);
    drawCube(0, 0, 0, 2.70f, 5.35f, 0.10f); glDisable(GL_BLEND);
    glColor3f(0.10f, 0.15f, 0.15f); drawCube(0, 0, 0.08f, 0.12f, 5.35f, 0.14f);
    glColor3f(0.05f, 0.05f, 0.05f); drawCube(0.88f, 0, -0.12f, 0.12f, 0.50f, 0.14f);
    glPopMatrix();
}
void drawGlass() {
    glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); glDepthMask(GL_FALSE); glDisable(GL_LIGHTING);
    glColor4f(0.55f, 0.85f, 0.98f, 0.24f);
    drawCube(-5.90f, 3, 0, 0.05f, 5.5f, 7.8f); drawCube( 5.90f, 3, 0, 0.05f, 5.5f, 7.8f); drawCube(0, 3, 3.90f, 11.7f, 5.5f, 0.05f);
    glBegin(GL_TRIANGLES);
    glVertex3f(-5.9f, 5.8f, -3.91f); glVertex3f(0, 7.2f, -3.91f); glVertex3f(5.9f, 5.8f, -3.91f);
    glVertex3f(-5.9f, 5.8f,  3.91f); glVertex3f(0, 7.2f,  3.91f); glVertex3f(5.9f, 5.8f,  3.91f);
    glEnd();
    glBegin(GL_QUADS);
    glVertex3f(-6, 5.8f, -4); glVertex3f(0, 7.2f, -4); glVertex3f(0, 7.2f, 4); glVertex3f(-6, 5.8f, 4);
    glVertex3f(0, 7.2f, -4); glVertex3f(6, 5.8f, -4); glVertex3f(6, 5.8f, 4); glVertex3f(0, 7.2f, 4);
    glEnd();
    glEnable(GL_LIGHTING); glDepthMask(GL_TRUE); glDisable(GL_BLEND);
}
void drawBackFrame() {
    glColor3f(0.78f, 0.65f, 0.45f);
    drawCube(-6, 3, 4, 0.25f, 6, 0.25f); drawCube( 6, 3, 4, 0.25f, 6, 0.25f);
    drawCube(0, 0.15f, 4, 12, 0.20f, 0.20f); drawCube(0, 5.8f, 4, 12, 0.20f, 0.20f);
    drawCube(-3.5f, 3, 4, 0.16f, 5.8f, 0.16f); drawCube( 3.5f, 3, 4, 0.16f, 5.8f, 0.16f);
}
void drawSingleGreenhouse(float doorAngle) {
    glColor3f(0.72f, 0.65f, 0.50f); drawCube(0, 0, 0, 12, 0.25f, 8);
    glColor3f(0.82f, 0.75f, 0.60f); drawCube(0, 0.18f, 0, 1.8f, 0.15f, 7.5f);
    drawCropBed(-3.7f, -2.3f); drawCropBed(3.7f, -2.3f); drawCropBed(-3.7f, 2.3f); drawCropBed(3.7f, 2.3f);
    glColor3f(0.22f, 0.13f, 0.05f);
    drawCube(-3.7f, 0.90f, -2.3f, 3.1f, 0.12f, 2.5f); drawCube( 3.7f, 0.90f, -2.3f, 3.1f, 0.12f, 2.5f);
    drawCube(-3.7f, 0.90f,  2.3f, 3.1f, 0.12f, 2.5f); drawCube( 3.7f, 0.90f,  2.3f, 3.1f, 0.12f, 2.5f);
    glColor3f(0.78f, 0.65f, 0.45f);
    drawCube(-6, 3, -4, 0.25f, 6, 0.25f); drawCube(6, 3, -4, 0.25f, 6, 0.25f);
    drawCube(-6, 3,  4, 0.25f, 6, 0.25f); drawCube(6, 3,  4, 0.25f, 6, 0.25f);
    for (int side = -1; side <= 1; side += 2) {
        float x = side * 6.0f;
        for (int z = -2; z <= 2; z += 2) drawCube(x, 3, (float)z, 0.18f, 6, 0.18f);
        drawCube(x, 0.8f, 0, 0.20f, 1.4f, 8); drawCube(x, 5.8f, 0, 0.20f, 0.20f, 8); drawCube(x, 3, 0, 0.18f, 0.18f, 8);
    }
    drawCube(0, 5.8f, -4, 12, 0.20f, 0.20f); drawCube(0, 5.8f,  4, 12, 0.20f, 0.20f);
    drawCube(-3.75f, 3, -4, 4.5f, 0.15f, 0.15f); drawCube( 3.75f, 3, -4, 4.5f, 0.15f, 0.15f);
    glColor3f(0.78f, 0.65f, 0.45f); drawCube(0, 7.2f, 0, 0.22f, 0.22f, 8);
    for (int z = -4; z <= 4; z += 2) { drawRoofBeam(-3, (float)z, 13.1f); drawRoofBeam( 3, (float)z, -13.1f); }
    drawDoor(doorAngle); drawIrrigationLines(); drawGlass(); drawBackFrame();
}
void drawGround() { glColor3f(0.38f, 0.30f, 0.18f); drawCube(0, -0.56f, 0, 90, 0.10f, 90); }
void drawGreenLand() { glColor3f(0.35f, 0.55f, 0.25f); drawCube(0, -0.35f, 0, 46, 0.30f, 24); }
void drawGrass(float x, float z) {
    glDisable(GL_LIGHTING); glColor3f(0.10f, 0.45f, 0.08f); glLineWidth(2.0f); glBegin(GL_LINES);
    glVertex3f(x, 0, z); glVertex3f(x - 0.12f + windSway * 0.2f, 0.45f, z);
    glVertex3f(x, 0, z); glVertex3f(x + windSway * 0.2f, 0.50f, z);
    glVertex3f(x, 0, z); glVertex3f(x + 0.12f + windSway * 0.2f, 0.40f, z);
    glEnd(); glLineWidth(1.0f); glEnable(GL_LIGHTING);
}
void drawGrassLand() { for (int x = -18; x <= 18; x += 4) { drawGrass((float)x, -8); drawGrass((float)x, 8); } drawGrass(-8, 0); drawGrass(8, 0); }
void drawZoneLabel(const char* text) {
    glColor3f(0.95f, 0.95f, 0.90f); drawCube(0, 0.48f, -4.75f, 4.8f, 0.75f, 0.12f);
    glDisable(GL_LIGHTING); glColor3f(0.05f, 0.10f, 0.06f);
    glPushMatrix(); glTranslatef(0, 0.30f, -4.90f); glScalef(-0.0045f, 0.0045f, 0.0045f);
    float w = 0; for (int i = 0; text[i] != '\0'; ++i) w += glutStrokeWidth(GLUT_STROKE_ROMAN, text[i]);
    glTranslatef(-w / 2.0f, -40, 0);
    for (int i = 0; text[i] != '\0'; ++i) glutStrokeCharacter(GLUT_STROKE_ROMAN, text[i]);
    glPopMatrix(); glEnable(GL_LIGHTING);
}
void drawGreenhouse() {
    int first = zonePage * ZONES_PER_PAGE;
    const float positions[3] = {14, 0, -14};
    for (int i = 0; i < ZONES_PER_PAGE; ++i) {
        int idx = first + i; if (idx >= (int)zones.size()) break;
        glPushMatrix(); glTranslatef(positions[i], 0, 0);
        drawSingleGreenhouse(zones[idx].doorAngle);
        char label[20]; std::snprintf(label, sizeof(label), "ZONE %d", idx + 1);
        drawZoneLabel(label); glPopMatrix();
    }
}
void setLightingAndSky() {
    GLfloat ambient[4] = {0.45f, 0.45f, 0.45f, 1}, diffuse[4] = {0.85f, 0.85f, 0.85f, 1};
    GLfloat specular[4] = {0.15f, 0.15f, 0.15f, 1}, globalAmbient[4] = {0.25f, 0.25f, 0.25f, 1};
    if (nightMode) {
        glClearColor(0.035f, 0.05f, 0.13f, 1);
        ambient[0]=ambient[1]=0.16f; ambient[2]=0.20f; diffuse[0]=diffuse[1]=diffuse[2]=0.35f;
        specular[0]=specular[1]=specular[2]=0.10f; globalAmbient[0]=globalAmbient[1]=0.10f; globalAmbient[2]=0.12f;
    } else glClearColor(0.65f, 0.80f, 0.90f, 1);
    if (weatherMode == WEATHER_RAIN) { if (!nightMode) glClearColor(0.30f, 0.38f, 0.47f, 1); ambient[0]=0.32f; ambient[1]=0.34f; ambient[2]=0.38f; diffuse[0]=0.58f; diffuse[1]=0.61f; diffuse[2]=0.66f; }
    else if (weatherMode == WEATHER_SMOKE) { if (!nightMode) glClearColor(0.48f, 0.50f, 0.50f, 1); ambient[0]=ambient[1]=ambient[2]=0.40f; diffuse[0]=diffuse[1]=diffuse[2]=0.65f; }
    else if (weatherMode == WEATHER_WIND) { if (!nightMode) glClearColor(0.42f, 0.49f, 0.56f, 1); }
    else if (weatherMode == WEATHER_STORM) { if (!nightMode) glClearColor(0.10f, 0.13f, 0.20f, 1); ambient[0]=0.18f; ambient[1]=0.19f; ambient[2]=0.23f; diffuse[0]=0.30f; diffuse[1]=0.32f; diffuse[2]=0.38f; }
    glLightfv(GL_LIGHT0, GL_AMBIENT, ambient); glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuse);
    glLightfv(GL_LIGHT0, GL_SPECULAR, specular); glLightModelfv(GL_LIGHT_MODEL_AMBIENT, globalAmbient);
}

// ---------------------------------------------------------------------------
// 5. 2D UI DRAWING & HELPERS
// ---------------------------------------------------------------------------
void beginOverlay() {
    glMatrixMode(GL_PROJECTION); glPushMatrix(); glLoadIdentity(); gluOrtho2D(0, winW, 0, winH);
    glMatrixMode(GL_MODELVIEW); glPushMatrix(); glLoadIdentity();
    glDisable(GL_DEPTH_TEST); glDisable(GL_LIGHTING); glDisable(GL_SCISSOR_TEST); glDisable(GL_BLEND);
}
void endOverlay() {
    glPopMatrix(); glMatrixMode(GL_PROJECTION); glPopMatrix(); glMatrixMode(GL_MODELVIEW);
    glEnable(GL_DEPTH_TEST); glEnable(GL_LIGHTING);
}
void fillRect(float x, float y, float w, float h) { glBegin(GL_QUADS); glVertex2f(x,y); glVertex2f(x+w,y); glVertex2f(x+w,y+h); glVertex2f(x,y+h); glEnd(); }
void strokeRect(float x, float y, float w, float h) { glBegin(GL_LINE_LOOP); glVertex2f(x,y); glVertex2f(x+w,y); glVertex2f(x+w,y+h); glVertex2f(x,y+h); glEnd(); }
void drawDot(float cx, float cy, float r) { glBegin(GL_TRIANGLE_FAN); for (int a=0; a<=360; a+=15) glVertex2f(cx+r*cos(a*3.14159f/180.0f), cy+r*sin(a*3.14159f/180.0f)); glEnd(); }
void drawText(float x, float y, void* font, const char* text) {
    if (!text) return; glRasterPos2f(x, y); for (int i = 0; text[i] != '\0'; ++i) glutBitmapCharacter(font, text[i]);
}
void drawTextFit(float x, float y, void* font, const char* text, int maxWidth) {
    if (!text || maxWidth <= 0) return; int width = 0, i = 0;
    while (text[i] != '\0') { int charWidth = glutBitmapWidth(font, (unsigned char)text[i]); if (width + charWidth > maxWidth) break; width += charWidth; ++i; }
    if (text[i] == '\0') { drawText(x, y, font, text); return; }
    while (i > 0 && width + glutBitmapWidth(font, '.') * 3 > maxWidth) { --i; width -= glutBitmapWidth(font, (unsigned char)text[i]); }
    char clipped[256]; int n = std::min(i, 250); std::memcpy(clipped, text, n);
    clipped[n] = '.'; clipped[n+1] = '.'; clipped[n+2] = '.'; clipped[n+3] = '\0'; drawText(x, y, font, clipped);
}

struct Rect { int x, y, w, h; };
Rect sidebarAddZoneBtn, dayNightBtn, weatherBtn, zoneListBtn[3], zoneSelectBtn[3], zoneDetailBtn[3];
Rect dashboardBackBtn, sensorMinusBtn[4], sensorPlusBtn[4], cropEditRect, saveCropBtn;
Rect zonePrevBtn, zoneNextBtn, zonePageLabel, alertQueueBtn;
Rect cropPickerArea, cropPickerUpBtn, cropPickerDownBtn, cropPickerBackBtn, cropPickerSearchRect, cropPickerAddBtn; 
Rect cropPickerItem[100];
Rect menuCard[7]; 
Rect marketFieldRect[4], seasonBtn[3]; 

bool inRect(Rect r, int x, int y) { return x >= r.x && x <= r.x + r.w && y >= r.y && y <= r.y + r.h; }
int totalPages() { return ((int)zones.size() + ZONES_PER_PAGE - 1) / ZONES_PER_PAGE; }

void updateLayout() {
    winW = std::max(1, glutGet(GLUT_WINDOW_WIDTH)); winH = std::max(1, glutGet(GLUT_WINDOW_HEIGHT));
    sidebarAddZoneBtn = {20, winH - headerH - 64, sidebarW - 40, 44};
    dayNightBtn = {winW - 300, winH - 50, 130, 36}; weatherBtn = {winW - 162, winH - 50, 150, 36};
    
    int listTop = sidebarAddZoneBtn.y - 46;
    for (int i=0; i<3; ++i) { 
        zoneListBtn[i] = {20, listTop - i * 54, sidebarW - 40, 44}; 
        zoneSelectBtn[i] = {110, listTop - i * 54 + 8, 85, 28};
        zoneDetailBtn[i] = {200, listTop - i * 54 + 8, 85, 28}; 
    }
    int arrowY = listTop - 3 * 54 - 5; 
    zonePrevBtn = {20, arrowY, 54, 30}; zoneNextBtn = {sidebarW - 74, arrowY, 54, 30}; zonePageLabel = {78, arrowY, sidebarW - 156, 30};
    alertQueueBtn = {20, arrowY - 46, sidebarW - 40, 36};
    
    dashboardBackBtn = {20, 20, 140, 40}; 
    
    int startY = winH - 200;
    int cardW = (winW - 60) / 2;
    int cardH = 100;
    int gap = 20;
    for(int i=0; i<7; i++) {
        int row = i / 2;
        int col = i % 2;
        menuCard[i] = { 20 + col * (cardW + gap), startY - row * (cardH + gap), cardW, cardH };
    }

    if (cropPickerOpen) {
        cropPickerArea = {20, 100, sidebarW - 40, winH - 200};
        int ctrlY = cropPickerArea.y + cropPickerArea.h - 34;
        cropPickerBackBtn = {cropPickerArea.x + 4, ctrlY, 60, 30};
        cropPickerUpBtn   = {cropPickerArea.x + cropPickerArea.w - 84, ctrlY, 36, 30};
        cropPickerDownBtn = {cropPickerArea.x + cropPickerArea.w - 44, ctrlY, 36, 30};
        
        // Search Bar Layout
        cropPickerSearchRect = {cropPickerArea.x + 10, ctrlY - 45, cropPickerArea.w - 20, 32};
        
        // Add Button Layout (Below search)
        cropPickerAddBtn = {cropPickerArea.x + 10, cropPickerSearchRect.y - 45, cropPickerArea.w - 20, 35};

        int listTopY = ctrlY - 55, listBotY = cropPickerArea.y + 4;
        int listH = listTopY - listBotY, itemH = 30, itemGap = 4;
        int totalH = (int)filteredCrops.size() * (itemH + itemGap);
        cropPickerMaxScroll = std::max(0, totalH - listH);
        if (cropPickerScroll > cropPickerMaxScroll) cropPickerScroll = cropPickerMaxScroll;
        if (cropPickerScroll < 0) cropPickerScroll = 0;
        
        for (int i = 0; i < (int)filteredCrops.size() && i < 100; i++) {
            int itemY = listTopY - (i + 1) * (itemH + itemGap) + cropPickerScroll;
            cropPickerItem[i] = {cropPickerArea.x + 4, itemY, cropPickerArea.w - 8, itemH};
        }
    }
}

void drawHeader() {
    beginOverlay(); glColor3f(0.95f, 0.98f, 0.97f); fillRect(0, winH - headerH, winW, headerH);
    glColor3f(0.10f, 0.30f, 0.20f); drawText(20, winH - 36, GLUT_BITMAP_TIMES_ROMAN_24, "AgriSense AI");
    glColor3f(0.30f, 0.38f, 0.34f); drawText(185, winH - 34, GLUT_BITMAP_HELVETICA_18, "Smart Greenhouse Management");
    glColor3f(0.20f, 0.30f, 0.25f); drawText(dayNightBtn.x + 5, dayNightBtn.y + 12, GLUT_BITMAP_HELVETICA_12, nightMode ? "Night Mode" : "Day Mode");
    glColor3f(nightMode ? 0.35f : 0.95f, nightMode ? 0.45f : 0.70f, 0.15f); drawDot(dayNightBtn.x + dayNightBtn.w - 13, dayNightBtn.y + 18, 6);
    glColor3f(0.18f, 0.45f, 0.32f); fillRect(weatherBtn.x, weatherBtn.y, weatherBtn.w, weatherBtn.h);
    glColor3f(1, 1, 1); drawText(weatherBtn.x + 8, weatherBtn.y + 12, GLUT_BITMAP_HELVETICA_12, "Weather:");
    drawText(weatherBtn.x + 65, weatherBtn.y + 12, GLUT_BITMAP_HELVETICA_12, weatherNames[(int)weatherMode]);
    endOverlay();
}

void drawCropPicker() {
    glColor4f(0.0f, 0.0f, 0.0f, 0.4f); fillRect(0, 0, winW, winH);
    glColor3f(0.98f, 0.99f, 0.98f); fillRect(cropPickerArea.x, cropPickerArea.y, cropPickerArea.w, cropPickerArea.h);
    glColor3f(0.18f, 0.54f, 0.32f); strokeRect(cropPickerArea.x, cropPickerArea.y, cropPickerArea.w, cropPickerArea.h);
    
    // Draw Top Buttons
    glColor3f(0.90f, 0.94f, 0.92f);
    fillRect(cropPickerUpBtn.x, cropPickerUpBtn.y, cropPickerUpBtn.w, cropPickerUpBtn.h);
    fillRect(cropPickerDownBtn.x, cropPickerDownBtn.y, cropPickerDownBtn.w, cropPickerDownBtn.h);
    fillRect(cropPickerBackBtn.x, cropPickerBackBtn.y, cropPickerBackBtn.w, cropPickerBackBtn.h);
    
    glColor3f(0.12f, 0.28f, 0.20f);
    drawText(cropPickerUpBtn.x + 12, cropPickerUpBtn.y + 10, GLUT_BITMAP_HELVETICA_12, "^");
    drawText(cropPickerDownBtn.x + 12, cropPickerDownBtn.y + 10, GLUT_BITMAP_HELVETICA_12, "v");
    drawText(cropPickerBackBtn.x + 10, cropPickerBackBtn.y + 10, GLUT_BITMAP_HELVETICA_12, "< Back");

    // Draw Search Bar
    glColor3f(editingSearch ? 0.82f : 0.94f, editingSearch ? 0.93f : 0.97f, editingSearch ? 0.86f : 0.95f);
    fillRect(cropPickerSearchRect.x, cropPickerSearchRect.y, cropPickerSearchRect.w, cropPickerSearchRect.h);
    glColor3f(0.75f, 0.82f, 0.78f); strokeRect(cropPickerSearchRect.x, cropPickerSearchRect.y, cropPickerSearchRect.w, cropPickerSearchRect.h);
    
    glColor3f(0.12f, 0.28f, 0.20f);
    string searchText = cropSearchQuery + (editingSearch ? "_" : "");
    if (searchText.empty()) searchText = "Search crop...";
    drawText(cropPickerSearchRect.x + 10, cropPickerSearchRect.y + 11, GLUT_BITMAP_HELVETICA_18, searchText.c_str());

    // Draw Filtered List OR Add Button
    int listTopY = cropPickerSearchRect.y - 10;
    int listBotY = cropPickerArea.y + 4;

    if (filteredCrops.empty()) {
        if (!cropSearchQuery.empty()) {
            // Show Add New Crop Button
            glColor3f(0.18f, 0.54f, 0.32f); 
            fillRect(cropPickerAddBtn.x, cropPickerAddBtn.y, cropPickerAddBtn.w, cropPickerAddBtn.h);
            glColor3f(0.75f, 0.85f, 0.80f); strokeRect(cropPickerAddBtn.x, cropPickerAddBtn.y, cropPickerAddBtn.w, cropPickerAddBtn.h);
            glColor3f(1, 1, 1); 
            string addText = "+ Add new crop: \"" + cropSearchQuery + "\"";
            drawTextFit(cropPickerAddBtn.x + 10, cropPickerAddBtn.y + 10, GLUT_BITMAP_HELVETICA_18, addText.c_str(), cropPickerAddBtn.w - 20);
        } else {
            glColor3f(0.55f, 0.60f, 0.57f);
            drawText(cropPickerArea.x + 20, listTopY - 20, GLUT_BITMAP_HELVETICA_12, "No crops in database. Type to add.");
        }
    } else {
        glScissor(cropPickerArea.x, listBotY, cropPickerArea.w, listTopY - listBotY);
        glEnable(GL_SCISSOR_TEST);

        for (int i = 0; i < (int)filteredCrops.size() && i < 100; i++) {
            Rect r = cropPickerItem[i];
            if (r.y + r.h < listBotY) continue;
            if (r.y > listTopY) continue;
            glColor3f(0.94f, 0.97f, 0.95f); fillRect(r.x, r.y, r.w, r.h);
            glColor3f(0.80f, 0.85f, 0.82f); strokeRect(r.x, r.y, r.w, r.h);
            glColor3f(0.20f, 0.55f, 0.30f); drawDot(r.x + 14, r.y + r.h / 2, 4);
            glColor3f(0.12f, 0.28f, 0.20f); drawText(r.x + 28, r.y + 11, GLUT_BITMAP_HELVETICA_12, filteredCrops[i].name.c_str());
        }
        glDisable(GL_SCISSOR_TEST);
    }

    // Scrollbar
    if (cropPickerMaxScroll > 0) {
        int trackX = cropPickerArea.x + cropPickerArea.w - 10;
        int trackY = listBotY, trackH = listTopY - listBotY;
        glColor3f(0.90f, 0.93f, 0.91f); fillRect(trackX, trackY, 6, trackH);
        int thumbH = std::max(20, (trackH * trackH) / (trackH + cropPickerMaxScroll));
        int thumbY = trackY + trackH - thumbH - (cropPickerScroll * (trackH - thumbH)) / std::max(1, cropPickerMaxScroll);
        glColor3f(0.55f, 0.65f, 0.59f); fillRect(trackX, thumbY, 6, thumbH);
    }
}

void drawSidebar() {
    beginOverlay(); 
    glColor3f(0.97f, 0.99f, 0.98f); fillRect(0, footerH, sidebarW, winH - headerH - footerH);
    glColor3f(0.85f, 0.90f, 0.87f); strokeRect(0, footerH, sidebarW, winH - headerH - footerH);
    
    glColor3f(0.18f, 0.54f, 0.32f); fillRect(sidebarAddZoneBtn.x, sidebarAddZoneBtn.y, sidebarAddZoneBtn.w, sidebarAddZoneBtn.h);
    glColor3f(1, 1, 1); drawText(sidebarAddZoneBtn.x + 28, sidebarAddZoneBtn.y + 15, GLUT_BITMAP_HELVETICA_18, "+ Add Zone");
    glColor3f(0.15f, 0.25f, 0.20f); drawText(20, sidebarAddZoneBtn.y - 26, GLUT_BITMAP_HELVETICA_18, "Zones");

    for (int i = 0; i < (int)zones.size(); i++) {
        if (zones[i].getAlertLevel() != ALERT_OK) pendingAlerts.enqueue(i); 
    }
    
    if (!pendingAlerts.isEmpty()) {
        int nextZone = pendingAlerts.peek();
        glColor3f(0.85f, 0.20f, 0.20f); 
        fillRect(alertQueueBtn.x, alertQueueBtn.y, alertQueueBtn.w, alertQueueBtn.h);
        glColor3f(1, 1, 1); 
        char alertMsg[64]; std::snprintf(alertMsg, sizeof(alertMsg), "! Fix Alert: Zone %d", nextZone + 1);
        drawText(alertQueueBtn.x + 20, alertQueueBtn.y + 10, GLUT_BITMAP_HELVETICA_18, alertMsg);
    }

    int first = zonePage * ZONES_PER_PAGE;
    for (int i = 0; i < 3; ++i) {
        int idx = first + i; if (idx >= (int)zones.size()) break;
        Rect r = zoneListBtn[i]; bool active = selectedZone == idx + 1;
        
        glColor3f(active ? 0.85f : 0.93f, active ? 0.95f : 0.96f, active ? 0.88f : 0.94f);
        fillRect(r.x, r.y, r.w, r.h); 
        glColor3f(active ? 0.18f : 0.75f, active ? 0.54f : 0.80f, active ? 0.32f : 0.77f);
        strokeRect(r.x, r.y, r.w, r.h); 
        glColor3f(0.20f, 0.55f, 0.30f); drawDot(r.x + 18, r.y + r.h / 2, 5);
        
        const char* currentCrop = zones[idx].getField(0);
        bool hasCrop = (currentCrop[0] != '\0');
        
        char label[64]; 
        if (hasCrop) std::snprintf(label, sizeof(label), "Zone %d: %s", idx + 1, currentCrop);
        else std::snprintf(label, sizeof(label), "Zone %d (empty)", idx + 1);
        
        glColor3f(0.12f, 0.28f, 0.20f); 
        drawTextFit(r.x + 28, r.y + r.h / 2 - 5, GLUT_BITMAP_HELVETICA_18, label, 78);
        
        glColor3f(0.18f, 0.54f, 0.32f); 
        fillRect(zoneSelectBtn[i].x, zoneSelectBtn[i].y, zoneSelectBtn[i].w, zoneSelectBtn[i].h);
        glColor3f(1, 1, 1); 
        drawTextFit(zoneSelectBtn[i].x + 6, zoneSelectBtn[i].y + 9, GLUT_BITMAP_HELVETICA_12, hasCrop ? "Change" : "Select Crop", zoneSelectBtn[i].w - 10);
        
        glColor3f(0.18f, 0.54f, 0.32f); 
        fillRect(zoneDetailBtn[i].x, zoneDetailBtn[i].y, zoneDetailBtn[i].w, zoneDetailBtn[i].h);
        glColor3f(1, 1, 1); 
        drawTextFit(zoneDetailBtn[i].x + 10, zoneDetailBtn[i].y + 9, GLUT_BITMAP_HELVETICA_12, hasCrop ? "View / Edit" : "Edit / Add", zoneDetailBtn[i].w - 15);
    }
    
    glColor3f(zonePage > 0 ? 0.18f : 0.70f, zonePage > 0 ? 0.54f : 0.75f, zonePage > 0 ? 0.32f : 0.72f);
    fillRect(zonePrevBtn.x, zonePrevBtn.y, zonePrevBtn.w, zonePrevBtn.h);
    glColor3f(1, 1, 1); drawText(zonePrevBtn.x + 12, zonePrevBtn.y + 9, GLUT_BITMAP_HELVETICA_18, "<<");
    bool canNext = zonePage < totalPages() - 1;
    glColor3f(canNext ? 0.18f : 0.70f, canNext ? 0.54f : 0.75f, canNext ? 0.32f : 0.72f);
    fillRect(zoneNextBtn.x, zoneNextBtn.y, zoneNextBtn.w, zoneNextBtn.h);
    glColor3f(1, 1, 1); drawText(zoneNextBtn.x + 12, zoneNextBtn.y + 9, GLUT_BITMAP_HELVETICA_18, ">>");
    char pageText[40]; std::snprintf(pageText, sizeof(pageText), "Page %d of %d", zonePage + 1, std::max(1, totalPages()));
    glColor3f(0.30f, 0.38f, 0.34f); drawText(zonePageLabel.x + 5, zonePageLabel.y + 10, GLUT_BITMAP_HELVETICA_12, pageText);
    
    glColor3f(0.90f, 0.96f, 0.92f); fillRect(20, 46, sidebarW - 40, 46);
    glColor3f(0.20f, 0.55f, 0.30f); drawDot(38, 46 + 30, 8);
    glColor3f(0.15f, 0.25f, 0.20f); 
    drawText(56, 46 + 34, GLUT_BITMAP_HELVETICA_12, "Use View/Edit to open dashboard"); 
    drawText(56, 46 + 16, GLUT_BITMAP_HELVETICA_12, "or Select Crop to pick from CSV.");

    if (cropPickerOpen) drawCropPicker();
    endOverlay();
}

// ---------------------------------------------------------------------------
// 6. FULL-SCREEN DASHBOARD (Menu & Sub-views)
// ---------------------------------------------------------------------------
void drawZoneDashboard() {
    if (selectedZone < 1 || selectedZone > (int)zones.size()) return;
    Zone& z = zones[selectedZone - 1];

    beginOverlay(); 
    glColor3f(0.95f, 0.98f, 0.96f); fillRect(0, 0, winW, winH);
    glColor3f(0.10f, 0.30f, 0.20f); fillRect(0, winH - 100, winW, 100); // Header
    glColor3f(0.10f, 0.30f, 0.20f); fillRect(0, 0, winW, 70);           // Footer
    
    glColor3f(1, 1, 1);
    char title[96]; 
    if (currentDashView == DASH_MENU) std::snprintf(title, sizeof(title), "AgriSense AI | Zone %d", selectedZone);
    else std::snprintf(title, sizeof(title), "AgriSense AI | Zone %d Details", selectedZone);
    drawText(24, winH - 42, GLUT_BITMAP_TIMES_ROMAN_24, title);
    drawText(26, winH - 72, GLUT_BITMAP_HELVETICA_18, currentDashView == DASH_MENU ? "Select a section. It opens in this window." : "SIMULATED GREENHOUSE MONITORING");

    // Back Button
    dashboardBackBtn = {20, 15, 140, 40};
    glColor3f(0.18f, 0.54f, 0.32f); fillRect(dashboardBackBtn.x, dashboardBackBtn.y, dashboardBackBtn.w, dashboardBackBtn.h);
    glColor3f(1,1,1); 
    drawText(dashboardBackBtn.x + 18, dashboardBackBtn.y + 12, GLUT_BITMAP_HELVETICA_18, currentDashView == DASH_MENU ? "< Back to Zones" : "< Back to Menu");

    if (currentDashView == DASH_MENU) {
        // Draw Main Menu Grid
        const char* titles[7] = {"Live Conditions", "Crop & Growth", "Seasonal Analysis", "Market Price Analysis", "Recent Zone History", "Zone Controls", "Alerts"};
        const char* descs[7] = {
            "Temperature, humidity, soil and light", "Crop name and growth indicator", "Season and sensor-based guidance",
            "Enter prices manually; no live feed", "Planting, watering, fertilizer, harvest", "Pump, door and lighting", "Warnings from sensor readings"
        };
        
        for(int i=0; i<7; i++) {
            Rect r = menuCard[i];
            glColor3f(1, 1, 1); fillRect(r.x, r.y, r.w, r.h);
            glColor3f(0.78f, 0.86f, 0.81f); strokeRect(r.x, r.y, r.w, r.h);
            
            glColor3f(0.10f, 0.30f, 0.20f); drawText(r.x + 20, r.y + r.h - 30, GLUT_BITMAP_HELVETICA_18, titles[i]);
            glColor3f(0.45f, 0.50f, 0.48f); drawText(r.x + 20, r.y + r.h - 55, GLUT_BITMAP_HELVETICA_12, descs[i]);
            glColor3f(0.18f, 0.54f, 0.32f); drawText(r.x + 20, r.y + 15, GLUT_BITMAP_HELVETICA_12, "Open section >");
        }
    } 
    else if (currentDashView == DASH_LIVE) {
        // --- LIVE CONDITIONS ---
        glColor3f(0.28f,0.36f,0.31f); drawText(40, winH - 150, GLUT_BITMAP_HELVETICA_18, "Live Sensor Readings");
        
        auto labelValue = [&](int y, const char* label, int fieldIdx, const char* unit, int control) {
            float val = z.getNumericField(fieldIdx);
            char v[80]; std::snprintf(v, sizeof(v), "%.1f %s", val, unit);
            glColor3f(0.28f, 0.36f, 0.31f); drawText(40, y, GLUT_BITMAP_HELVETICA_18, label);
            glColor3f(0.10f, 0.25f, 0.17f); drawText(250, y, GLUT_BITMAP_TIMES_ROMAN_24, v);
            
            sensorMinusBtn[control] = {400, y - 10, 30, 30}; sensorPlusBtn[control] = {450, y - 10, 30, 30};
            glColor3f(0.87f, 0.94f, 0.89f); fillRect(400, y - 10, 30, 30); fillRect(450, y - 10, 30, 30);
            glColor3f(0.10f, 0.30f, 0.20f); drawText(410, y, GLUT_BITMAP_HELVETICA_18, "-"); drawText(460, y, GLUT_BITMAP_HELVETICA_18, "+");
        };

        labelValue(winH - 200, "Temperature:", 1, "C", 0);
        labelValue(winH - 250, "Humidity:", 2, "%", 1);
        labelValue(winH - 300, "Soil Moisture:", 4, "%", 2);
        labelValue(winH - 350, "Light Intensity:", 5, "lux", 3);
        glColor3f(0.45f, 0.49f, 0.46f); drawText(40, winH - 420, GLUT_BITMAP_HELVETICA_12, "Adjust +/- to trigger the real x86-64 COA Assembly logic.");
    }
    else if (currentDashView == DASH_CROP) {
        // --- CROP & GROWTH ---
        glColor3f(0.28f,0.36f,0.31f); drawText(40, winH - 150, GLUT_BITMAP_HELVETICA_18, "Crop Type:");
        cropEditRect = {150, winH - 165, 300, 30};
        glColor3f(0.90f,0.96f,0.92f); fillRect(cropEditRect.x, cropEditRect.y, cropEditRect.w, cropEditRect.h);
        glColor3f(0.10f,0.25f,0.17f);
        char cropShown[80]; std::snprintf(cropShown, sizeof(cropShown), "%s%s", z.getField(0)[0] ? z.getField(0) : "Type here...", editingField == 0 ? "_" : "");
        drawText(cropEditRect.x + 10, cropEditRect.y + 10, GLUT_BITMAP_HELVETICA_18, cropShown);
        
        saveCropBtn = {470, winH - 165, 80, 30};
        glColor3f(cropSavedMessage ? 0.20f : 0.18f, cropSavedMessage ? 0.55f : 0.54f, cropSavedMessage ? 0.30f : 0.32f);
        fillRect(saveCropBtn.x, saveCropBtn.y, saveCropBtn.w, saveCropBtn.h);
        glColor3f(1,1,1); drawText(saveCropBtn.x + 15, saveCropBtn.y + 9, GLUT_BITMAP_HELVETICA_18, cropSavedMessage ? "Saved!" : "Save");

        const CropProfile* cp = findCropProfile(z.getField(0));
        if (cp) {
            char tStr[100], hStr[100], sStr[100];
            std::snprintf(tStr, sizeof(tStr), "Ideal Temp Range: %.1f C to %.1f C", cp->idealTempMin, cp->idealTempMax);
            std::snprintf(hStr, sizeof(hStr), "Ideal Humidity Range: %.1f %% to %.1f %%", cp->idealHumidityMin, cp->idealHumidityMax);
            std::snprintf(sStr, sizeof(sStr), "Ideal Soil Range: %.1f %% to %.1f %%", cp->idealSoilMoistureMin, cp->idealSoilMoistureMax);
            glColor3f(0.10f, 0.30f, 0.20f); 
            drawText(40, winH - 230, GLUT_BITMAP_HELVETICA_18, tStr);
            drawText(40, winH - 260, GLUT_BITMAP_HELVETICA_18, hStr);
            drawText(40, winH - 290, GLUT_BITMAP_HELVETICA_18, sStr);
        }
        
        char growth[64]; std::snprintf(growth, sizeof(growth), "Growth Indicator: %.1f%% / day", z.getNumericField(3));
        glColor3f(0.28f,0.36f,0.31f); drawText(40, winH - 350, GLUT_BITMAP_HELVETICA_18, growth);
    }
    else if (currentDashView == DASH_ALERTS) {
        // --- ALERTS ---
        glColor3f(0.28f,0.36f,0.31f); drawText(40, winH - 150, GLUT_BITMAP_HELVETICA_18, "System Alerts:");
        AlertLevel lvl = z.getAlertLevel();
        if (lvl == ALERT_DANGER) glColor3f(0.85f, 0.20f, 0.20f);
        else if (lvl == ALERT_WARNING) glColor3f(0.85f, 0.65f, 0.15f);
        else glColor3f(0.20f, 0.55f, 0.30f);
        drawText(180, winH - 150, GLUT_BITMAP_HELVETICA_18, z.getAlertMessage());
        
        glColor3f(0.10f, 0.14f, 0.20f); fillRect(40, winH - 450, winW - 80, 250);
        glColor3f(0.35f, 0.60f, 0.95f); strokeRect(40, winH - 450, winW - 80, 250);
        glColor3f(0.55f, 0.80f, 1.0f); drawText(60, winH - 220, GLUT_BITMAP_HELVETICA_18, "COA Assembly Instruction Trace:");
        
        const auto& trace = z.getCOATrace();
        int ty = winH - 250;
        for (size_t i = 0; i < trace.size() && ty > winH - 430; i++, ty -= 20) {
            glColor3f(0.85f, 0.90f, 0.95f); drawText(60, ty, GLUT_BITMAP_HELVETICA_18, trace[i].asm_line.c_str());
            glColor3f(0.55f, 0.80f, 1.0f); drawText(350, ty, GLUT_BITMAP_HELVETICA_18, trace[i].effect.c_str());
        }
    }
    else if (currentDashView == DASH_MARKET) {
        // --- MARKET PRICE ---
        glColor3f(0.28f,0.36f,0.31f); drawText(40, winH - 150, GLUT_BITMAP_HELVETICA_18, "Market Price Analysis (Manual Entry)");
        const char* marketLabels[4] = {"Crop", "Mandi", "Current Rs/kg", "Prev Rs/kg"};
        for (int i = 0; i < 4; ++i) {
            int cellX = 40 + (i % 2) * 300;
            int fy = winH - 200 - (i / 2) * 60;
            glColor3f(0.38f,0.43f,0.39f); drawText(cellX, fy + 25, GLUT_BITMAP_HELVETICA_12, marketLabels[i]);
            marketFieldRect[i] = {cellX, fy, 250, 30};
            glColor3f(editingMarketField == i ? 0.82f : 0.93f, editingMarketField == i ? 0.93f : 0.96f, editingMarketField == i ? 0.86f : 0.94f);
            fillRect(marketFieldRect[i].x, marketFieldRect[i].y, marketFieldRect[i].w, marketFieldRect[i].h);
            glColor3f(z.market[i][0] ? 0.12f : 0.58f, z.market[i][0] ? 0.25f : 0.61f, z.market[i][0] ? 0.17f : 0.59f);
            char shown[90]; std::snprintf(shown, sizeof(shown), "%s%s", z.market[i][0] ? z.market[i] : "Enter value", editingMarketField == i ? "_" : "");
            drawText(cellX + 10, fy + 10, GLUT_BITMAP_HELVETICA_18, shown);
        }
    }
    else {
        // --- PLACEHOLDER FOR SEASONAL & HISTORY & CONTROLS ---
        glColor3f(0.28f,0.36f,0.31f); drawText(40, winH - 150, GLUT_BITMAP_HELVETICA_18, "Section Under Construction");
    }

    // Footer Hints
    glColor3f(0.85f, 0.90f, 0.87f); drawText(180, 30, GLUT_BITMAP_HELVETICA_12, currentDashView == DASH_MENU ? "Choose a menu card | ESC: return to greenhouse" : "Click < Back to Menu to return");

    endOverlay();
}

// ---------------------------------------------------------------------------
// 7. EVENT HANDLERS
// ---------------------------------------------------------------------------
void mouse(int button, int state, int x, int y) {
    if (button != GLUT_LEFT_BUTTON || state != GLUT_DOWN) return;
    y = winH - y;

    if (cropPickerOpen) {
        // Handle Search Bar Click
        if (inRect(cropPickerSearchRect, x, y)) {
            editingSearch = true;
            glutPostRedisplay(); return;
        }

        // Handle Add New Crop Button Click
        if (filteredCrops.empty() && !cropSearchQuery.empty() && inRect(cropPickerAddBtn, x, y)) {
            Zone& z = zones[selectedZone - 1];
            snprintf(z.getField(0), 32, "%s", cropSearchQuery.c_str());
            
            cropPickerOpen = false; 
            editingSearch = false; 
            cropSearchQuery = ""; 
            updateCropSearch();
            
            zoneDashboardOpen = true;
            currentDashView = DASH_CROP; // Jump to Crop edit tab
            editingField = 0; // Focus input field
            cropSavedMessage = false;
            
            glutPostRedisplay(); return;
        }
        
        if (inRect(cropPickerBackBtn, x, y) || !inRect(cropPickerArea, x, y)) { 
            cropPickerOpen = false; editingSearch = false; cropSearchQuery = ""; updateCropSearch(); glutPostRedisplay(); return; 
        }
        if (inRect(cropPickerUpBtn, x, y)) { cropPickerScroll -= 30; if(cropPickerScroll<0) cropPickerScroll=0; glutPostRedisplay(); return; }
        if (inRect(cropPickerDownBtn, x, y)) { cropPickerScroll += 30; if(cropPickerScroll>cropPickerMaxScroll) cropPickerScroll=cropPickerMaxScroll; glutPostRedisplay(); return; }
        
        // Apply from filtered list
        for (int i = 0; i < (int)filteredCrops.size() && i < 100; i++) {
            if (inRect(cropPickerItem[i], x, y)) { 
                for(int j=0; j<(int)cropProfiles.size(); j++) {
                    if(cropProfiles[j].name == filteredCrops[i].name) {
                        applyCropSelection(zones[selectedZone - 1], j); 
                        break;
                    }
                }
                cropPickerOpen = false; editingSearch = false; cropSearchQuery = ""; updateCropSearch(); glutPostRedisplay(); return; 
            }
        }
        
        editingSearch = false;
        return;
    }

    if (zoneDashboardOpen) {
        // Handle Back button
        if (inRect(dashboardBackBtn, x, y)) { 
            if (currentDashView == DASH_MENU) {
                zoneDashboardOpen = false; editingField = editingMarketField = -1; cropSavedMessage = false; 
                navHistory.pop(); 
                if (!navHistory.isEmpty()) {
                    selectedZone = navHistory.pop(); navHistory.push(selectedZone); zoneDashboardOpen = true;
                }
            } else {
                currentDashView = DASH_MENU; // Go back to the grid menu
                editingField = editingMarketField = -1;
            }
            glutPostRedisplay(); return; 
        }

        if (selectedZone < 1 || selectedZone > (int)zones.size()) return;
        Zone& z = zones[selectedZone - 1];

        if (currentDashView == DASH_MENU) {
            for (int i = 0; i < 7; ++i) {
                if (inRect(menuCard[i], x, y)) {
                    currentDashView = (DashView)(i + 1); // Map index to enum
                    glutPostRedisplay(); return;
                }
            }
        } 
        else if (currentDashView == DASH_LIVE) {
            for (int i = 0; i < 4; ++i) {
                if (inRect(sensorMinusBtn[i], x, y)) { int f = (i==0)?1:(i==1)?2:(i==2)?4:5; z.setNumericField(f, std::max(0.0f, z.getNumericField(f)-(f==5?50.0f:2.0f))); glutPostRedisplay(); return; }
                if (inRect(sensorPlusBtn[i], x, y)) { int f = (i==0)?1:(i==1)?2:(i==2)?4:5; z.setNumericField(f, std::min(f==5?3000.0f:100.0f, z.getNumericField(f)+(f==5?50.0f:2.0f))); glutPostRedisplay(); return; }
            }
        }
        else if (currentDashView == DASH_CROP) {
            if (inRect(saveCropBtn, x, y)) {
                if (z.getField(0)[0] != '\0') {
                    saveNewCropToCSV(z.getField(0), z.getNumericField(1), z.getNumericField(2), z.getNumericField(4), "Custom");
                    cropSavedMessage = true; editingField = -1; glutPostRedisplay(); 
                }
                return;
            }
            if (inRect(cropEditRect, x, y)) { editingField = 0; cropSavedMessage=false; glutPostRedisplay(); return; }
        }
        else if (currentDashView == DASH_MARKET) {
            for (int i = 0; i < 4; ++i) if (inRect(marketFieldRect[i], x, y)) { editingMarketField = i; editingField = -1; glutPostRedisplay(); return; }
        }
        
        editingField = -1; editingMarketField = -1; cropSavedMessage = false; return;
    }

    if (!pendingAlerts.isEmpty() && inRect(alertQueueBtn, x, y)) {
        int targetZone = pendingAlerts.dequeue(); 
        selectedZone = targetZone + 1;            
        navHistory.push(selectedZone);            
        zoneDashboardOpen = true; currentDashView = DASH_MENU;                 
        glutPostRedisplay(); return;
    }

    if (inRect(sidebarAddZoneBtn, x, y)) { zones.emplace_back((int)zones.size() + 1); zonePage = totalPages() - 1; selectedZone = 0; zoneDashboardOpen = false; glutPostRedisplay(); return; }
    if (inRect(dayNightBtn, x, y)) { nightMode = !nightMode; glutPostRedisplay(); return; }
    if (inRect(weatherBtn, x, y)) { changeWeather(); return; }
    if (inRect(zonePrevBtn, x, y) && zonePage > 0) { --zonePage; glutPostRedisplay(); return; }
    if (inRect(zoneNextBtn, x, y) && zonePage < totalPages() - 1) { ++zonePage; glutPostRedisplay(); return; }
    
    for (int i = 0; i < ZONES_PER_PAGE; ++i) {
        int idx = zonePage * ZONES_PER_PAGE + i; if (idx >= (int)zones.size()) break;
        if (inRect(zoneDetailBtn[i], x, y)) { 
            selectedZone = idx + 1; 
            navHistory.push(selectedZone); 
            zoneDashboardOpen = true; currentDashView = DASH_MENU; editingField = -1; cropSavedMessage = false; glutPostRedisplay(); return; 
        }
        if (inRect(zoneSelectBtn[i], x, y)) { 
            selectedZone = idx + 1; cropPickerOpen = true; cropPickerScroll = 0; 
            cropSearchQuery = ""; updateCropSearch(); editingSearch = false;
            glutPostRedisplay(); return; 
        }
        if (inRect(zoneListBtn[i], x, y)) { selectedZone = idx + 1; glutPostRedisplay(); return; }
    }
}

void keyboard(unsigned char key, int, int) {
    if (key == 27) {
        if (cropPickerOpen) { 
            cropPickerOpen = false; editingSearch = false; cropSearchQuery = ""; updateCropSearch(); glutPostRedisplay(); return; 
        }
        if (zoneDashboardOpen) {
            if (editingField != -1 || editingMarketField != -1) { editingField = editingMarketField = -1; glutPostRedisplay(); return; }
            if (currentDashView != DASH_MENU) {
                currentDashView = DASH_MENU; // Go back to grid first
            } else {
                zoneDashboardOpen = false; 
                navHistory.pop();
                if (!navHistory.isEmpty()) {
                    selectedZone = navHistory.pop(); navHistory.push(selectedZone); zoneDashboardOpen = true;
                } else if (selectedZone > 0) { zonePage = (selectedZone - 1) / ZONES_PER_PAGE; } 
            }
            glutPostRedisplay(); return;
        }
    }
    
    // Handle Search Input
    if (cropPickerOpen && editingSearch) {
        int len = (int)cropSearchQuery.length();
        if (key == 8) { 
            if (len > 0) cropSearchQuery.pop_back();
        } else if (key == 13) { 
            editingSearch = false;
            if(!filteredCrops.empty()) {
                 for(int j=0; j<(int)cropProfiles.size(); j++) {
                    if(cropProfiles[j].name == filteredCrops[0].name) {
                        applyCropSelection(zones[selectedZone - 1], j); break;
                    }
                }
                cropPickerOpen = false;
            } else if (!cropSearchQuery.empty()) {
                // Trigger Add Crop flow via Enter
                Zone& z = zones[selectedZone - 1];
                snprintf(z.getField(0), 32, "%s", cropSearchQuery.c_str());
                cropPickerOpen = false;
                zoneDashboardOpen = true;
                currentDashView = DASH_CROP;
                editingField = 0;
                cropSearchQuery = "";
                updateCropSearch();
            }
        } else if (key >= 32 && key <= 126 && len < 20) {
            cropSearchQuery += key;
        }
        updateCropSearch();
        cropPickerScroll = 0; 
        glutPostRedisplay(); return;
    }

    if (zoneDashboardOpen && currentDashView == DASH_CROP && selectedZone >= 1 && selectedZone <= (int)zones.size()) {
        Zone& z = zones[selectedZone - 1]; char* buf = nullptr; int capacity = 0;
        if (editingField == 0) { buf = z.getField(0); capacity = 32; }
        if (buf) {
            int len = (int)std::strlen(buf);
            if (key == 8) { if (len > 0) buf[len - 1] = '\0'; }
            else if (key == 13) { editingField = -1; }
            else if (key >= 32 && key <= 126 && len < capacity - 1) { buf[len] = key; buf[len + 1] = '\0'; cropSavedMessage = false; }
            glutPostRedisplay(); return;
        }
    }
    else if (zoneDashboardOpen && currentDashView == DASH_MARKET && selectedZone >= 1 && selectedZone <= (int)zones.size()) {
        Zone& z = zones[selectedZone - 1]; char* buf = nullptr;
        if (editingMarketField >= 0 && editingMarketField < 4) buf = z.market[editingMarketField];
        if (buf) {
            int len = (int)std::strlen(buf);
            if (key == 8) { if (len > 0) buf[len - 1] = '\0'; }
            else if (key == 13) { editingMarketField = -1; }
            else if (key >= 32 && key <= 126 && len < 63) {
                if ((editingMarketField == 2 || editingMarketField == 3) && !((key >= '0' && key <= '9') || key == '.')) return;
                buf[len] = key; buf[len + 1] = '\0';
            }
            glutPostRedisplay(); return;
        }
    }
    if (key == '+') zoom += 2.0f; else if (key == '-') zoom -= 2.0f;
    else if (key == 'e' || key == 'E') changeWeather();
    else if (key == 'r' || key == 'R') { rotateX = rotateY = zoom = 0.0f; }
    glutPostRedisplay();
}

// ---------------------------------------------------------------------------
// 8. MAIN & DISPLAY
// ---------------------------------------------------------------------------
void display() {
    updateLayout(); 
    for (size_t i = 0; i < zones.size(); ++i) zones[i].checkThresholds();
    glViewport(0, 0, winW, winH);
    setLightingAndSky(); 
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if (zoneDashboardOpen) { 
        drawZoneDashboard(); 
        glutSwapBuffers(); 
        return; 
    }

    int viewX = std::min(sidebarW, winW - 1); 
    int viewW = std::max(1, winW - viewX); 
    int viewH = std::max(1, winH - headerH);
    glViewport(viewX, 0, viewW, viewH);
    glMatrixMode(GL_PROJECTION); glLoadIdentity(); gluPerspective(45.0, (double)viewW / viewH, 1.0, 220.0);
    glMatrixMode(GL_MODELVIEW); glLoadIdentity(); gluLookAt(0, 9, -54, 0, 4, 0, 0, 1, 0);
    glTranslatef(0, 0, zoom); glRotatef(rotateX, 1, 0, 0); glRotatef(rotateY, 0, 1, 0);
    
    GLfloat lightPos[] = {15, 30, -25, 1}; 
    glLightfv(GL_LIGHT0, GL_POSITION, lightPos);
    drawGround(); drawGreenLand(); drawGrassLand(); drawGreenhouse(); drawWeatherEffects();

    glViewport(0, 0, winW, winH); 
    drawSidebar(); 
    drawHeader();
    
    beginOverlay();
    glColor3f(0.95f, 0.98f, 0.97f); fillRect(0, 0, winW, footerH);
    glColor3f(0.30f, 0.38f, 0.34f); drawText(12, 12, GLUT_BITMAP_HELVETICA_12, "Arrows: rotate | +/-: zoom | E: weather");
    endOverlay();
    
    glutSwapBuffers();
}

void reshape(int w, int h) {
    if (w < 900 || h < 620) { glutReshapeWindow(std::max(900, w), std::max(620, h)); return; }
    winW = std::max(1, w); winH = std::max(1, h); glViewport(0, 0, winW, winH);
    glMatrixMode(GL_PROJECTION); glLoadIdentity(); gluPerspective(45.0, (double)winW / std::max(1, winH), 1.0, 220.0);
    glMatrixMode(GL_MODELVIEW);
}

int main(int argc, char** argv) {
    loadCropsFromCSV(); 
    updateCropSearch(); // Initialize the filtered list

    glutInit(&argc, argv); glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(1300, 780); glutCreateWindow("AgriSense AI - Greenhouse Dashboard");
    glEnable(GL_DEPTH_TEST); glShadeModel(GL_SMOOTH); glEnable(GL_LIGHTING); glEnable(GL_LIGHT0);
    glEnable(GL_COLOR_MATERIAL); glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE); glEnable(GL_NORMALIZE);
    
    zones.emplace_back(1); initializeWeather();
    
    glutDisplayFunc(display); glutReshapeFunc(reshape); 
    glutSpecialFunc([](int k, int, int) { if (k==GLUT_KEY_LEFT) rotateY-=5; if (k==GLUT_KEY_RIGHT) rotateY+=5; if (k==GLUT_KEY_UP) rotateX-=5; if (k==GLUT_KEY_DOWN) rotateX+=5; glutPostRedisplay(); });
    glutKeyboardFunc(keyboard); glutMouseFunc(mouse); glutTimerFunc(33, weatherTimer, 0);
    glutMainLoop(); return 0;
}