// =============================================================================
// AgriSense AI  --  Review 2 main application
// =============================================================================
// Features in this build:
//   [1] Real Zone objects + ZoneHistory linked list per zone (PSOOP + PL)
//   [2] Data-driven CropProfile table -> Zone::checkThresholds() live alerts
//   [3] Live alert banner in the sidebar
//   [4] CSV-driven crop picker dropdown (22 crops from Kaggle dataset)
//   [5] Crop reference card showing N/P/K, pH, rainfall, temp, humidity
//   [6] History tab backed by a real doubly-linked list
// =============================================================================

#include <GL/glut.h>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <cmath>

#include "CropProfile.h"
#include "ZoneHistory.h"
#include "Zone.h"
#include "CropDataset.h"

// ---------------------------------------------------------------------------
// Scene controls
// ---------------------------------------------------------------------------
float rotateX = 0.0f, rotateY = 0.0f, zoom = 0.0f;
float doorAngle1 = 0.0f, doorAngle2 = 0.0f, doorAngle3 = 0.0f;
int   zoneCount = 1;
bool  nightMode = false;

// ---------------------------------------------------------------------------
// Real data model
// ---------------------------------------------------------------------------
Zone zones[3];
char pendingHistory[3][32];

// ---------------------------------------------------------------------------
// UI metadata
// ---------------------------------------------------------------------------
const char* fieldLabels[6] = {
    "Crop Type", "Temperature", "Humidity",
    "Growth Rate", "Soil Moisture", "Light Intensity"
};
const char* historyCategoryLabels[6] = {
    "Crop Planted", "Watering", "Fertilizer Applied",
    "Growth Update", "Treatment / Disease Control", "Harvest"
};
const char* historyDetailLabel[6] = {
    "Crop Name", "Watering Details", "Fertilizer Name/Type",
    "Growth Observation", "Treatment Name/Type", "Harvest Details"
};

// ---------------------------------------------------------------------------
// UI state
// ---------------------------------------------------------------------------
int  selectedZone = 0;
int  editingField = -1;

bool historyOpen = false;
int  selectedHistoryCategory = -1;
int  editingHistoryField = -1;

int  scrollOffset = 0, maxScrollOffset = 0;

// Crop picker state
bool cropPickerOpen      = false;
int  cropPickerScroll    = 0;
int  cropPickerMaxScroll = 0;

int  winW = 1300, winH = 780;
const int headerH = 60, sidebarW = 340, footerH = 34;

struct Rect { int x, y, w, h; };

Rect sidebarAddZoneBtn, dayNightBtn;
Rect zoneListBtn[3];
Rect fieldRow[6];
Rect alertBannerRect;
Rect cropRefCardRect;
Rect historyToggleBtn;
Rect historyCategoryBtn[6];
Rect historyFieldRow[3];
Rect submitHistoryBtn;
Rect detailsScrollArea;
int  detailsTitleY, infoBoxY;

// Crop picker layout
Rect cropPickerArea;
Rect cropPickerItem[CROP_DATASET_COUNT];
Rect cropPickerUpBtn, cropPickerDownBtn, cropPickerCloseBtn;

const int MAX_HISTORY_DISPLAY = 20;
int  historyEntryY[MAX_HISTORY_DISPLAY];
int  historyEntryH[MAX_HISTORY_DISPLAY];
int  historyEntryCount = 0;
int  historyListHeaderY = 0;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------
bool inRect(Rect r, int x, int y) {
    return x >= r.x && x <= r.x + r.w && y >= r.y && y <= r.y + r.h;
}

// Copy CSV averages into a zone's editable fields.
void applyCropSelection(Zone& z, const CropData* cd) {
    std::snprintf(z.getField(0), 32, "%s",   cd->name);
    std::snprintf(z.getField(1), 32, "%.1f", cd->temperature);
    std::snprintf(z.getField(2), 32, "%.1f", cd->humidity);
    // No soil-moisture column in the CSV -- derive from rainfall.
    std::snprintf(z.getField(4), 32, "%.1f", cd->rainfall / 3.0f);
}

