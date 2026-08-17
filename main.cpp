#include <windows.h>
#include <GL/glut.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define PI 3.14159265358979323846

// ==================== GLOBAL VARIABLES ====================
bool isNight = false;
bool isRaining = false;
bool showIntro = true;
bool isLightning = false;
int lightningTimer = 0;

// Emergency Mode Variables
bool isEmergency = false;
float ambX = 53.5f;       // Hospital entrance initial X
float ambY = 27.8f;       // Hospital entrance initial Y
int ambState = 0;         // 0: Parked, 2: Drive Down facing Front, 3: Drive Main Road Right, 4: Finished Pass

// Traffic Light State (Red = true, Green = false)
bool isTrafficRed = false;
int trafficTimer = 0;

// Stars
float starX[200], starY[200], starPhase[200];

// Clouds
float cloudPos[12] = { -10, 5, 20, 35, 50, 65, 80, 95, 110, 125, 140, 155 };
float cloudSpeed[12] = { 0.03f, 0.015f, 0.04f, 0.02f, 0.035f, 0.018f, 0.045f, 0.012f, 0.028f, 0.032f, 0.019f, 0.038f };
float cloudScale[12] = { 0.8f, 1.2f, 0.7f, 1.1f, 0.9f, 1.3f, 0.75f, 1.0f, 0.85f, 1.15f, 0.95f, 1.05f };

// Rain & Splashes
#define RAIN_COUNT 2500
#define SPLASH_COUNT 150
float rainX[RAIN_COUNT], rainY[RAIN_COUNT], rainSpeed[RAIN_COUNT];
float splashX[SPLASH_COUNT], splashY[SPLASH_COUNT], splashRadius[SPLASH_COUNT], splashAlpha[SPLASH_COUNT];

// Birds & Fishing Animation
float flockX = -20.0f;
float wingAngle = 0.0f;
bool wingUp = true;
float fishJumpY = 0.0f;
float fishJumpX = 0.0f;
bool fishJumping = false;
float fishTimer = 0.0f;

// Dynamic Smoke Animation Array
#define SMOKE_PARTICLES 5
float smokeOffset[SMOKE_PARTICLES] = { 0.0f, 0.8f, 1.6f, 2.4f, 3.2f };

// Vehicles & Movements
float boatX = -35.0f;
int boatDir = 1;
int boatDockWaitTimer = 0;

float carX = 10.0f;
float busX = -25.0f;
float schoolBusX = -65.0f;

// Zebra Pedestrians System (Identical for Day & Rain Mode)
float ped1X = 0.0f;
float ped1Y = 22.2f;
int ped1State = 0; // 0: Top sidewalk right, 1: Wait top zebra, 2: Cross vertically down, 3: Bottom sidewalk right

float ped2X = 100.0f;
float ped2Y = 13.7f;
int ped2State = 0; // 0: Bottom sidewalk left, 1: Wait bottom zebra, 2: Cross vertically up, 3: Top sidewalk left

float lowerPedX = 10.0f;
float nightPedX = 0.0f; // Moving pedestrian for night mode

// Hospital Pedestrian Path Setup
float hospPedX = 0.0f;
float hospPedY = 22.2f;
int hospPedState = 0;

// Animation Counters
float waveOffset = 0.0f;
float windmillAngle = 0.0f;

// ==================== INITIALIZATION ====================

void initStars() {
    for (int i = 0; i < 200; i++) {
        starX[i] = (rand() % 10000) / 100.0f;
        starY[i] = 42.0f + (rand() % 3800) / 100.0f;
        starPhase[i] = (rand() % 360) * PI / 180.0f;
    }
}

void initRain() {
    for (int i = 0; i < RAIN_COUNT; i++) {
        rainX[i] = -10.0f + (rand() % 14000) / 100.0f;
        rainY[i] = (rand() % 8500) / 100.0f;
        rainSpeed[i] = 2.5f + (rand() % 150) / 100.0f;
    }
    for (int i = 0; i < SPLASH_COUNT; i++) {
        splashAlpha[i] = 0.0f;
    }
}

void init() {
    srand((unsigned int)time(NULL));
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0.0, 100.0, 0.0, 80.0);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_LINE_SMOOTH);
    glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
    glEnable(GL_POINT_SMOOTH);
    glHint(GL_POINT_SMOOTH_HINT, GL_NICEST);

    initStars();
    initRain();
}

// ==================== BASIC DRAWINGS & HELPER ====================

void drawCircle(float cx, float cy, float r, int num_segments) {
    glBegin(GL_POLYGON);
    for (int i = 0; i < num_segments; i++) {
        float theta = 2.0f * PI * float(i) / float(num_segments);
        glVertex2f(r * cosf(theta) + cx, r * sinf(theta) + cy);
    }
    glEnd();
}

void drawHollowCircle(float cx, float cy, float r, int num_segments) {
    glBegin(GL_LINE_LOOP);
    for (int i = 0; i < num_segments; i++) {
        float theta = 2.0f * PI * float(i) / float(num_segments);
        glVertex2f(r * cosf(theta) + cx, r * sinf(theta) + cy);
    }
    glEnd();
}

void drawGroundShadow(float x, float y, float rx, float ry) {
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColor4f(0.0f, 0.0f, 0.0f, 0.25f);
    glBegin(GL_POLYGON);
    for (int i = 0; i < 20; i++) {
        float theta = 2.0f * PI * float(i) / 20.0f;
        glVertex2f(x + rx * cosf(theta), y + ry * sinf(theta));
    }
    glEnd();
}

void renderText(float x, float y, const char* string, void* font) {
    glRasterPos2f(x, y);
    for (const char* c = string; *c != '\0'; c++) {
        glutBitmapCharacter(font, *c);
    }
}

// ==================== ENVIRONMENT ====================

void drawGradientSky() {
    glBegin(GL_QUADS);
    if (isNight) {
        glColor3f(0.02f, 0.03f, 0.08f); glVertex2f(0, 80);
        glColor3f(0.03f, 0.05f, 0.12f); glVertex2f(100, 80);
        glColor3f(0.08f, 0.12f, 0.22f); glVertex2f(100, 40);
        glColor3f(0.05f, 0.08f, 0.18f); glVertex2f(0, 40);
    } else if (isRaining) {
        if (isLightning) {
            glColor3f(0.92f, 0.94f, 1.00f); glVertex2f(0, 80);
            glColor3f(0.96f, 0.98f, 1.00f); glVertex2f(100, 80);
            glColor3f(0.80f, 0.85f, 0.95f); glVertex2f(100, 40);
            glColor3f(0.75f, 0.80f, 0.90f); glVertex2f(0, 40);
        } else {
            glColor3f(0.15f, 0.20f, 0.25f); glVertex2f(0, 80);
            glColor3f(0.20f, 0.25f, 0.30f); glVertex2f(100, 80);
            glColor3f(0.32f, 0.38f, 0.44f); glVertex2f(100, 40);
            glColor3f(0.28f, 0.32f, 0.38f); glVertex2f(0, 40);
        }
    } else {
        glColor3f(0.12f, 0.45f, 0.82f); glVertex2f(0, 80);
        glColor3f(0.18f, 0.52f, 0.88f); glVertex2f(100, 80);
        glColor3f(0.68f, 0.85f, 0.98f); glVertex2f(100, 40);
        glColor3f(0.60f, 0.80f, 0.95f); glVertex2f(0, 40);
    }
    glEnd();
}

void drawMountains() {
    glBegin(GL_TRIANGLES);
    if (isNight) glColor3f(0.03f, 0.05f, 0.10f);
    else if (isRaining) glColor3f(0.22f, 0.28f, 0.30f);
    else glColor3f(0.30f, 0.45f, 0.55f);

    glVertex2f(-10, 40); glVertex2f(15, 62); glVertex2f(38, 40);
    glVertex2f(20, 40); glVertex2f(50, 68); glVertex2f(80, 40);
    glVertex2f(60, 40); glVertex2f(88, 60); glVertex2f(115, 40);

    if (isNight) glColor3f(0.04f, 0.07f, 0.12f);
    else if (isRaining) glColor3f(0.16f, 0.22f, 0.20f);
    else glColor3f(0.20f, 0.38f, 0.26f);

    glVertex2f(-5, 40); glVertex2f(28, 55); glVertex2f(55, 40);
    glVertex2f(42, 40); glVertex2f(72, 58); glVertex2f(105, 40);
    glEnd();
}

void drawSun() {
    if (isNight || isRaining) return;
    float x = 85.0f, y = 68.0f, r = 5.2f;

    for (int i = 6; i > 0; i--) {
        glColor4f(1.0f, 0.88f, 0.35f, 0.03f * i);
        drawCircle(x, y, r + i * 1.2f, 40);
    }

    glColor3f(1.0f, 0.85f, 0.20f);
    drawCircle(x, y, r, 40);
}

void drawMoon() {
    if (!isNight) return;

    float mx = 18.0f, my = 68.0f;
    float r = 2.8f;

    glColor4f(0.9f, 0.95f, 1.0f, 0.05f);
    drawCircle(mx, my, r * 2.2f, 35);
    glColor4f(0.9f, 0.95f, 1.0f, 0.10f);
    drawCircle(mx, my, r * 1.5f, 35);

    glColor3f(0.95f, 0.96f, 0.98f);
    drawCircle(mx, my, r, 35);

    glColor3f(0.03f, 0.05f, 0.12f);
    drawCircle(mx + 1.1f, my + 0.6f, r * 0.92f, 35);
}

void drawStars() {
    if (!isNight) return;
    float time = glutGet(GLUT_ELAPSED_TIME) * 0.001f;
    glPointSize(2.2f);
    glBegin(GL_POINTS);
    for (int i = 0; i < 200; i++) {
        float twinkle = 0.3f + 0.7f * (0.5f + 0.5f * sin(time * 3.5f + starPhase[i]));
        glColor4f(1.0f, 1.0f, 1.0f, twinkle);
        glVertex2f(starX[i], starY[i]);
    }
    glEnd();
}

void drawClouds() {
    if (isNight) return;

    for (int c = 0; c < 12; c++) {
        float cx = cloudPos[c];
        float cy = 62.0f + sin(c * 1.8f) * 4.5f;
        float sc = cloudScale[c];

        glPushMatrix();
        glTranslatef(cx, cy, 0);
        glScalef(sc, sc, 1);

        if (isRaining) glColor4f(0.28f, 0.32f, 0.38f, 0.92f);
        else glColor4f(0.98f, 0.98f, 1.0f, 0.88f);

        drawCircle(0, 0, 3.2f, 20);
        drawCircle(2.5f, 0.9f, 2.5f, 20);
        drawCircle(-2.2f, 0.4f, 2.2f, 20);
        drawCircle(4.5f, -0.2f, 2.0f, 20);

        glPopMatrix();
    }
}