// ===========================================================================
// Layout
// ===========================================================================
void updateLayout()
{
    winW = glutGet(GLUT_WINDOW_WIDTH);
    winH = glutGet(GLUT_WINDOW_HEIGHT);

    sidebarAddZoneBtn = { 20, (winH - headerH) - 20 - 44, sidebarW - 40, 44 };
    dayNightBtn       = { winW - 150, winH - 50, 140, 40 };

    int listTop = sidebarAddZoneBtn.y - 46;
    for (int i = 0; i < 3; i++)
        zoneListBtn[i] = { 20, listTop - i * 58, sidebarW - 40, 48 };

    infoBoxY = footerH + 20;
    int scrollTop    = listTop - zoneCount * 58 - 10;
    int scrollBottom = infoBoxY + 46 + 10;
    detailsScrollArea = { 20, scrollBottom, sidebarW - 40, scrollTop - scrollBottom };

    int topOfScroll = detailsScrollArea.y + detailsScrollArea.h;
    detailsTitleY = topOfScroll - 25;
    int naturalY = detailsTitleY - 60;

    int zi = selectedZone - 1;

    // -----------------------------------------------------------------
    // CROP PICKER LAYOUT
    // -----------------------------------------------------------------
    if (cropPickerOpen && selectedZone != 0)
    {
        cropPickerArea = detailsScrollArea;

        int ctrlY = cropPickerArea.y + cropPickerArea.h - 34;
        cropPickerUpBtn    = { cropPickerArea.x + 4,                      ctrlY, 40, 30 };
        cropPickerCloseBtn = { cropPickerArea.x + cropPickerArea.w - 84,  ctrlY, 80, 30 };
        cropPickerDownBtn  = { cropPickerArea.x + cropPickerArea.w - 40,  ctrlY, 36, 30 };

        int listTopY = ctrlY - 8;
        int listBotY = cropPickerArea.y + 4;
        int listH    = listTopY - listBotY;
        int itemH    = 30;
        int itemGap  = 4;

        int totalH = CROP_DATASET_COUNT * (itemH + itemGap);
        cropPickerMaxScroll = totalH - listH;
        if (cropPickerMaxScroll < 0) cropPickerMaxScroll = 0;
        if (cropPickerScroll > cropPickerMaxScroll) cropPickerScroll = cropPickerMaxScroll;
        if (cropPickerScroll < 0) cropPickerScroll = 0;

        for (int i = 0; i < CROP_DATASET_COUNT; i++)
        {
            int itemY = listTopY - (i + 1) * (itemH + itemGap) + cropPickerScroll;
            cropPickerItem[i] = { cropPickerArea.x + 4, itemY, cropPickerArea.w - 8, itemH };
        }
        return;
    }

    // -----------------------------------------------------------------
    // NORMAL DETAILS / HISTORY LAYOUT
    // -----------------------------------------------------------------
    if (!historyOpen)
    {
        // Alert banner
        alertBannerRect = { 20, naturalY, detailsScrollArea.w, 50 };
        naturalY -= 60;

        // Six editable field rows
        for (int j = 0; j < 6; j++) {
            fieldRow[j] = { 20, naturalY, detailsScrollArea.w, 30 };
            naturalY -= 38;
        }

        // Reference card BELOW the last field row.
        const int cardH = 132;
        cropRefCardRect = { 20, naturalY - cardH, detailsScrollArea.w, cardH };
        naturalY -= (cardH + 12);
    }
    else
    {
        for (int i = 0; i < 6; i++) {
            historyCategoryBtn[i] = { 20, naturalY, detailsScrollArea.w, 32 };
            naturalY -= 38;
        }
        if (selectedHistoryCategory != -1) {
            naturalY -= 6;
            for (int k = 0; k < 3; k++) {
                historyFieldRow[k] = { 20, naturalY, detailsScrollArea.w, 30 };
                naturalY -= 36;
            }
            submitHistoryBtn = { 20, naturalY, detailsScrollArea.w, 32 };
            naturalY -= 42;
        }

        historyEntryCount = 0;
        if (selectedZone != 0 && zi >= 0 && zi < 3) {
            ZoneHistory& h = zones[zi].getHistory();
            int n = h.getCount();
            if (n > 0) {
                historyListHeaderY = naturalY;
                naturalY -= 22;
                for (int idx = 0; idx < n && historyEntryCount < MAX_HISTORY_DISPLAY; idx++) {
                    HistoryNode* node = h.getFromNewest(idx);
                    if (!node) break;
                    historyEntryY[historyEntryCount] = naturalY;
                    historyEntryH[historyEntryCount] = 50;
                    naturalY -= 54;
                    historyEntryCount++;
                }
            }
        }
    }

    naturalY -= 10;
    historyToggleBtn = { 20, naturalY, detailsScrollArea.w, 40 };
    naturalY -= 10;

    int contentTop    = detailsScrollArea.y + detailsScrollArea.h;
    int contentHeight = contentTop - naturalY;

    maxScrollOffset = contentHeight - detailsScrollArea.h;
    if (maxScrollOffset < 0) maxScrollOffset = 0;
    if (scrollOffset > maxScrollOffset) scrollOffset = maxScrollOffset;
    if (scrollOffset < 0) scrollOffset = 0;

    detailsTitleY += scrollOffset;
    alertBannerRect.y += scrollOffset;
    for (int j = 0; j < 6; j++) fieldRow[j].y += scrollOffset;
    cropRefCardRect.y += scrollOffset;
    for (int i = 0; i < 6; i++) historyCategoryBtn[i].y += scrollOffset;
    for (int k = 0; k < 3; k++) historyFieldRow[k].y += scrollOffset;
    submitHistoryBtn.y += scrollOffset;
    historyListHeaderY += scrollOffset;
    for (int e = 0; e < historyEntryCount; e++) historyEntryY[e] += scrollOffset;
    historyToggleBtn.y += scrollOffset;
}

// ===========================================================================
// 2D overlay primitives
// ===========================================================================
void beginOverlay() {
    glMatrixMode(GL_PROJECTION); glPushMatrix(); glLoadIdentity();
    gluOrtho2D(0, winW, 0, winH);
    glMatrixMode(GL_MODELVIEW);  glPushMatrix(); glLoadIdentity();
    glDisable(GL_DEPTH_TEST); glDisable(GL_LIGHTING);
}
void endOverlay() {
    glEnable(GL_LIGHTING); glEnable(GL_DEPTH_TEST);
    glPopMatrix();
    glMatrixMode(GL_PROJECTION); glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
}
void fillRect(float x, float y, float w, float h) {
    glBegin(GL_QUADS);
    glVertex2f(x, y); glVertex2f(x+w, y); glVertex2f(x+w, y+h); glVertex2f(x, y+h);
    glEnd();
}
void strokeRect(float x, float y, float w, float h) {
    glBegin(GL_LINE_LOOP);
    glVertex2f(x, y); glVertex2f(x+w, y); glVertex2f(x+w, y+h); glVertex2f(x, y+h);
    glEnd();
}
void drawText(float x, float y, void* font, const char* text) {
    glRasterPos2f(x, y);
    for (int i = 0; text[i]; i++) glutBitmapCharacter(font, text[i]);
}
void drawDot(float cx, float cy, float r) {
    glBegin(GL_TRIANGLE_FAN);
    for (int a = 0; a <= 360; a += 30)
        glVertex2f(cx + r * cos(a * 3.14159f / 180.0f),
                   cy + r * sin(a * 3.14159f / 180.0f));
    glEnd();
}