void drawGround() {
    glBegin(GL_QUADS);
    if (isNight) {
        glColor3f(0.02f, 0.06f, 0.03f); glVertex2f(0, 40);
        glColor3f(0.03f, 0.08f, 0.04f); glVertex2f(100, 40);
        glColor3f(0.01f, 0.05f, 0.02f); glVertex2f(100, 22);
        glColor3f(0.01f, 0.04f, 0.01f); glVertex2f(0, 22);
    } else if (isRaining) {
        glColor3f(0.18f, 0.35f, 0.18f); glVertex2f(0, 40);
        glColor3f(0.22f, 0.38f, 0.20f); glVertex2f(100, 40);
        glColor3f(0.15f, 0.28f, 0.14f); glVertex2f(100, 22);
        glColor3f(0.12f, 0.24f, 0.12f); glVertex2f(0, 22);
    } else {
        glColor3f(0.22f, 0.52f, 0.22f); glVertex2f(0, 40);
        glColor3f(0.26f, 0.58f, 0.25f); glVertex2f(100, 40);
        glColor3f(0.18f, 0.45f, 0.18f); glVertex2f(100, 22);
        glColor3f(0.15f, 0.38f, 0.15f); glVertex2f(0, 22);
    }
    glEnd();
}

void drawBroadGreenSpace() {
    glBegin(GL_QUADS);
    if (isNight) glColor3f(0.01f, 0.04f, 0.01f);
    else if (isRaining) glColor3f(0.12f, 0.24f, 0.12f);
    else glColor3f(0.15f, 0.42f, 0.15f);

    glVertex2f(0, 13.0f); glVertex2f(100, 13.0f);
    glVertex2f(100, 8.0f); glVertex2f(0, 8.0f);
    glEnd();
}

// ==================== VEGETATION & SHADOWS ====================

void drawFlowerCluster(float x, float y) {
    if (isNight) return;
    glPushMatrix();
    glTranslatef(x, y, 0);

    glColor3f(0.12f, 0.42f, 0.12f);
    glLineWidth(1.2f);
    glBegin(GL_LINES);
    glVertex2f(0, 0); glVertex2f(-0.3f, 0.8f);
    glVertex2f(0, 0); glVertex2f(0.3f, 0.9f);
    glVertex2f(0, 0); glVertex2f(0.0f, 1.1f);
    glVertex2f(0, 0); glVertex2f(-0.5f, 0.6f);
    glVertex2f(0, 0); glVertex2f(0.5f, 0.7f);
    glEnd();

    glColor3f(0.90f, 0.25f, 0.35f); drawCircle(-0.3f, 0.8f, 0.22f, 10);
    glColor3f(0.95f, 0.80f, 0.15f); drawCircle(0.3f, 0.9f, 0.22f, 10);
    glColor3f(0.70f, 0.35f, 0.85f); drawCircle(0.0f, 1.1f, 0.22f, 10);
    glColor3f(0.95f, 0.40f, 0.10f); drawCircle(-0.5f, 0.6f, 0.18f, 10);
    glColor3f(0.20f, 0.70f, 0.90f); drawCircle(0.5f, 0.7f, 0.20f, 10);

    glPopMatrix();
}

void drawBeautifulTree(float x, float y, float scale) {
    drawGroundShadow(x + 0.3f, y - 0.1f, 1.8f * scale, 0.4f * scale);

    glPushMatrix();
    glTranslatef(x, y, 0);
    glScalef(scale, scale, 1);

    glColor3f(0.28f, 0.16f, 0.08f);
    glBegin(GL_QUADS);
    glVertex2f(-0.4f, 0); glVertex2f(0.4f, 0);
    glVertex2f(0.28f, 4.5f); glVertex2f(-0.28f, 4.5f);
    glEnd();

    float g = isNight ? 0.08f : (isRaining ? 0.30f : 0.48f);

    glColor3f(0.02f, g - 0.05f, 0.02f); drawCircle(0.0f, 6.0f, 3.2f, 25);
    glColor3f(0.03f, g, 0.03f); drawCircle(-1.6f, 5.0f, 2.5f, 25); drawCircle(1.6f, 5.0f, 2.5f, 25);
    glColor3f(0.05f, g + 0.06f, 0.05f); drawCircle(-1.0f, 6.5f, 2.2f, 25); drawCircle(1.0f, 6.5f, 2.2f, 25); drawCircle(0.0f, 7.5f, 2.4f, 25);

    glPopMatrix();
}

void drawPineTree(float x, float y, float scale) {
    drawGroundShadow(x + 0.2f, y - 0.1f, 1.5f * scale, 0.35f * scale);

    glPushMatrix();
    glTranslatef(x, y, 0);
    glScalef(scale, scale, 1);

    glColor3f(0.22f, 0.12f, 0.06f);
    glBegin(GL_QUADS);
    glVertex2f(-0.3f, 0); glVertex2f(0.3f, 0);
    glVertex2f(0.2f, 3.0f); glVertex2f(-0.2f, 3.0f);
    glEnd();

    float g = isNight ? 0.08f : (isRaining ? 0.32f : 0.50f);
    glColor3f(0.03f, g, 0.08f);

    glBegin(GL_TRIANGLES);
    glVertex2f(-2.5f, 2.5f); glVertex2f(0.0f, 5.5f); glVertex2f(2.5f, 2.5f);
    glVertex2f(-2.0f, 4.5f); glVertex2f(0.0f, 7.2f); glVertex2f(2.0f, 4.5f);
    glVertex2f(-1.4f, 6.5f); glVertex2f(0.0f, 9.0f); glVertex2f(1.4f, 6.5f);
    glEnd();

    glPopMatrix();
}

void drawBush(float x, float y, float scale) {
    glPushMatrix();
    glTranslatef(x, y, 0);
    glScalef(scale, scale, 1);

    float g = isNight ? 0.08f : (isRaining ? 0.30f : 0.45f);
    glColor3f(0.05f, g, 0.05f);
    drawCircle(0, 0, 1.2f, 15); drawCircle(-0.9f, -0.2f, 0.9f, 15); drawCircle(0.9f, -0.2f, 0.9f, 15);

    if (!isNight && !isRaining) {
        glColor3f(0.90f, 0.2f, 0.25f);
        drawCircle(-0.4f, 0.4f, 0.18f, 10); drawCircle(0.5f, 0.2f, 0.18f, 10); drawCircle(0.0f, -0.3f, 0.18f, 10);
    }
    glPopMatrix();
}

// ==================== SMART CITY & VILLAGE ADDITIONS ====================

void drawTrafficSignal(float x, float y) {
    glPushMatrix();
    glTranslatef(x, y, 0);

    glColor3f(0.2f, 0.2f, 0.2f);
    glBegin(GL_QUADS);
    glVertex2f(-0.15f, 0); glVertex2f(0.15f, 0);
    glVertex2f(0.15f, 5.0f); glVertex2f(-0.15f, 5.0f);
    glEnd();

    glColor3f(0.1f, 0.1f, 0.1f);
    glBegin(GL_QUADS);
    glVertex2f(-0.6f, 5.0f); glVertex2f(0.6f, 5.0f);
    glVertex2f(0.6f, 7.5f); glVertex2f(-0.6f, 7.5f);
    glEnd();

    if (isTrafficRed || isEmergency) glColor3f(1.0f, 0.0f, 0.0f);
    else glColor3f(0.3f, 0.0f, 0.0f);
    drawCircle(0.0f, 6.8f, 0.4f, 15);

    if (!isTrafficRed && !isEmergency) glColor3f(0.0f, 1.0f, 0.0f);
    else glColor3f(0.0f, 0.3f, 0.0f);
    drawCircle(0.0f, 5.7f, 0.4f, 15);

    glPopMatrix();
}

void drawZebraCrossing(float x) {
    glColor3f(0.98f, 0.98f, 0.98f);
    for (float y = 15.3f; y < 21.7f; y += 1.1f) {
        glBegin(GL_QUADS);
        glVertex2f(x - 2.5f, y); glVertex2f(x + 2.5f, y);
        glVertex2f(x + 2.5f, y + 0.65f); glVertex2f(x - 2.5f, y + 0.65f);
        glEnd();
    }
}

void drawBusStop(float x, float y) {
    glPushMatrix();
    glTranslatef(x, y, 0);

    glColor3f(0.2f, 0.3f, 0.4f);
    glBegin(GL_QUADS);
    glVertex2f(-3.0f, 0); glVertex2f(-2.8f, 0);
    glVertex2f(-2.8f, 3.5f); glVertex2f(-3.0f, 3.5f);
    glVertex2f(2.8f, 0); glVertex2f(3.0f, 0);
    glVertex2f(3.0f, 3.5f); glVertex2f(2.8f, 3.5f);
    glEnd();

    glColor3f(0.8f, 0.2f, 0.2f);
    glBegin(GL_QUADS);
    glVertex2f(-3.5f, 3.5f); glVertex2f(3.5f, 3.5f);
    glVertex2f(3.2f, 4.0f); glVertex2f(-3.2f, 4.0f);
    glEnd();

    glColor3f(0.4f, 0.25f, 0.1f);
    glBegin(GL_QUADS);
    glVertex2f(-2.2f, 0.8f); glVertex2f(2.2f, 0.8f);
    glVertex2f(2.2f, 1.1f); glVertex2f(-2.2f, 1.1f);
    glEnd();

    glPopMatrix();
}

void drawCCTV(float x, float y, bool faceLeft) {
    glPushMatrix();
    glTranslatef(x, y, 0);
    if (faceLeft) glScalef(-1, 1, 1);

    glColor3f(0.1f, 0.1f, 0.1f);
    glBegin(GL_QUADS);
    glVertex2f(0, 0); glVertex2f(0.4f, 0.2f);
    glVertex2f(0.4f, 0.4f); glVertex2f(0, 0.2f);
    glEnd();

    glColor3f(0.9f, 0.9f, 0.9f);
    glBegin(GL_POLYGON);
    glVertex2f(0.3f, 0.1f); glVertex2f(1.0f, 0.1f);
    glVertex2f(1.1f, 0.5f); glVertex2f(0.3f, 0.5f);
    glEnd();

    glColor3f(0.1f, 0.1f, 0.1f);
    drawCircle(1.05f, 0.3f, 0.15f, 10);

    glPopMatrix();
}

void drawDustbin(float x, float y) {
    glPushMatrix();
    glTranslatef(x, y, 0);

    glColor3f(0.1f, 0.6f, 0.2f);
    glBegin(GL_QUADS);
    glVertex2f(-0.3f, 0); glVertex2f(0.3f, 0);
    glVertex2f(0.4f, 1.0f); glVertex2f(-0.4f, 1.0f);
    glEnd();

    glColor3f(0.0f, 0.4f, 0.1f);
    glBegin(GL_QUADS);
    glVertex2f(-0.5f, 1.0f); glVertex2f(0.5f, 1.0f);
    glVertex2f(0.4f, 1.2f); glVertex2f(-0.4f, 1.2f);
    glEnd();

    glPopMatrix();
}

void drawDirectionBoard(float x, float y) {
    glPushMatrix();
    glTranslatef(x, y, 0);

    glColor3f(0.3f, 0.2f, 0.1f);
    glBegin(GL_QUADS);
    glVertex2f(-0.1f, 0); glVertex2f(0.1f, 0);
    glVertex2f(0.1f, 3.5f); glVertex2f(-0.1f, 3.5f);
    glEnd();

    glColor3f(0.1f, 0.5f, 0.2f);
    glBegin(GL_POLYGON);
    glVertex2f(-2.5f, 2.6f); glVertex2f(0.8f, 2.6f);
    glVertex2f(0.8f, 3.4f); glVertex2f(-2.5f, 3.4f);
    glEnd();
    glColor3f(1.0f, 1.0f, 1.0f);
    renderText(-2.3f, 2.8f, "< Market", GLUT_BITMAP_HELVETICA_10);

    glColor3f(0.8f, 0.2f, 0.2f);
    glBegin(GL_POLYGON);
    glVertex2f(-0.8f, 1.6f); glVertex2f(2.8f, 1.6f);
    glVertex2f(2.8f, 2.4f); glVertex2f(-0.8f, 2.4f);
    glEnd();
    glColor3f(1.0f, 1.0f, 1.0f);
    renderText(-0.6f, 1.8f, "Hospital >", GLUT_BITMAP_HELVETICA_10);

    glPopMatrix();
}

void drawBench(float x, float y) {
    glPushMatrix();
    glTranslatef(x, y, 0);

    glColor3f(0.2f, 0.2f, 0.2f);
    glLineWidth(2.0f);
    glBegin(GL_LINES);
    glVertex2f(-1.0f, 0); glVertex2f(-1.0f, 0.8f);
    glVertex2f(1.0f, 0); glVertex2f(1.0f, 0.8f);
    glVertex2f(-1.0f, 0.8f); glVertex2f(-1.0f, 1.5f);
    glVertex2f(1.0f, 0.8f); glVertex2f(1.0f, 1.5f);
    glEnd();

    glColor3f(0.5f, 0.3f, 0.1f);
    glBegin(GL_QUADS);
    glVertex2f(-1.2f, 0.8f); glVertex2f(1.2f, 0.8f);
    glVertex2f(1.2f, 1.0f); glVertex2f(-1.2f, 1.0f);
    glVertex2f(-1.2f, 1.2f); glVertex2f(1.2f, 1.2f);
    glVertex2f(1.2f, 1.4f); glVertex2f(-1.2f, 1.4f);
    glEnd();

    glPopMatrix();
}

void drawFence(float startX, float endX, float y) {
    glColor3f(0.8f, 0.8f, 0.8f);
    glLineWidth(1.5f);
    glBegin(GL_LINES);
    glVertex2f(startX, y + 0.4f); glVertex2f(endX, y + 0.4f);
    glVertex2f(startX, y + 0.9f); glVertex2f(endX, y + 0.9f);
    glEnd();

    for (float x = startX; x <= endX; x += 0.8f) {
        glBegin(GL_TRIANGLES);
        glVertex2f(x - 0.1f, y); glVertex2f(x + 0.1f, y); glVertex2f(x, y + 1.2f);
        glEnd();
    }
}

void drawVillageBoard(float x, float y) {
    glPushMatrix();
    glTranslatef(x, y, 0);

    glColor3f(0.3f, 0.18f, 0.05f);
    glBegin(GL_QUADS);
    glVertex2f(-0.2f, 0); glVertex2f(0.2f, 0);
    glVertex2f(0.2f, 4.0f); glVertex2f(-0.2f, 4.0f);
    glVertex2f(8.8f, 0); glVertex2f(9.2f, 0);
    glVertex2f(9.2f, 4.0f); glVertex2f(8.8f, 4.0f);
    glEnd();

    glColor3f(0.1f, 0.4f, 0.1f);
    glBegin(GL_QUADS);
    glVertex2f(-0.8f, 2.0f); glVertex2f(9.8f, 2.0f);
    glVertex2f(9.8f, 4.8f); glVertex2f(-0.8f, 4.8f);
    glEnd();

    glColor3f(0.9f, 0.9f, 0.9f);
    glLineWidth(1.5f);
    glBegin(GL_LINE_LOOP);
    glVertex2f(-0.6f, 2.2f); glVertex2f(9.6f, 2.2f);
    glVertex2f(9.6f, 4.6f); glVertex2f(-0.6f, 4.6f);
    glEnd();

    glColor3f(1.0f, 1.0f, 1.0f);
    renderText(1.2f, 3.6f, "WELCOME TO", GLUT_BITMAP_HELVETICA_10);
    renderText(0.2f, 2.6f, "SMART VILLAGE", GLUT_BITMAP_HELVETICA_12);

    glPopMatrix();
}

// ==================== SMART PHARMACY ====================

void drawSmartPharmacy(float x, float y) {
    glPushMatrix();
    glTranslatef(x, y, 0);

    glColor3f(0.30f, 0.33f, 0.36f);
    glBegin(GL_QUADS);
    glVertex2f(-3.5f, -0.4f); glVertex2f(3.5f, -0.4f);
    glVertex2f(3.2f, 0.0f); glVertex2f(-3.2f, 0.0f);
    glEnd();

    if (isNight) glColor3f(0.12f, 0.22f, 0.25f);
    else glColor3f(0.85f, 0.92f, 0.95f);

    glBegin(GL_QUADS);
    glVertex2f(-3.2f, 0.0f); glVertex2f(3.2f, 0.0f);
    glVertex2f(3.2f, 6.5f); glVertex2f(-3.2f, 6.5f);
    glEnd();

    glColor3f(0.1f, 0.2f, 0.3f);
    glBegin(GL_QUADS);
    glVertex2f(-1.2f, 0.0f); glVertex2f(1.2f, 0.0f);
    glVertex2f(1.2f, 2.8f); glVertex2f(-1.2f, 2.8f);
    glEnd();

    glColor3f(0.0f, 0.6f, 0.5f);
    glLineWidth(1.5f);
    glBegin(GL_LINE_LOOP);
    glVertex2f(-1.2f, 0.0f); glVertex2f(1.2f, 0.0f);
    glVertex2f(1.2f, 2.8f); glVertex2f(-1.2f, 2.8f);
    glEnd();

    for (float wx = -2.6f; wx <= 2.0f; wx += 3.6f) {
        if (isNight) glColor3f(0.98f, 0.85f, 0.35f);
        else glColor3f(0.40f, 0.75f, 0.85f);

        glBegin(GL_QUADS);
        glVertex2f(wx, 3.4f); glVertex2f(wx + 1.2f, 3.4f);
        glVertex2f(wx + 1.2f, 4.8f); glVertex2f(wx, 4.8f);
        glEnd();
    }

    // Signboard kept strictly within pharmacy building bounds (-3.0 to 3.0)
    glColor3f(0.0f, 0.55f, 0.45f);
    glBegin(GL_QUADS);
    glVertex2f(-3.0f, 5.2f); glVertex2f(3.0f, 5.2f);
    glVertex2f(3.0f, 6.4f); glVertex2f(-3.0f, 6.4f);
    glEnd();

    glColor3f(0.0f, 0.95f, 0.45f);
    glBegin(GL_QUADS);
    glVertex2f(-2.6f, 5.4f); glVertex2f(-2.2f, 5.4f);
    glVertex2f(-2.2f, 6.2f); glVertex2f(-2.6f, 6.2f);
    glVertex2f(-2.8f, 5.7f); glVertex2f(-2.0f, 5.7f);
    glVertex2f(-2.0f, 5.9f); glVertex2f(-2.8f, 5.9f);
    glEnd();

    glColor3f(1.0f, 1.0f, 1.0f);
    renderText(-2.85f, 5.6f, "24/7 PHARMACY", GLUT_BITMAP_HELVETICA_10);

    glPopMatrix();
}

// ==================== HOSPITAL & AMBULANCES ====================

void drawAmbulanceSide(float x, float y) {
    glPushMatrix();
    glTranslatef(x, y, 0);
    glScalef(1.25f, 1.25f, 1.0f);

    drawGroundShadow(0.0f, 0.1f, 3.2f, 0.3f);

    glColor3f(0.1f, 0.1f, 0.1f);
    glLineWidth(2.0f);
    glBegin(GL_LINE_LOOP);
    glVertex2f(-3.1f, 0.15f); glVertex2f(2.9f, 0.15f);
    glVertex2f(2.9f, 1.7f); glVertex2f(1.9f, 2.45f);
    glVertex2f(-3.1f, 2.45f);
    glEnd();

    glColor3f(0.94f, 0.94f, 0.96f);
    glBegin(GL_POLYGON);
    glVertex2f(-3.0f, 0.2f); glVertex2f(2.8f, 0.2f);
    glVertex2f(2.8f, 1.65f); glVertex2f(1.8f, 2.4f);
    glVertex2f(-3.0f, 2.4f);
    glEnd();

    glColor3f(0.88f, 0.12f, 0.12f);
    glBegin(GL_QUADS);
    glVertex2f(-3.0f, 0.8f); glVertex2f(2.75f, 0.8f);
    glVertex2f(2.75f, 1.25f); glVertex2f(-3.0f, 1.25f);
    glEnd();

    glColor4f(0.12f, 0.22f, 0.32f, 0.85f);
    glBegin(GL_QUADS);
    glVertex2f(0.6f, 1.45f); glVertex2f(1.6f, 1.45f);
    glVertex2f(1.45f, 2.2f); glVertex2f(0.6f, 2.2f);
    glVertex2f(-1.1f, 1.45f); glVertex2f(-0.1f, 1.45f);
    glVertex2f(-0.1f, 2.2f); glVertex2f(-1.1f, 2.2f);
    glEnd();

    glColor3f(0.88f, 0.12f, 0.12f);
    glBegin(GL_QUADS);
    glVertex2f(-2.1f, 1.4f); glVertex2f(-1.7f, 1.4f);
    glVertex2f(-1.7f, 2.2f); glVertex2f(-2.1f, 2.2f);
    glVertex2f(-2.5f, 1.65f); glVertex2f(-1.3f, 1.65f);
    glVertex2f(-1.3f, 1.95f); glVertex2f(-2.5f, 1.95f);
    glEnd();

    glColor3f(0.12f, 0.12f, 0.12f);
    drawCircle(-1.8f, 0.25f, 0.6f, 20);
    drawCircle(1.8f, 0.25f, 0.6f, 20);

    glColor3f(0.75f, 0.75f, 0.80f);
    drawCircle(-1.8f, 0.25f, 0.3f, 15);
    drawCircle(1.8f, 0.25f, 0.3f, 15);

    glColor3f(1.0f, 0.90f, 0.3f);
    drawCircle(2.8f, 1.0f, 0.20f, 12);

    float timeVal = glutGet(GLUT_ELAPSED_TIME) * (isEmergency ? 0.025f : 0.008f);
    bool flashToggle = ((int)timeVal % 2) == 0;

    glColor3f(flashToggle ? 0.95f : 0.20f, 0.0f, 0.0f);
    glBegin(GL_QUADS);
    glVertex2f(-0.4f, 2.4f); glVertex2f(0.0f, 2.4f);
    glVertex2f(0.0f, 2.7f); glVertex2f(-0.4f, 2.7f);
    glEnd();

    glColor3f(0.0f, 0.20f, flashToggle ? 0.20f : 0.95f);
    glBegin(GL_QUADS);
    glVertex2f(0.0f, 2.4f); glVertex2f(0.4f, 2.4f);
    glVertex2f(0.4f, 2.7f); glVertex2f(0.0f, 2.7f);
    glEnd();

    glPopMatrix();
}