// ===========================================================================
// 3D scene
// ===========================================================================
void drawCube(float x,float y,float z,float sx,float sy,float sz){
    glPushMatrix(); glTranslatef(x,y,z); glScalef(sx,sy,sz); glutSolidCube(1.0); glPopMatrix();
}
void drawPlant(float x,float z){
    glColor3f(0.12,0.50,0.08); drawCube(x,1.25,z,0.12,1.4,0.12);
    glColor3f(0.08,0.65,0.12);
    glPushMatrix(); glTranslatef(x-0.25,1.45,z); glutSolidSphere(0.25,12,12); glPopMatrix();
    glPushMatrix(); glTranslatef(x+0.25,1.60,z); glutSolidSphere(0.25,12,12); glPopMatrix();
    glPushMatrix(); glTranslatef(x,     1.90,z); glutSolidSphere(0.30,12,12); glPopMatrix();
}
void drawCropBed(float x,float z){
    glColor3f(0.36,0.20,0.08); drawCube(x,0.55,z,3.0,0.65,2.4);
    drawPlant(x-0.9,z-0.55); drawPlant(x,z-0.55); drawPlant(x+0.9,z-0.55);
    drawPlant(x-0.9,z+0.55); drawPlant(x,z+0.55); drawPlant(x+0.9,z+0.55);
}
void drawRoofBeam(float x,float z,float angle){
    glPushMatrix(); glTranslatef(x,6.5,z); glRotatef(angle,0,0,1);
    glScalef(6.16,0.18,0.18); glutSolidCube(1.0); glPopMatrix();
}
void drawBresenhamLine(int x1,int y1,int x2,int y2,float z){
    int dx=abs(x2-x1),dy=abs(y2-y1);
    int sx=(x1<x2)?1:-1, sy=(y1<y2)?1:-1;
    int err=dx-dy;
    glDisable(GL_LIGHTING); glBegin(GL_POINTS);
    while(true){
        glVertex3f(x1/10.0f,y1/10.0f,z);
        if(x1==x2&&y1==y2)break;
        int e2=2*err;
        if(e2>-dy){err-=dy;x1+=sx;}
        if(e2< dx){err+=dx;y1+=sy;}
    }
    glEnd(); glEnable(GL_LIGHTING);
}
void drawIrrigationLines(){
    glColor3f(0.10,0.45,0.85); glPointSize(3.0);
    drawBresenhamLine(-48,12,-28,12,-2.3); drawBresenhamLine(28,12,48,12,-2.3);
    drawBresenhamLine(-48,12,-28,12, 2.3); drawBresenhamLine(28,12,48,12, 2.3);
}
void drawDoor(float a){
    glColor3f(0.10,0.15,0.15);
    drawCube(-1.50,2.85,-4.10,0.22,5.7,0.22);
    drawCube( 1.50,2.85,-4.10,0.22,5.7,0.22);
    drawCube(0,5.70,-4.10,3.0,0.22,0.22);
    drawCube(0,0.12,-4.10,3.0,0.18,0.22);
    glPushMatrix();
    glTranslatef(-1.38,0,-4.05); glRotatef(a,0,1,0); glTranslatef(1.38,2.85,0);
    glColor4f(0.20,0.55,0.72,0.50); glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    drawCube(0,0,0,2.70,5.35,0.10); glDisable(GL_BLEND);
    glColor3f(0.10,0.15,0.15); drawCube(0,0,0.08,0.12,5.35,0.14);
    glColor3f(0.05,0.05,0.05); drawCube(0.88,0,-0.12,0.12,0.50,0.14);
    glPopMatrix();
}
void drawBackFrame(){
    glColor3f(0.78,0.65,0.45);
    drawCube(-6,3,4,0.25,6,0.25); drawCube(6,3,4,0.25,6,0.25);
    drawCube(0,0.15,4,12,0.20,0.20); drawCube(0,5.8,4,12,0.20,0.20);
    drawCube(-3.5,3,4,0.16,5.8,0.16); drawCube(3.5,3,4,0.16,5.8,0.16);
}
void drawGlass(){
    glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE); glDisable(GL_LIGHTING);
    glColor4f(0.55,0.85,0.98,0.30);
    drawCube(-5.90,3,0,0.05,5.5,7.8); drawCube(5.90,3,0,0.05,5.5,7.8);
    drawCube(0,3,3.90,11.7,5.5,0.05);
    glColor4f(0.55,0.85,0.98,0.32);
    glBegin(GL_TRIANGLES);
    glVertex3f(-5.9,5.8,-3.91); glVertex3f(0,7.2,-3.91); glVertex3f(5.9,5.8,-3.91);
    glEnd();
    glColor4f(0.55,0.85,0.98,0.28);
    glBegin(GL_TRIANGLES);
    glVertex3f(-5.9,5.8,3.91); glVertex3f(0,7.2,3.91); glVertex3f(5.9,5.8,3.91);
    glEnd();
    glColor4f(0.55,0.85,0.98,0.26);
    glBegin(GL_QUADS);
    glVertex3f(-6,5.8,-4); glVertex3f(0,7.2,-4); glVertex3f(0,7.2,4); glVertex3f(-6,5.8,4);
    glEnd();
    glBegin(GL_QUADS);
    glVertex3f(0,7.2,-4); glVertex3f(6,5.8,-4); glVertex3f(6,5.8,4); glVertex3f(0,7.2,4);
    glEnd();
    glEnable(GL_LIGHTING); glDepthMask(GL_TRUE); glDisable(GL_BLEND);
}
void drawSingleGreenhouse(float a){
    glColor3f(0.72,0.65,0.50); drawCube(0,0,0,12,0.25,8);
    glColor3f(0.82,0.75,0.60); drawCube(0,0.18,0,1.8,0.15,7.5);
    drawCropBed(-3.7,-2.3); drawCropBed(3.7,-2.3);
    drawCropBed(-3.7, 2.3); drawCropBed(3.7, 2.3);
    glColor3f(0.22,0.13,0.05);
    drawCube(-3.7,0.90,-2.3,3.1,0.12,2.5); drawCube(3.7,0.90,-2.3,3.1,0.12,2.5);
    drawCube(-3.7,0.90, 2.3,3.1,0.12,2.5); drawCube(3.7,0.90, 2.3,3.1,0.12,2.5);
    glColor3f(0.78,0.65,0.45);
    drawCube(-6,3,-4,0.25,6,0.25); drawCube(6,3,-4,0.25,6,0.25);
    drawCube(-6,3, 4,0.25,6,0.25); drawCube(6,3, 4,0.25,6,0.25);
    drawCube(-6,3,-2,0.18,6,0.18); drawCube(-6,3,0,0.18,6,0.18); drawCube(-6,3,2,0.18,6,0.18);
    drawCube( 6,3,-2,0.18,6,0.18); drawCube( 6,3,0,0.18,6,0.18); drawCube( 6,3,2,0.18,6,0.18);
    drawCube(-6,0.8,0,0.20,1.4,8); drawCube(6,0.8,0,0.20,1.4,8);
    drawCube(-6,5.8,0,0.20,0.20,8); drawCube(6,5.8,0,0.20,0.20,8);
    drawCube(-6,3,0,0.18,0.18,8);  drawCube(6,3,0,0.18,0.18,8);
    drawCube(0,5.8,-4,12,0.20,0.20); drawCube(0,5.8,4,12,0.20,0.20);
    drawCube(-3.75,3,-4,4.5,0.15,0.15); drawCube(3.75,3,-4,4.5,0.15,0.15);
    drawCube(0,7.2,0,0.22,0.22,8);
    drawRoofBeam(-3,-4,13.1); drawRoofBeam(3,-4,-13.1);
    drawRoofBeam(-3,-2,13.1); drawRoofBeam(3,-2,-13.1);
    drawRoofBeam(-3, 0,13.1); drawRoofBeam(3, 0,-13.1);
    drawRoofBeam(-3, 2,13.1); drawRoofBeam(3, 2,-13.1);
    drawRoofBeam(-3, 4,13.1); drawRoofBeam(3, 4,-13.1);
    drawDoor(a); drawIrrigationLines(); drawGlass(); drawBackFrame();
}
void drawGreenLand(){ glColor3f(0.35f,0.55f,0.25f); drawCube(0,-0.35f,0,46,0.30f,24); }
void drawGround(){ glColor3f(0.38f,0.30f,0.18f); drawCube(0,-0.56f,0,90,0.10f,90); }
void drawGrass(float x,float z){
    glColor3f(0.10,0.45,0.08); glLineWidth(2.0f); glDisable(GL_LIGHTING);
    glBegin(GL_LINES);
    glVertex3f(x,0,z); glVertex3f(x-0.12f,0.45f,z);
    glVertex3f(x,0,z); glVertex3f(x,0.50f,z);
    glVertex3f(x,0,z); glVertex3f(x+0.12f,0.40f,z);
    glEnd(); glEnable(GL_LIGHTING);
}
void drawGrassLand(){
    drawGrass(-19,-8); drawGrass(-16,-7); drawGrass(-12,-9);
    drawGrass(-8,-8);  drawGrass(-4,-9);  drawGrass(0,-8);
    drawGrass(4,-9);   drawGrass(8,-8);   drawGrass(12,-9);
    drawGrass(16,-7);  drawGrass(19,-8);
    drawGrass(-8,0);   drawGrass(8,0);
    drawGrass(-18,7);  drawGrass(-12,8);  drawGrass(-6,7);
    drawGrass(6,7);    drawGrass(12,8);   drawGrass(18,7);
}
void drawZoneLabel(const char* t){
    glColor3f(1,1,1); drawCube(0,0.48f,-4.75f,4.8f,0.75f,0.12f);
    glColor3f(0,0,0); glDisable(GL_LIGHTING);
    glPushMatrix(); glTranslatef(0,0.30f,-4.90f); glScalef(-0.0045f,0.0045f,0.0045f);
    float w=0; for(int i=0;t[i];i++) w+=glutStrokeWidth(GLUT_STROKE_ROMAN,t[i]);
    glTranslatef(-w/2,-40,0); glLineWidth(1.0f);
    for(int i=0;t[i];i++) glutStrokeCharacter(GLUT_STROKE_ROMAN,t[i]);
    glPopMatrix(); glEnable(GL_LIGHTING);
}
void drawGreenhouse(){
    if(zoneCount>=1){ glPushMatrix(); glTranslatef(14,0,0); drawSingleGreenhouse(doorAngle1); drawZoneLabel("ZONE 1"); glPopMatrix(); }
    if(zoneCount>=2){ glPushMatrix(); drawSingleGreenhouse(doorAngle2); drawZoneLabel("ZONE 2"); glPopMatrix(); }
    if(zoneCount>=3){ glPushMatrix(); glTranslatef(-14,0,0); drawSingleGreenhouse(doorAngle3); drawZoneLabel("ZONE 3"); glPopMatrix(); }
}