void drawAmbulanceVertical(float x, float y) {
    glPushMatrix();
    glTranslatef(x, y, 0);

    drawGroundShadow(0.0f, 0.0f, 1.6f, 2.2f);

    glColor3f(0.94f, 0.94f, 0.96f);
    glBegin(GL_POLYGON);
    glVertex2f(-1.5f, -2.0f); glVertex2f(1.5f, -2.0f);
    glVertex2f(1.6f, 1.8f);   glVertex2f(-1.6f, 1.8f);
    glEnd();

    glColor3f(0.88f, 0.12f, 0.12f);
    glBegin(GL_QUADS);
    glVertex2f(-1.6f, -1.8f); glVertex2f(-1.3f, -1.8f);
    glVertex2f(-1.3f, 1.6f);  glVertex2f(-1.6f, 1.6f);
    glVertex2f(1.3f, -1.8f);  glVertex2f(1.6f, -1.8f);
    glVertex2f(1.6f, 1.6f);   glVertex2f(1.3f, 1.6f);
    glEnd();

    glColor4f(0.12f, 0.22f, 0.32f, 0.85f);
    glBegin(GL_QUADS);
    glVertex2f(-1.2f, -1.4f); glVertex2f(1.2f, -1.4f);
    glVertex2f(1.1f, -0.6f);  glVertex2f(-1.1f, -0.6f);
    glEnd();

    glColor3f(0.88f, 0.12f, 0.12f);
    glBegin(GL_QUADS);
    glVertex2f(-0.25f, -0.4f); glVertex2f(0.25f, -0.4f);
    glVertex2f(0.25f, 0.4f);   glVertex2f(-0.25f, 0.4f);
    glVertex2f(-0.6f, -0.12f); glVertex2f(0.6f, -0.12f);
    glVertex2f(0.6f, 0.12f);   glVertex2f(-0.6f, 0.12f);
    glEnd();

    glColor3f(1.0f, 0.92f, 0.35f);
    drawCircle(-1.2f, -1.9f, 0.25f, 10);
    drawCircle(1.2f, -1.9f, 0.25f, 10);

    float timeVal = glutGet(GLUT_ELAPSED_TIME) * 0.025f;
    bool flashToggle = ((int)timeVal % 2) == 0;

    glColor3f(flashToggle ? 0.95f : 0.20f, 0.0f, 0.0f);
    glBegin(GL_QUADS);
    glVertex2f(-1.0f, 1.5f); glVertex2f(0.0f, 1.5f);
    glVertex2f(0.0f, 1.8f);  glVertex2f(-1.0f, 1.8f);
    glEnd();

    glColor3f(0.0f, 0.20f, flashToggle ? 0.20f : 0.95f);
    glBegin(GL_QUADS);
    glVertex2f(0.0f, 1.5f); glVertex2f(1.0f, 1.5f);
    glVertex2f(1.0f, 1.8f); glVertex2f(0.0f, 1.8f);
    glEnd();

    glColor3f(0.2f, 0.2f, 0.2f);
    glBegin(GL_QUADS);
    glVertex2f(-1.8f, -1.1f); glVertex2f(-1.5f, -1.1f);
    glVertex2f(-1.5f, -0.8f); glVertex2f(-1.8f, -0.8f);
    glVertex2f(1.5f, -1.1f);  glVertex2f(1.8f, -1.1f);
    glVertex2f(1.8f, -0.8f);  glVertex2f(1.5f, -0.8f);
    glEnd();

    glPopMatrix();
}

void drawHospital(float x, float y) {
    glPushMatrix();
    glTranslatef(x, y, 0);

    glColor3f(0.35f, 0.38f, 0.42f);
    glBegin(GL_QUADS);
    glVertex2f(-2.5f, -0.4f); glVertex2f(2.5f, -0.4f);
    glVertex2f(2.2f, 0.0f); glVertex2f(-2.2f, 0.0f);
    glEnd();

    if (isNight) glColor3f(0.18f, 0.22f, 0.28f);
    else glColor3f(0.85f, 0.88f, 0.92f);

    glBegin(GL_QUADS);
    glVertex2f(-10.0f, 0); glVertex2f(10.0f, 0);
    glVertex2f(10.0f, 11.5f); glVertex2f(-10.0f, 11.5f);
    glEnd();

    if (isNight) glColor3f(0.14f, 0.18f, 0.24f);
    else glColor3f(0.78f, 0.81f, 0.85f);

    glBegin(GL_QUADS);
    glVertex2f(-13.0f, 0); glVertex2f(-10.0f, 0);
    glVertex2f(-10.0f, 8.5f); glVertex2f(-13.0f, 8.5f);
    glVertex2f(13.0f, 0); glVertex2f(10.0f, 0);
    glVertex2f(10.0f, 8.5f); glVertex2f(13.0f, 8.5f);
    glEnd();

    glColor3f(0.18f, 0.28f, 0.38f);
    glBegin(GL_QUADS);
    glVertex2f(-10.5f, 11.5f); glVertex2f(10.5f, 11.5f);
    glVertex2f(10.0f, 12.3f); glVertex2f(-10.0f, 12.3f);
    glEnd();

    if (isNight) glColor3f(0.20f, 0.24f, 0.30f);
    else glColor3f(0.82f, 0.85f, 0.88f);

    glBegin(GL_QUADS);
    glVertex2f(-3.5f, 12.3f); glVertex2f(3.5f, 12.3f);
    glVertex2f(3.5f, 15.3f); glVertex2f(-3.5f, 15.3f);
    glEnd();

    glColor3f(0.15f, 0.30f, 0.45f);
    glBegin(GL_QUADS);
    glVertex2f(-2.5f, 0); glVertex2f(2.5f, 0);
    glVertex2f(2.5f, 4.0f); glVertex2f(-2.5f, 4.0f);
    glEnd();

    for (float wy = 4.8f; wy <= 9.2f; wy += 2.2f) {
        for (float wx = -8.5f; wx <= 7.5f; wx += 2.8f) {
            if (fabs(wx) < 1.8f && wy < 5.5f) continue;

            if (isNight) glColor3f(0.98f, 0.85f, 0.35f);
            else glColor3f(0.48f, 0.72f, 0.88f);

            glBegin(GL_QUADS);
            glVertex2f(wx, wy); glVertex2f(wx + 1.8f, wy);
            glVertex2f(wx + 1.8f, wy + 1.5f); glVertex2f(wx, wy + 1.5f);
            glEnd();

            glColor3f(0.1f, 0.15f, 0.2f);
            glLineWidth(1.0f);
            glBegin(GL_LINES);
            glVertex2f(wx + 0.9f, wy); glVertex2f(wx + 0.9f, wy + 1.5f);
            glVertex2f(wx, wy + 0.75f); glVertex2f(wx + 1.8f, wy + 0.75f);
            glEnd();
        }
    }

    glColor3f(0.88f, 0.12f, 0.12f);
    glBegin(GL_QUADS);
    glVertex2f(-0.6f, 13.2f); glVertex2f(0.6f, 13.2f);
    glVertex2f(0.6f, 14.8f); glVertex2f(-0.6f, 14.8f);
    glVertex2f(-1.4f, 13.7f); glVertex2f(1.4f, 13.7f);
    glVertex2f(1.4f, 14.3f); glVertex2f(-1.4f, 14.3f);
    glEnd();

    glColor3f(0.1f, 0.1f, 0.5f);
    glBegin(GL_QUADS);
    glVertex2f(-7.0f, 10.2f); glVertex2f(7.0f, 10.2f);
    glVertex2f(7.0f, 11.2f); glVertex2f(-7.0f, 11.2f);
    glEnd();
    glColor3f(1.0f, 1.0f, 1.0f);
    renderText(-2.8f, 10.5f, "HOSPITAL", GLUT_BITMAP_HELVETICA_12);

    for (float sx = -9.0f; sx <= 5.0f; sx += 4.5f) {
        glColor3f(0.05f, 0.15f, 0.35f);
        glBegin(GL_QUADS);
        glVertex2f(sx, 11.6f); glVertex2f(sx + 3.8f, 11.6f);
        glVertex2f(sx + 3.8f, 12.2f); glVertex2f(sx, 12.2f);
        glEnd();
        glColor3f(0.8f, 0.8f, 0.9f);
        glLineWidth(1.0f);
        glBegin(GL_LINE_LOOP);
        glVertex2f(sx, 11.6f); glVertex2f(sx + 3.8f, 11.6f);
        glVertex2f(sx + 3.8f, 12.2f); glVertex2f(sx, 12.2f);
        glEnd();
    }

    glColor3f(0.1f, 0.4f, 0.8f);
    glBegin(GL_QUADS);
    glVertex2f(-12.5f, 0.0f); glVertex2f(-10.5f, 0.0f);
    glVertex2f(-10.5f, 2.0f); glVertex2f(-12.5f, 2.0f);
    glEnd();
    glColor3f(1.0f, 1.0f, 1.0f);
    renderText(-11.8f, 0.7f, "P", GLUT_BITMAP_HELVETICA_18);

    glColor3f(0.8f, 0.1f, 0.1f);
    glBegin(GL_QUADS);
    glVertex2f(-10.0f, 2.2f); glVertex2f(-3.5f, 2.2f);
    glVertex2f(-3.5f, 3.4f); glVertex2f(-10.0f, 3.4f);
    glEnd();
    glColor3f(1.0f, 1.0f, 1.0f);
    renderText(-9.5f, 2.5f, "EMERGENCY", GLUT_BITMAP_HELVETICA_10);

    glPopMatrix();
}

void drawHospitalConnectingRoad() {
    if (isNight) glColor3f(0.08f, 0.08f, 0.10f);
    else glColor3f(0.25f, 0.25f, 0.27f);

    glBegin(GL_QUADS);
    glVertex2f(46.0f, 22.0f);
    glVertex2f(50.0f, 22.0f);
    glVertex2f(50.0f, 28.0f);
    glVertex2f(46.0f, 28.0f);
    glEnd();

    if (isNight) glColor3f(0.15f, 0.15f, 0.15f);
    else glColor3f(0.60f, 0.60f, 0.62f);

    glBegin(GL_QUADS);
    glVertex2f(45.5f, 22.0f); glVertex2f(46.0f, 22.0f);
    glVertex2f(46.0f, 28.0f); glVertex2f(45.5f, 28.0f);
    glVertex2f(50.0f, 22.0f); glVertex2f(50.5f, 22.0f);
    glVertex2f(50.5f, 28.0f); glVertex2f(50.0f, 28.0f);
    glEnd();
}

void drawVillageHouse(float x, float y, float scale, float r, float g, float b, bool isDoubleStorey) {
    glPushMatrix();
    glTranslatef(x, y, 0);
    glScalef(scale, scale, 1);

    if (isNight) glColor3f(r * 0.32f, g * 0.32f, b * 0.32f);
    else glColor3f(r, g, b);

    glBegin(GL_QUADS);
    glVertex2f(-4.5f, 0); glVertex2f(4.5f, 0);
    glVertex2f(4.5f, isDoubleStorey ? 6.5f : 4.2f);
    glVertex2f(-4.5f, isDoubleStorey ? 6.5f : 4.2f);
    glEnd();

    float roofBaseY = isDoubleStorey ? 6.5f : 4.2f;
    if (isNight) glColor3f(0.30f, 0.10f, 0.05f);
    else glColor3f(0.75f, 0.25f, 0.15f);

    glBegin(GL_TRIANGLES);
    glVertex2f(-5.2f, roofBaseY);
    glVertex2f(0.0f, roofBaseY + 3.0f);
    glVertex2f(5.2f, roofBaseY);
    glEnd();

    glColor3f(0.22f, 0.20f, 0.20f);
    glBegin(GL_QUADS);
    glVertex2f(2.0f, roofBaseY + 1.0f); glVertex2f(2.8f, roofBaseY + 1.0f);
    glVertex2f(2.8f, roofBaseY + 3.3f); glVertex2f(2.0f, roofBaseY + 3.3f);
    glEnd();

    if (!isRaining) {
        for (int p = 0; p < SMOKE_PARTICLES; p++) {
            float sy = smokeOffset[p];
            float sx = sin(sy * 1.5f) * 0.35f;
            float radius = 0.22f + (sy * 0.15f);
            float alpha = isNight ? (0.30f - (sy * 0.07f)) : (0.50f - (sy * 0.10f));

            if (alpha > 0.0f) {
                glColor4f(0.78f, 0.78f, 0.82f, alpha);
                drawCircle(2.4f + sx, roofBaseY + 3.4f + sy, radius, 16);
            }
        }
    }

    glColor3f(0.18f, 0.09f, 0.04f);
    glBegin(GL_QUADS);
    glVertex2f(-1.0f, 0); glVertex2f(1.0f, 0);
    glVertex2f(1.0f, 2.5f); glVertex2f(-1.0f, 2.5f);
    glEnd();

    if (isNight) glColor3f(0.98f, 0.85f, 0.35f);
    else glColor3f(0.42f, 0.68f, 0.88f);

    glBegin(GL_QUADS);
    glVertex2f(-3.5f, 1.8f); glVertex2f(-2.0f, 1.8f);
    glVertex2f(-2.0f, 3.2f); glVertex2f(-3.5f, 3.2f);
    glVertex2f(2.0f, 1.8f); glVertex2f(3.5f, 1.8f);
    glVertex2f(3.5f, 3.2f); glVertex2f(2.0f, 1.8f);
    glEnd();

    glColor3f(0.1f, 0.05f, 0.02f);
    glLineWidth(1.2f);
    glBegin(GL_LINES);
    glVertex2f(-2.75f, 1.8f); glVertex2f(-2.75f, 3.2f);
    glVertex2f(-3.5f, 2.5f);  glVertex2f(-2.0f, 2.5f);
    glVertex2f(2.75f, 1.8f);  glVertex2f(2.75f, 3.2f);
    glVertex2f(2.0f, 2.5f);   glVertex2f(3.5f, 2.5f);
    glEnd();

    glPopMatrix();
}

void drawLampPost(float x, float y) {
    glPushMatrix();
    glTranslatef(x, y, 0);

    glColor3f(0.12f, 0.14f, 0.16f);
    glBegin(GL_QUADS);
    glVertex2f(-0.35f, 0.0f); glVertex2f(0.35f, 0.0f);
    glVertex2f(0.25f, 0.6f); glVertex2f(-0.25f, 0.6f);
    glEnd();

    glColor3f(0.22f, 0.24f, 0.28f);
    glBegin(GL_QUADS);
    glVertex2f(-0.10f, 0.6f); glVertex2f(0.10f, 0.6f);
    glVertex2f(0.08f, 6.2f); glVertex2f(-0.08f, 6.2f);
    glEnd();

    glBegin(GL_QUADS);
    glVertex2f(-0.08f, 6.2f); glVertex2f(0.08f, 6.2f);
    glVertex2f(1.1f, 6.7f); glVertex2f(0.95f, 6.85f);
    glVertex2f(1.6f, 6.45f); glVertex2f(1.5f, 6.35f);
    glEnd();

    glColor3f(0.12f, 0.14f, 0.16f);
    glBegin(GL_POLYGON);
    glVertex2f(1.1f, 6.55f); glVertex2f(2.1f, 6.35f);
    glVertex2f(2.1f, 6.15f); glVertex2f(1.1f, 6.3f);
    glEnd();

    if (isNight) {
        glColor3f(1.0f, 0.98f, 0.85f);
        glBegin(GL_QUADS);
        glVertex2f(1.25f, 6.3f); glVertex2f(1.95f, 6.18f);
        glVertex2f(1.95f, 6.08f); glVertex2f(1.25f, 6.2f);
        glEnd();

        glBegin(GL_TRIANGLES);
        glColor4f(1.0f, 0.92f, 0.60f, 0.32f);
        glVertex2f(1.6f, 6.15f);
        glColor4f(1.0f, 0.92f, 0.55f, 0.0f);
        glVertex2f(-2.5f, -0.2f);
        glVertex2f(5.8f, -0.2f);
        glEnd();
    } else {
        glColor3f(0.70f, 0.72f, 0.75f);
        glBegin(GL_QUADS);
        glVertex2f(1.25f, 6.3f); glVertex2f(1.95f, 6.18f);
        glVertex2f(1.95f, 6.12f); glVertex2f(1.25f, 6.24f);
        glEnd();
    }

    glPopMatrix();
}

// ==================== ANGLER & FISHING ====================

void drawFishermanAndFish() {
    if (isNight || isRaining) return;

    glPushMatrix();
    glTranslatef(20.0f, 9.5f, 0);

    glColor3f(0.35f, 0.25f, 0.15f);
    drawCircle(0.0f, -0.2f, 0.6f, 10);

    glColor3f(0.15f, 0.25f, 0.45f);
    glLineWidth(2.5f);
    glBegin(GL_LINES);
    glVertex2f(0.0f, 0.5f); glVertex2f(-0.4f, -0.4f);
    glVertex2f(0.0f, 0.5f); glVertex2f(0.4f, -0.4f);
    glEnd();

    glColor3f(0.82f, 0.32f, 0.18f);
    glBegin(GL_QUADS);
    glVertex2f(-0.35f, 0.5f); glVertex2f(0.35f, 0.5f);
    glVertex2f(0.25f, 1.8f); glVertex2f(-0.25f, 1.8f);
    glEnd();

    glColor3f(0.88f, 0.68f, 0.52f);
    drawCircle(0.0f, 2.2f, 0.35f, 15);

    glColor3f(0.78f, 0.62f, 0.22f);
    glBegin(GL_TRIANGLES);
    glVertex2f(-1.1f, 2.3f); glVertex2f(0.0f, 3.1f); glVertex2f(1.1f, 2.3f);
    glEnd();

    glColor3f(0.2f, 0.1f, 0.05f);
    glLineWidth(2.0f);
    glBegin(GL_LINES);
    glVertex2f(0.2f, 1.2f); glVertex2f(8.5f, 3.2f);
    glEnd();

    glColor4f(0.9f, 0.9f, 0.9f, 0.5f);
    glLineWidth(1.0f);
    glBegin(GL_LINES);
    glVertex2f(8.5f, 3.2f); glVertex2f(8.5f, -4.0f);
    glEnd();

    glColor3f(0.9f, 0.1f, 0.1f);
    drawCircle(8.5f, -4.0f + sin(waveOffset * 2.0f) * 0.2f, 0.22f, 10);

    if (fishJumping) {
        glPushMatrix();
        glTranslatef(8.5f + fishJumpX, -4.0f + fishJumpY, 0);

        glColor3f(0.95f, 0.52f, 0.15f);
        drawCircle(0, 0, 0.50f, 15);

        glColor3f(0.85f, 0.22f, 0.12f);
        glBegin(GL_TRIANGLES);
        glVertex2f(-0.4f, 0.0f); glVertex2f(-0.9f, 0.40f); glVertex2f(-0.9f, -0.40f);
        glEnd();

        glPopMatrix();
    }

    glPopMatrix();
}

// ==================== ROAD & VEHICLES ====================

void drawBeautifulRoad() {
    if (isNight) glColor3f(0.12f, 0.12f, 0.14f);
    else glColor3f(0.65f, 0.65f, 0.68f);

    glBegin(GL_QUADS);
    glVertex2f(0, 22.0f); glVertex2f(100, 22.0f);
    glVertex2f(100, 22.8f); glVertex2f(0, 22.8f);
    glEnd();

    if (isNight) glColor3f(0.08f, 0.08f, 0.10f);
    else glColor3f(0.22f, 0.22f, 0.24f);

    glBegin(GL_QUADS);
    glVertex2f(0, 15.0f); glVertex2f(100, 15.0f);
    glVertex2f(100, 22.0f); glVertex2f(0, 22.0f);
    glEnd();

    glColor3f(1.0f, 1.0f, 1.0f);
    glLineWidth(3.0f);
    glBegin(GL_LINES);
    glVertex2f(42.0f, 15.0f); glVertex2f(42.0f, 22.0f);
    glEnd();

    if (isNight) glColor3f(0.65f, 0.65f, 0.48f);
    else glColor3f(0.92f, 0.92f, 0.92f);

    for (float x = 1; x < 100; x += 6.5f) {
        glBegin(GL_QUADS);
        glVertex2f(x, 18.3f); glVertex2f(x + 3.5f, 18.3f);
        glVertex2f(x + 3.5f, 18.7f); glVertex2f(x, 18.7f);
        glEnd();

        glColor3f(1.0f, 0.8f, 0.0f);
        drawCircle(x - 0.5f, 18.5f, 0.15f, 8);
        if (isNight) glColor3f(0.65f, 0.65f, 0.48f);
        else glColor3f(0.92f, 0.92f, 0.92f);
    }
}