// ===========================================================================
// Header / footer
// ===========================================================================
void drawHeader(){
    beginOverlay();
    glColor3f(0.95f,0.98f,0.97f); fillRect(0,winH-headerH,winW,headerH);
    glColor3f(0.10f,0.30f,0.20f);
    drawText(24,winH-36,GLUT_BITMAP_TIMES_ROMAN_24,"AgriSense AI");
    glColor3f(0.60f,0.60f,0.60f);
    drawText(190,winH-34,GLUT_BITMAP_HELVETICA_18,"|");
    glColor3f(0.30f,0.38f,0.34f);
    drawText(205,winH-34,GLUT_BITMAP_HELVETICA_18,"Smart Greenhouse Management");
    if(nightMode){
        glColor3f(0.55f,0.60f,0.75f); drawDot(winW-130,winH-30,8);
        glColor3f(0.20f,0.30f,0.25f);
        drawText(winW-110,winH-34,GLUT_BITMAP_HELVETICA_18,"Night Mode");
    } else {
        glColor3f(0.95f,0.70f,0.15f); drawDot(winW-130,winH-30,8);
        glColor3f(0.20f,0.30f,0.25f);
        drawText(winW-110,winH-34,GLUT_BITMAP_HELVETICA_18,"Day Mode");
    }
    endOverlay();
}
void drawFooter(){
    beginOverlay();
    glColor3f(0.95f,0.98f,0.97f); fillRect(0,0,winW,footerH);
    glColor3f(0.30f,0.38f,0.34f);
    drawText(20,12,GLUT_BITMAP_HELVETICA_12,
        "Controls:  </> Rotate   ^/v Zoom   Click a Zone to view details");
    endOverlay();
}

// ===========================================================================
// Crop picker overlay drawing
// ===========================================================================
void drawCropPicker(int zi)
{
    // Dim overlay
    glColor4f(0.0f, 0.0f, 0.0f, 0.15f);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    fillRect(cropPickerArea.x, cropPickerArea.y, cropPickerArea.w, cropPickerArea.h);
    glDisable(GL_BLEND);

    // Panel background
    glColor3f(0.98f, 0.99f, 0.98f);
    fillRect(cropPickerArea.x, cropPickerArea.y, cropPickerArea.w, cropPickerArea.h);
    glColor3f(0.18f, 0.54f, 0.32f);
    strokeRect(cropPickerArea.x, cropPickerArea.y, cropPickerArea.w, cropPickerArea.h);

    glColor3f(0.12f, 0.28f, 0.20f);
    drawText(cropPickerArea.x + 10, cropPickerArea.y + cropPickerArea.h - 56,
             GLUT_BITMAP_HELVETICA_12, "Select a crop (from Kaggle dataset)");

    // Controls
    glColor3f(0.90f, 0.94f, 0.92f);
    fillRect(cropPickerUpBtn.x,    cropPickerUpBtn.y,    cropPickerUpBtn.w,    cropPickerUpBtn.h);
    fillRect(cropPickerCloseBtn.x, cropPickerCloseBtn.y, cropPickerCloseBtn.w, cropPickerCloseBtn.h);
    fillRect(cropPickerDownBtn.x,  cropPickerDownBtn.y,  cropPickerDownBtn.w,  cropPickerDownBtn.h);
    glColor3f(0.18f, 0.54f, 0.32f);
    strokeRect(cropPickerUpBtn.x,    cropPickerUpBtn.y,    cropPickerUpBtn.w,    cropPickerUpBtn.h);
    strokeRect(cropPickerCloseBtn.x, cropPickerCloseBtn.y, cropPickerCloseBtn.w, cropPickerCloseBtn.h);
    strokeRect(cropPickerDownBtn.x,  cropPickerDownBtn.y,  cropPickerDownBtn.w,  cropPickerDownBtn.h);
    glColor3f(0.12f, 0.28f, 0.20f);
    drawText(cropPickerUpBtn.x + 12,    cropPickerUpBtn.y + 10,    GLUT_BITMAP_HELVETICA_12, "^");
    drawText(cropPickerCloseBtn.x + 12, cropPickerCloseBtn.y + 10, GLUT_BITMAP_HELVETICA_12, "Cancel");
    drawText(cropPickerDownBtn.x + 12,  cropPickerDownBtn.y + 10,  GLUT_BITMAP_HELVETICA_12, "v");

    // List clipping
    int listTopY = cropPickerUpBtn.y - 8;
    int listBotY = cropPickerArea.y + 4;
    glScissor(cropPickerArea.x, listBotY, cropPickerArea.w, listTopY - listBotY);
    glEnable(GL_SCISSOR_TEST);

    const char* current = zones[zi].getField(0);

    for (int i = 0; i < CROP_DATASET_COUNT; i++)
    {
        Rect r = cropPickerItem[i];
        if (r.y + r.h < listBotY) continue;
        if (r.y > listTopY) continue;

        bool sel = (std::strcmp(current, CROP_DATASET[i].name) == 0);
        if (sel) glColor3f(0.78f, 0.94f, 0.83f);
        else     glColor3f(0.94f, 0.97f, 0.95f);
        fillRect(r.x, r.y, r.w, r.h);
        if (sel) glColor3f(0.18f, 0.54f, 0.32f);
        else     glColor3f(0.80f, 0.85f, 0.82f);
        strokeRect(r.x, r.y, r.w, r.h);

        glColor3f(0.20f, 0.55f, 0.30f);
        drawDot(r.x + 14, r.y + r.h / 2, 4);

        glColor3f(0.12f, 0.28f, 0.20f);
        drawText(r.x + 28, r.y + 11, GLUT_BITMAP_HELVETICA_12, CROP_DATASET[i].name);
    }
    glDisable(GL_SCISSOR_TEST);

    // Scrollbar
    if (cropPickerMaxScroll > 0)
    {
        int trackX = cropPickerArea.x + cropPickerArea.w - 10;
        int trackY = listBotY;
        int trackH = listTopY - listBotY;
        glColor3f(0.90f, 0.93f, 0.91f);
        fillRect(trackX, trackY, 6, trackH);
        int thumbH = (trackH * trackH) / (trackH + cropPickerMaxScroll);
        if (thumbH < 20) thumbH = 20;
        int thumbY = trackY + trackH - thumbH
                   - (cropPickerScroll * (trackH - thumbH)) / cropPickerMaxScroll;
        glColor3f(0.55f, 0.65f, 0.59f);
        fillRect(trackX, thumbY, 6, thumbH);
    }
}

// ===========================================================================
// Sidebar
// ===========================================================================
void drawSidebar()
{
    beginOverlay();

    glColor3f(0.97f, 0.99f, 0.98f);
    fillRect(0, footerH, sidebarW, winH - headerH - footerH);
    glColor3f(0.85f, 0.90f, 0.87f);
    strokeRect(0, footerH, sidebarW, winH - headerH - footerH);

    // Add-Zone button
    Rect ab = sidebarAddZoneBtn;
    if (zoneCount < 3) glColor3f(0.18f, 0.54f, 0.32f); else glColor3f(0.55f, 0.55f, 0.55f);
    fillRect(ab.x, ab.y, ab.w, ab.h);
    glColor3f(1, 1, 1);
    drawText(ab.x + 30, ab.y + 15, GLUT_BITMAP_HELVETICA_18,
             (zoneCount < 3) ? "+  Add Zone" : "Max 3 Zones");

    glColor3f(0.15f, 0.25f, 0.20f);
    drawText(20, ab.y - 26, GLUT_BITMAP_HELVETICA_18, "Zones");

    for (int i = 0; i < zoneCount; i++) {
        Rect r = zoneListBtn[i];
        bool active = (selectedZone == i + 1);
        if (active) glColor3f(0.85f, 0.95f, 0.88f); else glColor3f(0.93f, 0.96f, 0.94f);
        fillRect(r.x, r.y, r.w, r.h);
        if (active) glColor3f(0.18f, 0.54f, 0.32f); else glColor3f(0.75f, 0.80f, 0.77f);
        strokeRect(r.x, r.y, r.w, r.h);
        glColor3f(0.20f, 0.55f, 0.30f);
        drawDot(r.x + 18, r.y + r.h / 2, 5);
        char label[16]; std::sprintf(label, "Zone %d", i + 1);
        glColor3f(0.12f, 0.28f, 0.20f);
        drawText(r.x + 36, r.y + r.h / 2 - 5, GLUT_BITMAP_HELVETICA_18, label);
    }

    if (selectedZone != 0)
    {
        int zi = selectedZone - 1;

        glScissor(detailsScrollArea.x, detailsScrollArea.y,
                  detailsScrollArea.w, detailsScrollArea.h);
        glEnable(GL_SCISSOR_TEST);

        char title[40];
        if (cropPickerOpen)   std::sprintf(title, "Zone %d - Pick Crop", selectedZone);
        else if (historyOpen) std::sprintf(title, "Zone %d History", selectedZone);
        else                  std::sprintf(title, "Zone %d Details", selectedZone);
        glColor3f(0.12f, 0.28f, 0.20f);
        drawText(20, detailsTitleY, GLUT_BITMAP_HELVETICA_18, title);

        // =============================================================
        // CROP PICKER MODE
        // =============================================================
        if (cropPickerOpen)
        {
            drawCropPicker(zi);
            glDisable(GL_SCISSOR_TEST);
            endOverlay();
            return;
        }

        // =============================================================
        // DETAILS VIEW
        // =============================================================
        if (!historyOpen)
        {
            // ---- Alert banner ----
            AlertLevel lvl = zones[zi].getAlertLevel();
            float bgR, bgG, bgB, txR, txG, txB; const char* statusWord;
            if (lvl == ALERT_DANGER)      { bgR=0.98f;bgG=0.85f;bgB=0.85f; txR=0.65f;txG=0.12f;txB=0.12f; statusWord="ALERT"; }
            else if (lvl == ALERT_WARNING){ bgR=0.99f;bgG=0.94f;bgB=0.78f; txR=0.55f;txG=0.38f;txB=0.02f; statusWord="WARN"; }
            else                          { bgR=0.88f;bgG=0.96f;bgB=0.90f; txR=0.10f;txG=0.40f;txB=0.20f; statusWord="OK"; }

            glColor3f(bgR, bgG, bgB);
            fillRect(alertBannerRect.x, alertBannerRect.y, alertBannerRect.w, alertBannerRect.h);
            glColor3f(txR, txG, txB);
            strokeRect(alertBannerRect.x, alertBannerRect.y, alertBannerRect.w, alertBannerRect.h);

            glColor3f(txR, txG, txB);
            fillRect(alertBannerRect.x + 8, alertBannerRect.y + 15, 60, 20);
            glColor3f(1, 1, 1);
            drawText(alertBannerRect.x + 14, alertBannerRect.y + 22,
                     GLUT_BITMAP_HELVETICA_12, statusWord);
            glColor3f(txR, txG, txB);
            drawText(alertBannerRect.x + 76, alertBannerRect.y + 22,
                     GLUT_BITMAP_HELVETICA_12, zones[zi].getAlertMessage());
            drawText(alertBannerRect.x + 8, alertBannerRect.y + 6,
                     GLUT_BITMAP_HELVETICA_12, "Live threshold check");

            // ---- Field rows ----
            for (int j = 0; j < 6; j++)
            {
                Rect r = fieldRow[j];
                glColor3f(0.30f, 0.38f, 0.34f);
                drawText(r.x, r.y + 8, GLUT_BITMAP_HELVETICA_12, fieldLabels[j]);

                if (j == 0)
                {
                    // Crop Type -> dropdown-style button
                    const char* cur = zones[zi].getField(0);
                    bool empty = (cur[0] == '\0');
                    glColor3f(0.86f, 0.94f, 0.89f);
                    fillRect(r.x + 150, r.y - 6, r.w - 150, 24);
                    glColor3f(0.18f, 0.54f, 0.32f);
                    strokeRect(r.x + 150, r.y - 6, r.w - 150, 24);
                    glColor3f(0.12f, 0.28f, 0.20f);
                    if (empty) {
                        drawText(r.x + 158, r.y + 2, GLUT_BITMAP_HELVETICA_12,
                                 "Select crop...  v");
                    } else {
                        char shown[40]; std::sprintf(shown, "%s  v", cur);
                        drawText(r.x + 158, r.y + 2, GLUT_BITMAP_HELVETICA_12, shown);
                    }
                }
                else
                {
                    // Editable text fields
                    char* buf = zones[zi].getField(j);
                    if (editingField == j) {
                        glColor3f(0.90f, 0.96f, 0.92f);
                        fillRect(r.x + 150, r.y - 4, r.w - 150, 22);
                    }
                    char shown[40];
                    if (buf[0] == '\0') std::strcpy(shown, "Not entered");
                    else std::sprintf(shown, "%s%s", buf, (editingField == j) ? "_" : "");
                    if (buf[0] == '\0') glColor3f(0.65f, 0.70f, 0.67f);
                    else                glColor3f(0.12f, 0.28f, 0.20f);
                    drawText(r.x + 156, r.y + 2, GLUT_BITMAP_HELVETICA_12, shown);
                }
            }

            // ---- Reference card ----
            const CropData* cd = findCropData(zones[zi].getField(0));
            if (cd)
            {
                Rect c = cropRefCardRect;
                glColor3f(0.94f, 0.98f, 0.95f);
                fillRect(c.x, c.y, c.w, c.h);
                glColor3f(0.18f, 0.54f, 0.32f);
                strokeRect(c.x, c.y, c.w, c.h);

                glColor3f(0.10f, 0.40f, 0.24f);
                drawText(c.x + 8, c.y + c.h - 18,
                         GLUT_BITMAP_HELVETICA_12, "Crop Reference Card (CSV)");

                char line1[64], line2[64], line3[64], line4[64];
                std::sprintf(line1, "N:%.0f   P:%.0f   K:%.0f", cd->N, cd->P, cd->K);
                std::sprintf(line2, "Temp avg: %.1f C", cd->temperature);
                std::sprintf(line3, "Humidity: %.1f %%", cd->humidity);
                std::sprintf(line4, "pH: %.2f   Rain: %.0f mm", cd->ph, cd->rainfall);

                glColor3f(0.15f, 0.25f, 0.20f);
                drawText(c.x + 8, c.y + c.h - 40, GLUT_BITMAP_HELVETICA_12, line1);
                drawText(c.x + 8, c.y + c.h - 60, GLUT_BITMAP_HELVETICA_12, line2);
                drawText(c.x + 8, c.y + c.h - 80, GLUT_BITMAP_HELVETICA_12, line3);
                drawText(c.x + 8, c.y + c.h - 100, GLUT_BITMAP_HELVETICA_12, line4);
            }
        }
        // =============================================================
        // HISTORY VIEW
        // =============================================================
        else
        {
            for (int i = 0; i < 6; i++) {
                Rect r = historyCategoryBtn[i];
                bool active = (selectedHistoryCategory == i);
                if (active) glColor3f(0.85f, 0.95f, 0.88f); else glColor3f(0.93f, 0.96f, 0.94f);
                fillRect(r.x, r.y, r.w, r.h);
                if (active) glColor3f(0.18f, 0.54f, 0.32f); else glColor3f(0.75f, 0.80f, 0.77f);
                strokeRect(r.x, r.y, r.w, r.h);
                glColor3f(0.12f, 0.28f, 0.20f);
                drawText(r.x + 12, r.y + r.h / 2 - 5,
                         GLUT_BITMAP_HELVETICA_12, historyCategoryLabels[i]);
            }
            if (selectedHistoryCategory == -1) {
                glColor3f(0.55f, 0.60f, 0.57f);
                drawText(20, historyCategoryBtn[5].y - 24, GLUT_BITMAP_HELVETICA_12,
                         "Pick a category to add an entry");
            } else {
                const char* subLabels[3] = {
                    "Date", historyDetailLabel[selectedHistoryCategory], "Optional Note"
                };
                for (int k = 0; k < 3; k++) {
                    Rect r = historyFieldRow[k];
                    char* buf = pendingHistory[k];
                    glColor3f(0.30f, 0.38f, 0.34f);
                    drawText(r.x, r.y + 8, GLUT_BITMAP_HELVETICA_12, subLabels[k]);
                    if (editingHistoryField == k) {
                        glColor3f(0.90f, 0.96f, 0.92f);
                        fillRect(r.x + 150, r.y - 4, r.w - 150, 22);
                    }
                    char shown[40];
                    if (buf[0] == '\0') std::strcpy(shown, "Not entered");
                    else std::sprintf(shown, "%s%s", buf, (editingHistoryField == k) ? "_" : "");
                    if (buf[0] == '\0') glColor3f(0.65f, 0.70f, 0.67f);
                    else                glColor3f(0.12f, 0.28f, 0.20f);
                    drawText(r.x + 156, r.y + 2, GLUT_BITMAP_HELVETICA_12, shown);
                }
                Rect sb = submitHistoryBtn;
                glColor3f(0.18f, 0.54f, 0.32f);
                fillRect(sb.x, sb.y, sb.w, sb.h);
                glColor3f(1, 1, 1);
                drawText(sb.x + 20, sb.y + 10, GLUT_BITMAP_HELVETICA_12,
                         "Submit Entry (insert into linked list)");
            }

            if (historyEntryCount > 0) {
                glColor3f(0.15f, 0.25f, 0.20f);
                drawText(20, historyListHeaderY, GLUT_BITMAP_HELVETICA_12,
                         "Past Entries (newest first)");
                ZoneHistory& h = zones[zi].getHistory();
                for (int e = 0; e < historyEntryCount; e++) {
                    HistoryNode* node = h.getFromNewest(e);
                    if (!node) break;
                    int y = historyEntryY[e];
                    glColor3f(0.94f, 0.97f, 0.95f);
                    fillRect(20, y, detailsScrollArea.w, 50);
                    glColor3f(0.75f, 0.82f, 0.77f);
                    strokeRect(20, y, detailsScrollArea.w, 50);
                    char line1[96];
                    std::sprintf(line1, "%s  -  %s", node->category, node->date);
                    glColor3f(0.10f, 0.30f, 0.20f);
                    drawText(28, y + 36, GLUT_BITMAP_HELVETICA_12, line1);
                    glColor3f(0.20f, 0.28f, 0.24f);
                    drawText(28, y + 20, GLUT_BITMAP_HELVETICA_12, node->detail);
                    if (node->note[0]) {
                        glColor3f(0.45f, 0.50f, 0.48f);
                        drawText(28, y + 6, GLUT_BITMAP_HELVETICA_12, node->note);
                    }
                }
            }
        }

        // History toggle
        Rect hb = historyToggleBtn;
        if (historyOpen) glColor3f(0.18f, 0.54f, 0.32f); else glColor3f(0.90f, 0.94f, 0.92f);
        fillRect(hb.x, hb.y, hb.w, hb.h);
        if (historyOpen) glColor3f(1, 1, 1); else glColor3f(0.20f, 0.30f, 0.25f);
        drawText(hb.x + 20, hb.y + 14, GLUT_BITMAP_HELVETICA_18,
                 historyOpen ? "- Zone History" : "+ Zone History");

        glDisable(GL_SCISSOR_TEST);

        if (maxScrollOffset > 0) {
            int trackX = detailsScrollArea.x + detailsScrollArea.w - 6;
            int trackY = detailsScrollArea.y;
            int trackH = detailsScrollArea.h;
            glColor3f(0.90f, 0.93f, 0.91f);
            fillRect(trackX, trackY, 6, trackH);
            int thumbH = (trackH * trackH) / (trackH + maxScrollOffset);
            if (thumbH < 20) thumbH = 20;
            int thumbY = trackY + trackH - thumbH
                       - (scrollOffset * (trackH - thumbH)) / maxScrollOffset;
            glColor3f(0.55f, 0.65f, 0.59f);
            fillRect(trackX, thumbY, 6, thumbH);
        }
    }

    glColor3f(0.90f, 0.96f, 0.92f);
    fillRect(20, infoBoxY, sidebarW - 40, 46);
    glColor3f(0.20f, 0.55f, 0.30f);
    drawDot(38, infoBoxY + 30, 8);
    glColor3f(0.15f, 0.25f, 0.20f);
    drawText(56, infoBoxY + 34, GLUT_BITMAP_HELVETICA_12, "Click on a zone to view");
    drawText(56, infoBoxY + 16, GLUT_BITMAP_HELVETICA_12, "and edit its details.");

    endOverlay();
}