void drawLowerPedestrianRoad() {
    if (isNight) glColor3f(0.18f, 0.18f, 0.20f);
    else glColor3f(0.70f, 0.70f, 0.72f);

    glBegin(GL_QUADS);
    glVertex2f(0, 13.0f); glVertex2f(100, 13.0f);
    glVertex2f(100, 15.0f); glVertex2f(0, 15.0f);
    glEnd();

    glColor3f(0.40f, 0.40f, 0.42f);
    glLineWidth(1.5f);
    glBegin(GL_LINES);
    glVertex2f(0, 15.0f); glVertex2f(100, 15.0f);
    glVertex2f(0, 13.0f); glVertex2f(100, 13.0f);
    glEnd();
}

void drawCar() {
    glPushMatrix();
    glTranslatef(carX, 16.5f, 0);

    if (isNight) glColor3f(0.48f, 0.06f, 0.06f);
    else glColor3f(0.82f, 0.15f, 0.15f);

    glBegin(GL_POLYGON);
    glVertex2f(-3.8f, 0.5f); glVertex2f(3.8f, 0.5f);
    glVertex2f(3.5f, 1.8f); glVertex2f(1.8f, 1.8f);
    glVertex2f(0.8f, 3.0f); glVertex2f(-2.2f, 3.0f);
    glVertex2f(-3.5f, 1.8f);
    glEnd();

    if (isNight) glColor3f(0.2f, 0.3f, 0.4f);
    else glColor3f(0.6f, 0.8f, 0.95f);

    glBegin(GL_QUADS);
    glVertex2f(-2.0f, 2.0f); glVertex2f(0.5f, 2.0f);
    glVertex2f(0.5f, 2.8f); glVertex2f(-1.7f, 2.8f);
    glEnd();

    glColor3f(0.12f, 0.12f, 0.12f);
    drawCircle(-2.2f, 0.5f, 0.7f, 20);
    drawCircle(2.2f, 0.5f, 0.7f, 20);

    if (isNight) {
        glBegin(GL_TRIANGLES);
        glColor4f(1.0f, 0.95f, 0.6f, 0.35f);
        glVertex2f(3.8f, 1.2f);
        glColor4f(1.0f, 0.95f, 0.6f, 0.0f);
        glVertex2f(10.0f, 0.2f); glVertex2f(10.0f, 2.2f);
        glEnd();
    }

    glPopMatrix();
}

void drawBus() {
    glPushMatrix();
    glTranslatef(busX, 17.0f, 0);

    if (isNight) glColor3f(0.52f, 0.40f, 0.06f);
    else glColor3f(0.90f, 0.68f, 0.12f);

    glBegin(GL_POLYGON);
    glVertex2f(-6.5f, 0.5f); glVertex2f(6.5f, 0.5f);
    glVertex2f(6.5f, 4.2f); glVertex2f(-6.2f, 4.2f);
    glVertex2f(-6.5f, 2.8f);
    glEnd();

    if (isNight) glColor3f(0.98f, 0.85f, 0.35f);
    else glColor3f(0.2f, 0.25f, 0.35f);

    for (float wx = -5.0f; wx <= 4.5f; wx += 2.2f) {
        glBegin(GL_QUADS);
        glVertex2f(wx, 2.2f); glVertex2f(wx + 1.6f, 2.2f);
        glVertex2f(wx + 1.6f, 3.6f); glVertex2f(wx, 3.6f);
        glEnd();
    }

    glColor3f(0.12f, 0.12f, 0.12f);
    drawCircle(-4.2f, 0.5f, 0.9f, 20);
    drawCircle(4.2f, 0.5f, 0.9f, 20);

    if (isNight) {
        glBegin(GL_TRIANGLES);
        glColor4f(1.0f, 0.95f, 0.6f, 0.35f);
        glVertex2f(6.5f, 1.5f);
        glColor4f(1.0f, 0.95f, 0.6f, 0.0f);
        glVertex2f(14.0f, 0.2f); glVertex2f(14.0f, 2.8f);
        glEnd();
    }

    glPopMatrix();
}

void drawSchoolBus() {
    if (isNight) return;

    glPushMatrix();
    glTranslatef(schoolBusX, 17.0f, 0);

    glColor3f(0.95f, 0.75f, 0.05f);
    glBegin(GL_POLYGON);
    glVertex2f(-6.0f, 0.5f); glVertex2f(6.0f, 0.5f);
    glVertex2f(6.0f, 4.0f); glVertex2f(-5.8f, 4.0f);
    glVertex2f(-6.0f, 2.5f);
    glEnd();

    glColor3f(0.1f, 0.1f, 0.1f);
    glBegin(GL_QUADS);
    glVertex2f(-6.0f, 1.5f); glVertex2f(6.0f, 1.5f);
    glVertex2f(6.0f, 1.8f); glVertex2f(-6.0f, 1.8f);
    glEnd();

    glColor3f(0.0f, 0.0f, 0.0f);
    renderText(-3.5f, 2.0f, "SCHOOL BUS", GLUT_BITMAP_HELVETICA_10);

    glColor3f(0.3f, 0.5f, 0.7f);

    for (float wx = -4.5f; wx <= 4.0f; wx += 2.0f) {
        glBegin(GL_QUADS);
        glVertex2f(wx, 2.5f); glVertex2f(wx + 1.4f, 2.5f);
        glVertex2f(wx + 1.4f, 3.6f); glVertex2f(wx, 3.6f);
        glEnd();
    }

    glColor3f(0.12f, 0.12f, 0.12f);
    drawCircle(-3.8f, 0.5f, 0.85f, 20);
    drawCircle(3.8f, 0.5f, 0.85f, 20);

    glPopMatrix();
}

// ==================== RIVER & EXTENDED DOCK ====================

void drawExtendedWoodenDock() {
    glColor3f(0.25f, 0.14f, 0.06f);
    glBegin(GL_QUADS);
    glVertex2f(88.0f, 0.0f); glVertex2f(89.4f, 0.0f); glVertex2f(89.4f, 4.0f); glVertex2f(88.0f, 4.0f);
    glVertex2f(93.0f, 0.0f); glVertex2f(94.4f, 0.0f); glVertex2f(94.4f, 4.0f); glVertex2f(93.0f, 4.0f);
    glVertex2f(98.0f, 0.0f); glVertex2f(99.4f, 0.0f); glVertex2f(99.4f, 4.0f); glVertex2f(98.0f, 4.0f);
    glEnd();

    glColor3f(0.42f, 0.26f, 0.12f);
    glBegin(GL_QUADS);
    glVertex2f(85.0f, 3.0f); glVertex2f(100.0f, 3.0f);
    glVertex2f(100.0f, 4.5f); glVertex2f(85.0f, 4.5f);
    glEnd();

    glColor3f(0.20f, 0.10f, 0.04f);
    glLineWidth(1.5f);
    glBegin(GL_LINES);
    for (float dx = 85.5f; dx < 100.0f; dx += 1.2f) {
        glVertex2f(dx, 3.0f); glVertex2f(dx, 4.5f);
    }
    glEnd();

    glColor3f(0.2f, 0.2f, 0.2f);
    glBegin(GL_QUADS);
    glVertex2f(87.0f, 4.5f); glVertex2f(87.5f, 4.5f);
    glVertex2f(87.5f, 5.2f); glVertex2f(87.0f, 5.2f);
    glEnd();
}

void drawRiver() {
    glBegin(GL_QUADS);
    if (isNight) {
        glColor3f(0.01f, 0.03f, 0.08f); glVertex2f(0, 8); glVertex2f(100, 8);
        glColor3f(0.00f, 0.01f, 0.04f); glVertex2f(100, 0); glVertex2f(0, 0);
    } else if (isRaining) {
        glColor3f(0.15f, 0.28f, 0.42f); glVertex2f(0, 8); glVertex2f(100, 8);
        glColor3f(0.08f, 0.16f, 0.28f); glVertex2f(100, 0); glVertex2f(0, 0);
    } else {
        glColor3f(0.15f, 0.45f, 0.72f); glVertex2f(0, 8); glVertex2f(100, 8);
        glColor3f(0.06f, 0.28f, 0.52f); glVertex2f(100, 0); glVertex2f(0, 0);
    }
    glEnd();

    glColor4f(1.0f, 1.0f, 1.0f, 0.22f);
    glLineWidth(1.2f);
    glBegin(GL_LINES);
    for (float rx = 2.0f; rx < 98.0f; rx += 5.0f) {
        float ry = 1.0f + fmod(rx * 1.5f + waveOffset * 2.5f, 6.0f);
        glVertex2f(rx, ry);
        glVertex2f(rx + 4.0f, ry);
    }
    glEnd();

    drawExtendedWoodenDock();
}

void drawBeautifulBoat() {
    if (isRaining || isNight) return;

    glPushMatrix();
    glTranslatef(boatX, 4.0f, 0);
    if (boatDir == 2) glScalef(-1, 1, 1);

    glColor3f(0.28f, 0.14f, 0.06f);
    glBegin(GL_POLYGON);
    glVertex2f(-7.0f, 0.9f);  glVertex2f(-5.2f, -0.6f);
    glVertex2f(5.2f, -0.6f);  glVertex2f(7.0f, 0.9f);
    glVertex2f(4.5f, 0.6f);   glVertex2f(-4.5f, 0.6f);
    glEnd();

    glColor3f(0.12f, 0.10f, 0.08f);
    glBegin(GL_QUADS);
    glVertex2f(-2.8f, 0.7f);  glVertex2f(2.8f, 0.7f);
    glVertex2f(2.2f, 2.8f);   glVertex2f(-2.2f, 2.8f);
    glEnd();

    glColor3f(0.55f, 0.45f, 0.28f);
    glLineWidth(2.5f);
    glBegin(GL_LINES);
    glVertex2f(0.0f, 2.8f);   glVertex2f(0.0f, 6.0f);
    glEnd();

    glColor4f(0.92f, 0.92f, 0.88f, 0.85f);
    glBegin(GL_TRIANGLES);
    glVertex2f(0.0f, 3.0f);   glVertex2f(4.2f, 4.2f);  glVertex2f(0.0f, 5.8f);
    glEnd();

    glPushMatrix();
    glTranslatef(-4.2f, 0.7f, 0);

    glColor3f(0.22f, 0.12f, 0.05f);
    glLineWidth(2.2f);
    glBegin(GL_LINES);
    glVertex2f(0.2f, 1.2f);   glVertex2f(-2.5f, -2.2f);
    glEnd();
    glBegin(GL_TRIANGLES);
    glVertex2f(-2.5f, -2.2f); glVertex2f(-2.8f, -2.7f); glVertex2f(-2.1f, -2.5f);
    glEnd();

    glColor3f(0.15f, 0.40f, 0.55f);
    glBegin(GL_POLYGON);
    glVertex2f(-0.4f, 0.0f);  glVertex2f(0.4f, 0.0f);
    glVertex2f(0.3f, 0.8f);   glVertex2f(-0.3f, 0.8f);
    glEnd();

    glColor3f(0.85f, 0.25f, 0.20f);
    glBegin(GL_QUADS);
    glVertex2f(-0.3f, 0.8f);  glVertex2f(0.3f, 0.8f);
    glVertex2f(0.25f, 1.8f);  glVertex2f(-0.25f, 1.8f);
    glEnd();

    glColor3f(0.82f, 0.62f, 0.48f);
    drawCircle(0.0f, 2.2f, 0.32f, 15);

    glColor3f(0.88f, 0.75f, 0.20f);
    glBegin(GL_TRIANGLES);
    glVertex2f(-0.45f, 2.25f); glVertex2f(0.0f, 2.8f); glVertex2f(0.45f, 2.25f);
    glEnd();

    glPopMatrix();

    if (boatX >= 75.0f) {
        glColor3f(0.2f, 0.15f, 0.1f);
        glLineWidth(2.0f);
        glBegin(GL_LINES);
        glVertex2f(6.8f, 0.5f);
        glVertex2f(87.2f - boatX, 1.2f);
        glEnd();
    }

    glPopMatrix();
}