// ===========================================================================
// Mouse
// ===========================================================================
void mouse(int button, int state, int x, int y)
{
    // Scroll wheel
    if (button == 3 || button == 4) {
        if (state != GLUT_DOWN) return;
        int wy = winH - y;

        if (cropPickerOpen && inRect(cropPickerArea, x, wy)) {
            int step = (button == 3) ? 30 : -30;
            cropPickerScroll += step;
            if (cropPickerScroll < 0) cropPickerScroll = 0;
            if (cropPickerScroll > cropPickerMaxScroll) cropPickerScroll = cropPickerMaxScroll;
            glutPostRedisplay(); return;
        }

        if (selectedZone != 0 && inRect(detailsScrollArea, x, wy)) {
            const int step = 24;
            int direction = (button == 3) ? 1 : -1;
            scrollOffset -= direction * step;
            if (scrollOffset < 0) scrollOffset = 0;
            if (scrollOffset > maxScrollOffset) scrollOffset = maxScrollOffset;
            glutPostRedisplay();
        }
        return;
    }

    if (button != GLUT_LEFT_BUTTON || state != GLUT_DOWN) return;
    y = winH - y;

    // ---- Crop picker mode ----
    if (cropPickerOpen && selectedZone != 0)
    {
        int zi = selectedZone - 1;

        if (inRect(cropPickerCloseBtn, x, y) || !inRect(cropPickerArea, x, y)) {
            cropPickerOpen = false;
            cropPickerScroll = 0;
            glutPostRedisplay(); return;
        }
        if (inRect(cropPickerUpBtn, x, y)) {
            cropPickerScroll -= 30;
            if (cropPickerScroll < 0) cropPickerScroll = 0;
            glutPostRedisplay(); return;
        }
        if (inRect(cropPickerDownBtn, x, y)) {
            cropPickerScroll += 30;
            if (cropPickerScroll > cropPickerMaxScroll) cropPickerScroll = cropPickerMaxScroll;
            glutPostRedisplay(); return;
        }
        for (int i = 0; i < CROP_DATASET_COUNT; i++) {
            if (inRect(cropPickerItem[i], x, y)) {
                applyCropSelection(zones[zi], &CROP_DATASET[i]);
                cropPickerOpen = false;
                cropPickerScroll = 0;
                glutPostRedisplay(); return;
            }
        }
        return;
    }

    // Add zone
    if (inRect(sidebarAddZoneBtn, x, y)) {
        if (zoneCount < 3) zoneCount++;
        glutPostRedisplay(); return;
    }
    if (inRect(dayNightBtn, x, y)) {
        nightMode = !nightMode;
        glutPostRedisplay(); return;
    }

    for (int i = 0; i < zoneCount; i++) {
        if (inRect(zoneListBtn[i], x, y)) {
            selectedZone = i + 1;
            editingField = -1;
            historyOpen = false;
            selectedHistoryCategory = -1;
            editingHistoryField = -1;
            scrollOffset = 0;
            cropPickerOpen = false;
            for (int k = 0; k < 3; k++) pendingHistory[k][0] = '\0';
            glutPostRedisplay(); return;
        }
    }

    if (selectedZone != 0 && inRect(detailsScrollArea, x, y))
    {
        if (inRect(historyToggleBtn, x, y)) {
            historyOpen = !historyOpen;
            selectedHistoryCategory = -1;
            editingField = -1;
            editingHistoryField = -1;
            scrollOffset = 0;
            cropPickerOpen = false;
            for (int k = 0; k < 3; k++) pendingHistory[k][0] = '\0';
            glutPostRedisplay(); return;
        }

        if (!historyOpen)
        {
            for (int j = 0; j < 6; j++) {
                if (inRect(fieldRow[j], x, y)) {
                    if (j == 0) {
                        cropPickerOpen   = true;
                        cropPickerScroll = 0;
                        editingField     = -1;
                    } else {
                        editingField = j;
                    }
                    glutPostRedisplay(); return;
                }
            }
        }
        else
        {
            for (int i = 0; i < 6; i++) {
                if (inRect(historyCategoryBtn[i], x, y)) {
                    selectedHistoryCategory = (selectedHistoryCategory == i) ? -1 : i;
                    editingHistoryField = -1;
                    scrollOffset = 0;
                    for (int k = 0; k < 3; k++) pendingHistory[k][0] = '\0';
                    glutPostRedisplay(); return;
                }
            }
            if (selectedHistoryCategory != -1) {
                for (int k = 0; k < 3; k++) {
                    if (inRect(historyFieldRow[k], x, y)) {
                        editingHistoryField = k;
                        glutPostRedisplay(); return;
                    }
                }
                if (inRect(submitHistoryBtn, x, y)) {
                    zones[selectedZone - 1].getHistory().insertEvent(
                        historyCategoryLabels[selectedHistoryCategory],
                        pendingHistory[0], pendingHistory[1], pendingHistory[2]);
                    for (int k = 0; k < 3; k++) pendingHistory[k][0] = '\0';
                    editingHistoryField = -1;
                    scrollOffset = 0;
                    glutPostRedisplay(); return;
                }
            }
        }
    }

    editingField = -1;
    editingHistoryField = -1;
    glutPostRedisplay();
}