void drawBeautifulPerson(float x, float y, float r, float g, float b, int dir, float animStep) {
    glPushMatrix();
    glTranslatef(x, y, 0);
    glScalef(dir * 0.75f, 0.75f, 1);

    float legAngle = sin(animStep * 6.0f) * 0.30f;

    glColor3f(0.15f, 0.15f, 0.25f);
    glLineWidth(2.2f);
    glBegin(GL_LINES);
    glVertex2f(-0.1f, 0.9f); glVertex2f(-0.1f - legAngle, 0.0f);
    glVertex2f(0.1f, 0.9f);  glVertex2f(0.1f + legAngle, 0.0f);
    glEnd();

    glColor3f(r, g, b);
    glBegin(GL_POLYGON);
    glVertex2f(-0.3f, 0.9f); glVertex2f(0.3f, 0.9f);
    glVertex2f(0.25f, 2.0f); glVertex2f(-0.25f, 2.0f);
    glEnd();

    glColor3f(0.88f, 0.68f, 0.55f);
    drawCircle(0.0f, 2.4f, 0.3f, 20);

    if (isRaining) {
        glColor3f(0.1f, 0.1f, 0.1f);
        //Umbrella strip
        glLineWidth(2.0f);
        glBegin(GL_LINES);
        glVertex2f(0.2f, 1.5f); glVertex2f(0.2f, 3.8f);
        glEnd();

        glColor3f(0.82f, 0.12f, 0.22f);
        //umbrella Head
        glBegin(GL_TRIANGLES);
        glVertex2f(-1.8f, 3.5f); glVertex2f(0.2f, 4.5f); glVertex2f(2.2f, 3.5f);
        glEnd();
    }

    glPopMatrix();
}

void drawWindmill(float x, float y) {
    glPushMatrix();
    glTranslatef(x, y, 0);

    if (isNight) glColor3f(0.30f, 0.30f, 0.34f);
    else glColor3f(0.85f, 0.85f, 0.88f);

    glBegin(GL_POLYGON);
    glVertex2f(-0.4f, 0); glVertex2f(0.4f, 0);
    glVertex2f(0.2f, 7.0f); glVertex2f(-0.2f, 7.0f);
    glEnd();

    drawCircle(0.0f, 7.0f, 0.35f, 15);

    glTranslatef(0.0f, 7.0f, 0);
    glRotatef(windmillAngle, 0, 0, 1);

    glColor3f(0.92f, 0.92f, 0.95f);
    for (int i = 0; i < 3; i++) {
        glRotatef(120, 0, 0, 1);
        glBegin(GL_TRIANGLES);
        glVertex2f(0.0f, 0.0f); glVertex2f(0.2f, 4.2f); glVertex2f(-0.2f, 4.2f);
        glEnd();
    }

    glPopMatrix();
}

void drawRainAndSplashes() {
    if (!isRaining) return;

    glLineWidth(1.2f);
    glBegin(GL_LINES);
    for (int i = 0; i < RAIN_COUNT; i++) {
        glColor4f(0.80f, 0.90f, 1.0f, 0.45f);
        glVertex2f(rainX[i], rainY[i]);
        glColor4f(0.92f, 0.96f, 1.0f, 0.08f);
        glVertex2f(rainX[i] - 0.5f, rainY[i] - 1.8f);
    }
    glEnd();

    for (int i = 0; i < SPLASH_COUNT; i++) {
        if (splashAlpha[i] > 0.01f) {
            glColor4f(0.85f, 0.92f, 1.0f, splashAlpha[i]);
            glLineWidth(1.0f);
            drawHollowCircle(splashX[i], splashY[i], splashRadius[i], 12);
        }
    }
}

void drawSingleBeautifulBird(float x, float y, float scale, float wingOffset) {
    glPushMatrix();
    glTranslatef(x, y, 0);
    glScalef(scale, scale, 1.0f);

    float wingY = sin((wingAngle + wingOffset) * PI / 180.0f) * 0.75f;

    glColor3f(0.08f, 0.08f, 0.12f);
    drawCircle(0.0f, 0.0f, 0.28f, 10);
    drawCircle(0.25f, 0.1f, 0.22f, 10);

    glBegin(GL_TRIANGLES);
    glVertex2f(0.45f, 0.12f); glVertex2f(0.65f, 0.05f); glVertex2f(0.45f, -0.02f);
    glEnd();

    glBegin(GL_TRIANGLES);
    glVertex2f(-0.20f, 0.05f); glVertex2f(-0.70f, -0.15f); glVertex2f(-0.35f, -0.10f);
    glEnd();

    glBegin(GL_POLYGON);
    glVertex2f(-0.1f, 0.0f); glVertex2f(-0.8f, 0.6f + wingY);
    glVertex2f(-1.4f, 1.1f + wingY); glVertex2f(-0.7f, 0.4f + (wingY * 0.6f));
    glVertex2f(0.1f, 0.1f);
    glEnd();

    glBegin(GL_POLYGON);
    glVertex2f(-0.05f, 0.05f); glVertex2f(0.6f, 0.65f + wingY);
    glVertex2f(1.2f, 1.15f + wingY); glVertex2f(0.5f, 0.45f + (wingY * 0.6f));
    glVertex2f(0.15f, 0.15f);
    glEnd();

    glPopMatrix();
}

void drawFlockOfBirds() {
    if (isNight || isRaining) return;

    float birdData[7][4] = {
        {  0.0f,  0.0f, 1.00f,   0.0f },
        { -3.0f, -1.2f, 0.88f,  20.0f },
        {  3.0f, -1.0f, 0.88f,  25.0f },
        { -6.0f, -2.4f, 0.78f,  40.0f },
        {  6.0f, -2.2f, 0.78f,  45.0f },
        { -9.0f, -3.6f, 0.68f,  60.0f },
        {  9.0f, -3.5f, 0.68f,  65.0f }
    };

    for (int i = 0; i < 7; i++) {
        drawSingleBeautifulBird(
            flockX + birdData[i][0],
            65.0f + birdData[i][1] + sin((flockX + i*5) * 0.05f) * 1.5f,
            birdData[i][2],
            birdData[i][3]
        );
    }
}

// ==================== INTRO SCREEN ====================

void introScreen() {
    glClearColor(0.06f, 0.08f, 0.12f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    glColor3f(0.98f, 0.82f, 0.25f);
    renderText(22.0f, 58.0f, "SMART VILLAGE SCENERY - COMPUTER GRAPHICS", GLUT_BITMAP_TIMES_ROMAN_24);

    glColor3f(0.92f, 0.92f, 0.95f);
    renderText(24.0f, 48.0f, "Project By: Jannatul Ferdous, Md Sakibur Rahman, Abu Hossain", GLUT_BITMAP_HELVETICA_18);

    glColor3f(0.35f, 0.85f, 0.55f);
    renderText(32.0f, 36.0f, "PRESS ANY KEY TO START PROJECT", GLUT_BITMAP_HELVETICA_18);

    glColor3f(0.65f, 0.70f, 0.78f);
    renderText(12.0f, 22.0f, "CONTROLS: [D] Day | [N] Night | [R] Rain | [E] Emergency | [B] Boat | [ESC] Exit", GLUT_BITMAP_HELVETICA_12);

    glutSwapBuffers();
}

// ==================== DISPLAY FUNCTION ====================

void display() {
    if (showIntro) {
        introScreen();
        return;
    }

    glClear(GL_COLOR_BUFFER_BIT);

    drawGradientSky();
    drawStars();
    drawSun();
    drawMoon();
    drawMountains();
    drawClouds();

    drawWindmill(92.0f, 40.0f);

    drawPineTree(4.0f, 40.0f, 1.1f);
    drawPineTree(8.0f, 40.0f, 1.3f);
    drawBeautifulTree(14.0f, 40.0f, 1.4f);
    drawBeautifulTree(22.0f, 40.0f, 1.2f);
    drawPineTree(88.0f, 40.0f, 1.2f);

    drawGround();

    drawVillageHouse(7.0f, 28.0f, 1.1f, 0.75f, 0.55f, 0.38f, false);
    drawVillageHouse(18.0f, 29.0f, 1.25f, 0.68f, 0.45f, 0.32f, true);

    drawHospitalConnectingRoad();
    drawHospital(48.0f, 28.0f);
    drawSmartPharmacy(70.0f, 28.0f);

    drawCCTV(41.0f, 32.0f, false);

    drawLampPost(33.0f, 22.0f);
    drawCCTV(33.0f, 28.0f, true);
    drawLampPost(73.0f, 22.0f);

    drawVillageHouse(80.0f, 29.0f, 1.2f, 0.78f, 0.60f, 0.40f, false);
    drawVillageHouse(90.0f, 28.0f, 1.3f, 0.62f, 0.40f, 0.28f, true);

    drawFlowerCluster(12.0f, 22.5f);
    drawFlowerCluster(23.0f, 23.5f);
    drawFlowerCluster(70.0f, 23.5f);
    drawFlowerCluster(94.0f, 22.5f);
    drawBush(30.0f, 22.5f, 1.2f);
    drawBush(71.0f, 22.5f, 1.3f);

    drawBench(25.0f, 22.5f);

    drawDustbin(11.0f, 22.5f);
    drawDustbin(36.0f, 22.5f);
    drawDustbin(61.0f, 22.5f);

    drawVillageBoard(1.0f, 22.5f);
    drawDirectionBoard(35.0f, 22.5f);

    drawBusStop(75.0f, 22.0f);

    if (isNight) {
        // Bus stop waiting pedestrian stays
        drawBeautifulPerson(75.5f, 22.2f, 0.2f, 0.7f, 0.8f, 1, 0.0f);
        // Walking pedestrian replaces the standing one on the street
        drawBeautifulPerson(nightPedX, 22.2f, 0.8f, 0.5f, 0.2f, 1, nightPedX);
    }

    drawBeautifulRoad();
    drawZebraCrossing(48.0f);
    drawLowerPedestrianRoad();

    drawTrafficSignal(42.0f, 22.0f);

    drawCar();
    drawBus();
    drawSchoolBus();

    if (!isEmergency) {
        if (!isNight && (hospPedState == 0 || hospPedState == 1)) {
            drawBeautifulPerson(hospPedX, hospPedY, 0.2f, 0.4f, 0.8f, 1, (hospPedState == 0 ? hospPedX : hospPedY));
        }
    }

    if (ambState == 2) {
        drawAmbulanceVertical(ambX, ambY);
    } else if (ambState == 0 || ambState == 3) {
        drawAmbulanceSide(ambX, ambY);
    }

    if (!isNight) {
        float anim1 = (ped1State == 2) ? ped1Y : ped1X;
        drawBeautifulPerson(ped1X, ped1Y, 0.1f, 0.6f, 0.3f, 1, anim1);

        float anim2 = (ped2State == 2) ? ped2Y : ped2X;
        drawBeautifulPerson(ped2X, ped2Y, 0.9f, 0.4f, 0.1f, -1, anim2);

        drawBeautifulPerson(lowerPedX, 13.2f, 0.8f, 0.2f, 0.5f, 1, lowerPedX);
    }

    drawBroadGreenSpace();

    drawFence(0.0f, 35.0f, 10.3f);
    drawFence(65.0f, 100.0f, 10.3f);

    drawRiver();
    drawFishermanAndFish();
    drawBeautifulBoat();

    drawFlockOfBirds();
    drawRainAndSplashes();

    if (isEmergency) {
        glColor3f(1.0f, 0.1f, 0.1f);
        renderText(3.0f, 75.0f, "EMERGENCY MODE ACTIVE", GLUT_BITMAP_HELVETICA_18);
    }

    glutSwapBuffers();
}

// ==================== ANIMATION TIMER ====================

void update(int value) {
    windmillAngle += 1.2f;

    if (!isEmergency) {
        trafficTimer++;
        if (trafficTimer > 260) {
            isTrafficRed = !isTrafficRed;
            trafficTimer = 0;
        }
    } else {
        isTrafficRed = true;
    }

    if (isEmergency) {
        float ambSpeed = 0.18f;

        if (ambState == 0) {
            ambX -= (ambSpeed * 0.5f);
            if (ambX <= 48.0f) {
                ambX = 48.0f;
                ambState = 2;
            }
        } else if (ambState == 2) {
            ambY -= ambSpeed;
            if (ambY <= 16.2f) {
                ambY = 16.2f;
                ambState = 3;
            }
        } else if (ambState == 3) {
            ambX += ambSpeed * 1.5f;
            if (ambX > 115.0f) {
                ambState = 4;
            }
        }
    } else {
        ambX = 53.5f;
        ambY = 27.8f;
        ambState = 0;
    }

    float carSpeed = 0.35f;
    float busSpeed = 0.26f;
    float schoolBusSpeed = 0.24f;

    bool stopTraffic = isTrafficRed || isEmergency;

    bool carMustStop = stopTraffic && (carX + 3.8f >= 36.0f && carX < 42.0f);
    if (!carMustStop) {
        carX += carSpeed;
        if (carX > 115.0f) carX = -25.0f;
    }

    bool busMustStop = (stopTraffic && (busX + 6.5f >= 32.0f && busX < 38.0f))
                    || (carX > busX && (carX - busX) < 17.0f);
    if (!busMustStop) {
        busX += busSpeed;
        if (busX > 115.0f) busX = -45.0f;
    }

    if (!isNight) {
        bool schoolBusMustStop = (stopTraffic && (schoolBusX + 6.0f >= 16.0f && schoolBusX < 22.0f))
                               || (busX > schoolBusX && (busX - schoolBusX) < 17.0f);
        if (!schoolBusMustStop) {
            schoolBusX += schoolBusSpeed;
            if (schoolBusX > 115.0f) schoolBusX = -65.0f;
        }
    }

    float walkSpeed = 0.040f;
    float crossSpeed = 0.045f;

    if (isNight) {
        nightPedX += walkSpeed;
        if (nightPedX > 105.0f) nightPedX = -5.0f;
    }

    if (ped1State == 0) {
        ped1X += walkSpeed;
        ped1Y = 22.2f;
        if (ped1X >= 47.0f) {
            ped1X = 47.0f;
            ped1State = 1;
        }
    } else if (ped1State == 1) {
        ped1X = 47.0f;
        ped1Y = 22.2f;
        if (isTrafficRed || isEmergency) {
            ped1State = 2;
        }
    } else if (ped1State == 2) {
        ped1X = 47.0f;
        ped1Y -= crossSpeed;
        if (ped1Y <= 13.7f) {
            ped1Y = 13.7f;
            ped1State = 3;
        }
    } else if (ped1State == 3) {
        ped1X += walkSpeed;
        ped1Y = 13.7f;
        if (ped1X > 110.0f) {
            ped1X = -10.0f;
            ped1State = 0;
        }
    }

    if (ped2State == 0) {
        ped2X -= walkSpeed;
        ped2Y = 13.7f;
        if (ped2X <= 49.0f) {
            ped2X = 49.0f;
            ped2State = 1;
        }
    } else if (ped2State == 1) {
        ped2X = 49.0f;
        ped2Y = 13.7f;
        if (isTrafficRed || isEmergency) {
            ped2State = 2;
        }
    } else if (ped2State == 2) {
        ped2X = 49.0f;
        ped2Y += crossSpeed;
        if (ped2Y >= 22.2f) {
            ped2Y = 22.2f;
            ped2State = 3;
        }
    } else if (ped2State == 3) {
        ped2X -= walkSpeed;
        ped2Y = 22.2f;
        if (ped2X < -10.0f) {
            ped2X = 110.0f;
            ped2State = 0;
        }
    }

    lowerPedX += walkSpeed;
    if (lowerPedX > 105.0f) lowerPedX = -5.0f;

    if (!isEmergency) {
        if (hospPedState == 0) {
            hospPedX += walkSpeed;
            if (hospPedX >= 48.0f) {
                hospPedX = 48.0f;
                hospPedState = 1;
            }
        } else if (hospPedState == 1) {
            hospPedY += (walkSpeed * 0.6f);
            if (hospPedY >= 28.5f) hospPedState = 2;
        } else if (hospPedState == 2) {
            static int hospWait = 0;
            hospWait++;
            if (hospWait > 150) {
                hospPedX = 10.0f;
                hospPedY = 22.2f;
                hospPedState = 0;
                hospWait = 0;
            }
        }
    }

    if (!isRaining &&  boatDir != 0) {
        float dockLocationX = 79.0f;
        float boatSpeed = 0.055f;

        if (boatDir == 1) {
            if (boatX < dockLocationX) {
                boatX += boatSpeed;
            } else {
                boatDockWaitTimer++;
                if (boatDockWaitTimer > 120) {
                    boatDir = 2;
                    boatDockWaitTimer = 0;
                }
            }
        } else if (boatDir == 2) {
            boatX -= boatSpeed;
            if (boatX < -35.0f) boatDir = 1;
        }
    }

    for (int p = 0; p < SMOKE_PARTICLES; p++) {
        smokeOffset[p] += 0.04f;
        if (smokeOffset[p] > 4.0f) smokeOffset[p] = 0.0f;
    }

    for (int i = 0; i < 12; i++) {
        cloudPos[i] += cloudSpeed[i];
        if (cloudPos[i] > 130) cloudPos[i] = -20;
    }

    if (isRaining) {
        for (int i = 0; i < RAIN_COUNT; i++) {
            rainY[i] -= rainSpeed[i];
            rainX[i] -= 0.5f;

            if (rainY[i] < 2.0f) {
                if (rand() % 12 == 0) {
                    int sIndex = rand() % SPLASH_COUNT;
                    splashX[sIndex] = rainX[i];
                    splashY[sIndex] = rainY[i];
                    splashRadius[sIndex] = 0.2f;
                    splashAlpha[sIndex] = 0.8f;
                }
                rainY[i] = 85.0f;
                rainX[i] = -10.0f + (rand() % 14000) / 100.0f;
            }
        }

        for (int i = 0; i < SPLASH_COUNT; i++) {
            if (splashAlpha[i] > 0.0f) {
                splashRadius[i] += 0.08f;
                splashAlpha[i] -= 0.05f;
            }
        }

        if (rand() % 100 < 3) {
            isLightning = true;
            lightningTimer = 3;
        }
        if (isLightning) {
            lightningTimer--;
            if (lightningTimer <= 0) isLightning = false;
        }
    }

    if (!isNight && !isRaining) {
        fishTimer += 0.05f;
        if (sin(fishTimer) > 0.8f) {
            fishJumping = true;
            fishJumpY = sin((fishTimer - 0.8f) * 8.0f) * 2.8f;
            fishJumpX = cos(fishTimer) * 0.5f;
        } else {
            fishJumping = false;
        }
    }

    if (!isNight && !isRaining) {
        flockX += 0.15f;
        if (flockX > 120.0f) flockX = -20.0f;
    }

    if (wingUp) {
        wingAngle += 3.0f;
        if (wingAngle > 28) wingUp = false;
    } else {
        wingAngle -= 3.0f;
        if (wingAngle < -28) wingUp = true;
    }

    waveOffset += 0.06f;

    glutPostRedisplay();
    glutTimerFunc(16, update, 0);
}

// ==================== KEYBOARD CONTROLS ====================

void keyboard(unsigned char key, int x, int y) {
    if (showIntro) {
        showIntro = false;
        glutPostRedisplay();
        return;
    }

    switch (key) {
    case 'd': case 'D':
        isNight = false; isRaining = false;
        break;
    case 'n': case 'N':
        isNight = true; isRaining = false;
        break;
    case 'r': case 'R':
        isRaining = !isRaining; isNight = false;
        break;
    case 'e': case 'E':
        isEmergency = !isEmergency;
        if (!isEmergency) {
            ambX = 53.5f;
            ambY = 27.8f;
            ambState = 0;
            isTrafficRed = false;
        }
        break;
    case 'b': case 'B':
        boatDir = (boatDir == 0) ? 1 : 0;
        break;
    case 27: // ESC
        exit(0);
        break;
    }
    glutPostRedisplay();
}

// ==================== MAIN FUNCTION ====================

int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
    glutInitWindowSize(1280, 720);
    glutInitWindowPosition(80, 40);
    glutCreateWindow("Smart Village Scenery - Computer Graphics Project");
    init();
    glutDisplayFunc(display);
    glutKeyboardFunc(keyboard);
    glutTimerFunc(0, update, 0);
    glutMainLoop();
    return 0;
}