// ===========================================================================
// Special keys
// ===========================================================================
void specialKeys(int key, int x, int y) {
    if (key == GLUT_KEY_LEFT)  rotateY -= 5.0f;
    if (key == GLUT_KEY_RIGHT) rotateY += 5.0f;
    if (key == GLUT_KEY_UP)    rotateX -= 5.0f;
    if (key == GLUT_KEY_DOWN)  rotateX += 5.0f;
    glutPostRedisplay();
}

// ===========================================================================
// Keyboard
// ===========================================================================
void keyboard(unsigned char key, int x, int y)
{
    // ESC closes crop picker
    if (key == 27 && cropPickerOpen) {
        cropPickerOpen = false;
        cropPickerScroll = 0;
        glutPostRedisplay(); return;
    }

    // Don't allow text typing while picker is open
    if (cropPickerOpen) return;

    if (editingField != -1 && selectedZone != 0)
    {
        char* buf = zones[selectedZone - 1].getField(editingField);
        int len = (int)std::strlen(buf);
        if (key == 8) { if (len > 0) buf[len - 1] = '\0'; }
        else if (key == 13) editingField = -1;
        else if (key >= 32 && key <= 126 && len < 31) { buf[len] = key; buf[len + 1] = '\0'; }
        glutPostRedisplay(); return;
    }
    if (editingHistoryField != -1 && selectedZone != 0 && selectedHistoryCategory != -1)
    {
        char* buf = pendingHistory[editingHistoryField];
        int len = (int)std::strlen(buf);
        if (key == 8) { if (len > 0) buf[len - 1] = '\0'; }
        else if (key == 13) editingHistoryField = -1;
        else if (key >= 32 && key <= 126 && len < 31) { buf[len] = key; buf[len + 1] = '\0'; }
        glutPostRedisplay(); return;
    }

    if (key == '+') zoom -= 1.0f;
    if (key == '-') zoom += 1.0f;
    if (key == '1') { doorAngle1 = 90; doorAngle2 = 0;  doorAngle3 = 0; }
    if (key == '2') { doorAngle1 = 0;  doorAngle2 = 90; doorAngle3 = 0; }
    if (key == '3') { doorAngle1 = 0;  doorAngle2 = 0;  doorAngle3 = 90; }
    if (key == 'a' || key == 'A' || key == 'o' || key == 'O')
        doorAngle1 = doorAngle2 = doorAngle3 = 90;
    if (key == 'c' || key == 'C')
        doorAngle1 = doorAngle2 = doorAngle3 = 0;
    if (key == 'r' || key == 'R') {
        rotateX = rotateY = zoom = 0;
        doorAngle1 = doorAngle2 = doorAngle3 = 0;
    }
    glutPostRedisplay();
}

// ===========================================================================
// Display
// ===========================================================================
void display()
{
    updateLayout();
    for (int i = 0; i < 3; i++) zones[i].checkThresholds();

    if (nightMode) {
        glClearColor(0.04f, 0.06f, 0.16f, 1.0f);
        GLfloat a[] = {0.16f,0.16f,0.20f,1}, d[]={0.35f,0.35f,0.40f,1};
        GLfloat s[] = {0.10f,0.10f,0.10f,1}, g[]={0.10f,0.10f,0.12f,1};
        glLightfv(GL_LIGHT0, GL_AMBIENT, a); glLightfv(GL_LIGHT0, GL_DIFFUSE, d);
        glLightfv(GL_LIGHT0, GL_SPECULAR, s); glLightModelfv(GL_LIGHT_MODEL_AMBIENT, g);
    } else {
        glClearColor(0.65f, 0.80f, 0.90f, 1.0f);
        GLfloat a[] = {0.45f,0.45f,0.45f,1}, d[]={0.85f,0.85f,0.85f,1};
        GLfloat s[] = {0.15f,0.15f,0.15f,1}, g[]={0.25f,0.25f,0.25f,1};
        glLightfv(GL_LIGHT0, GL_AMBIENT, a); glLightfv(GL_LIGHT0, GL_DIFFUSE, d);
        glLightfv(GL_LIGHT0, GL_SPECULAR, s); glLightModelfv(GL_LIGHT_MODEL_AMBIENT, g);
    }

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    int viewX = sidebarW, viewW = winW - sidebarW;
    if (viewW < 1) viewW = 1;
    glViewport(viewX, 0, viewW, winH);

    glMatrixMode(GL_PROJECTION); glLoadIdentity();
    gluPerspective(45, (float)viewW / (float)winH, 1, 200);

    glMatrixMode(GL_MODELVIEW); glLoadIdentity();
    gluLookAt(0, 9, -62, 0, 4.0, 0.0, 0, 1, 0);
    glTranslatef(0, 0, zoom);
    glRotatef(rotateX, 1, 0, 0);
    glRotatef(rotateY, 0, 1, 0);

    GLfloat lightPos[] = { 15, 30, -25, 1 };
    glLightfv(GL_LIGHT0, GL_POSITION, lightPos);

    drawGround(); drawGreenLand(); drawGrassLand(); drawGreenhouse();

    glViewport(0, 0, winW, winH);
    drawSidebar(); drawHeader(); drawFooter();

    glutSwapBuffers();
}

void reshape(int w, int h) {
    if (h == 0) h = 1;
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION); glLoadIdentity();
    gluPerspective(45, (float)w / h, 1, 200);
    glMatrixMode(GL_MODELVIEW);
}

int main(int argc, char** argv)
{
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(1300, 780);
    glutCreateWindow("AgriSense AI - Greenhouse Dashboard");

    glClearColor(0.65, 0.80, 0.90, 1.0);
    glEnable(GL_DEPTH_TEST);
    glShadeModel(GL_SMOOTH);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
    glEnable(GL_NORMALIZE);

    GLfloat lightPos[]      = { 15, 30, -25, 1 };
    GLfloat lightAmbient[]  = { 0.45f, 0.45f, 0.45f, 1 };
    GLfloat lightDiffuse[]  = { 0.85f, 0.85f, 0.85f, 1 };
    GLfloat lightSpecular[] = { 0.15f, 0.15f, 0.15f, 1 };
    glLightfv(GL_LIGHT0, GL_POSITION, lightPos);
    glLightfv(GL_LIGHT0, GL_AMBIENT,  lightAmbient);
    glLightfv(GL_LIGHT0, GL_DIFFUSE,  lightDiffuse);
    glLightfv(GL_LIGHT0, GL_SPECULAR, lightSpecular);
    GLfloat globalAmbient[] = { 0.25f, 0.25f, 0.25f, 1 };
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, globalAmbient);

    for (int i = 0; i < 3; i++) zones[i] = Zone(i + 1);

    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutSpecialFunc(specialKeys);
    glutKeyboardFunc(keyboard);
    glutMouseFunc(mouse);

    glutMainLoop();
    return 0;
}