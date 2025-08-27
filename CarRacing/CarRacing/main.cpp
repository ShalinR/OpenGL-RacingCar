
#define NOMINMAX
#include <windows.h>
#include <GL/glut.h>
#include <GL/glu.h>
#include <cmath>
#include <vector>
#include <utility>
#include <SOIL2.h>

GLuint asphaltTex;
GLuint tireTexture=0;
GLuint grassTex;
GLuint carTex;
GLuint helmetTex;
GLuint treeTexture;
GLuint buildingTexture;
// ===== Track Parameters =====
const float TRACK_WIDTH = 12.0f;   // constant track width

// ===== Car State =====
float carX = 50.0f, carZ = 50.0f;
float carAngle = 0.0f;
float carSpeed = 10.0f;
float tireRotation = 0.0f; // in degrees

float startLineAngle = 0.0f;


float camYawOffset = 0.0f;   // Left/right look
float camPitchOffset = 0.0f; // Up/down look

std::pair<float, float> startLineCenter; // already set in generateTrackPoints()
float startLineWidth =2.0;      // width of track
float startLineLength = 4.0;
// ===== Movement Flags =====
bool moveForward = false;
bool moveBackward = false;
bool moveLeft = false;
bool moveRight = false;

// ===== Constants =====
const float MAX_SPEED_FW = 30.0f;
const float MAX_SPEED_BW = 30.0f;
const float ACCELERATION = 20.0f;
const float TURN_ANGLE = 90.0f;
const float FRICTION = 8.0f;
const float M_PI_F = 3.14159265358979323846f;



std::vector<std::pair<float, float>> innerTrack;
std::vector<std::pair<float, float>> outerTrack;

std::pair<float, float> catmullRom(
    const std::pair<float, float>& p0,
    const std::pair<float, float>& p1,
    const std::pair<float, float>& p2,
    const std::pair<float, float>& p3,
    float t
) {
    float t2 = t * t;
    float t3 = t2 * t;

    float x = 0.5f * ((2.0f * p1.first) +
        (-p0.first + p2.first) * t +
        (2.0f * p0.first - 5.0f * p1.first + 4.0f * p2.first - p3.first) * t2 +
        (-p0.first + 3.0f * p1.first - 3.0f * p2.first + p3.first) * t3);

    float z = 0.5f * ((2.0f * p1.second) +
        (-p0.second + p2.second) * t +
        (2.0f * p0.second - 5.0f * p1.second + 4.0f * p2.second - p3.second) * t2 +
        (-p0.second + 3.0f * p1.second - 3.0f * p2.second + p3.second) * t3);

    return { x, z };
}
// ===== Timing =====
int lastTime = 0;

// ===== Display Lists =====
GLuint trackList = 0;
GLuint tireList = 0;

#define NUM_AUDIENCE 200

struct Person {
    float x, z;
    float baseY;      // ground level
    float jumpPhase;
    float jumpSpeed;  // speed of jumping

    float r, g, b;    // clothing color
    // phase for sine jump
};

struct Stand {
    float x, z;     // position of the stand
    float width;
    float depth;
    float height;
};
std::vector<Stand> stands;

std::vector<Person> audience;
struct Building {
    float x, z;       // position
    float width, depth, height;
};
std::vector<Building> buildings;


struct Tree {
    float x, z;
    float height;
    float radius;
};

struct TrackObject {
    float x, z;
    int type; // 0 = tire stack, 1 = barrier, 2 = lamp post, 3 = banner, etc.
};
std::vector<Tree> trees;
std::vector<TrackObject> trackObjects;



// Simple function to generate random float in [a,b]
float randf(float a, float b) {
    return a + ((float)rand() / RAND_MAX) * (b - a);
}

//// Call after generating the track
void generateAudience() {
    audience.clear();
    int trackSize = innerTrack.size();

    for (int i = 0; i < NUM_AUDIENCE; i++) {
        int idx = rand() % trackSize;

        float offset = TRACK_WIDTH * 1.5f + randf(0, 10);
        float side = (rand() % 2 == 0 ? 1 : -1);

        size_t nextIdx = (idx + 1) % trackSize;
        float dx = outerTrack[nextIdx].first - outerTrack[idx].first;
        float dz = outerTrack[nextIdx].second - outerTrack[idx].second;
        float len = sqrtf(dx * dx + dz * dz);
        dx /= len; dz /= len;
        float px = -dz;
        float pz = dx;

        Person p;
        p.x = outerTrack[idx].first + side * px * offset;
        p.z = outerTrack[idx].second + side * pz * offset;
        p.baseY = 0.0f;
        p.jumpPhase = randf(0.0f, 3.14f * 2.0f);
        p.jumpSpeed = randf(0.15f, 0.25f);  // faster jump speed

        p.r = randf(0.0f, 1.0f);
        p.g = randf(0.0f, 1.0f);
        p.b = randf(0.0f, 1.0f);

        audience.push_back(p);
    }
}



void generateStands(int numStands) {
    stands.clear();
    int trackSize = innerTrack.size();
    for (int i = 0; i < numStands; i++) {
        int idx = rand() % trackSize;

        // pick midpoint between inner and outer track
        float midX = (innerTrack[idx].first + outerTrack[idx].first) * 0.5f;
        float midZ = (innerTrack[idx].second + outerTrack[idx].second) * 0.5f;

        // place stand offset outward
        float dx = outerTrack[idx].first - innerTrack[idx].first;
        float dz = outerTrack[idx].second - innerTrack[idx].second;
        float len = sqrtf(dx * dx + dz * dz);
        dx /= len; dz /= len;

        float side = (rand() % 2 == 0 ? 1.0f : -1.0f);
        float offset = TRACK_WIDTH * 3.0f + randf(0, 30.0f);

        Stand s;
        s.x = midX + side * dx * offset;
        s.z = midZ + side * dz * offset;
        s.width = 15.0f + randf(0, 10.0f);
        s.depth = 10.0f + randf(0, 10.0f);
        s.height = 6.0f;
        stands.push_back(s);
    }
}




void generateBuildings(int numBuildings,
    float trackMinX, float trackMaxX,
    float trackMinZ, float trackMaxZ,
    float border = 20.0f) {
    buildings.clear();
    for (int i = 0; i < numBuildings; i++) {
        Building b;

        // Randomly decide which side to place the building
        int side = rand() % 4; // 0=left, 1=right, 2=front, 3=back

        switch (side) {
        case 0: // left
            b.x = trackMinX - border - static_cast<float>(rand()) / RAND_MAX * border;
            b.z = trackMinZ - border + static_cast<float>(rand()) / RAND_MAX * (trackMaxZ - trackMinZ + 2 * border);
            break;
        case 1: // right
            b.x = trackMaxX + border + static_cast<float>(rand()) / RAND_MAX * border;
            b.z = trackMinZ - border + static_cast<float>(rand()) / RAND_MAX * (trackMaxZ - trackMinZ + 2 * border);
            break;
        case 2: // front
            b.z = trackMinZ - border - static_cast<float>(rand()) / RAND_MAX * border;
            b.x = trackMinX - border + static_cast<float>(rand()) / RAND_MAX * (trackMaxX - trackMinX + 2 * border);
            break;
        case 3: // back
            b.z = trackMaxZ + border + static_cast<float>(rand()) / RAND_MAX * border;
            b.x = trackMinX - border + static_cast<float>(rand()) / RAND_MAX * (trackMaxX - trackMinX + 2 * border);
            break;
        }

        // Random building dimensions
        b.width = 2.0f + static_cast<float>(rand()) / RAND_MAX * 3.0f;
        b.depth = 2.0f + static_cast<float>(rand()) / RAND_MAX * 3.0f;
        b.height = 10.0f + static_cast<float>(rand()) / RAND_MAX * 40.0f;

        buildings.push_back(b);
    }
}




GLuint loadTexture(const char* filename) {
    GLuint texID = SOIL_load_OGL_texture(
        filename,
        SOIL_LOAD_AUTO,
        SOIL_CREATE_NEW_ID,
        SOIL_FLAG_MIPMAPS | SOIL_FLAG_INVERT_Y
    );

    if (texID == 0) {
        printf("SOIL loading error for '%s': %s\n", filename, SOIL_last_result());
        return 0;
    }

    // Bind and configure texture parameters
    glBindTexture(GL_TEXTURE_2D, texID);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    return texID;
}
void generateAllAudience() {
    audience.clear();
    generateStands(5);
    generateAudience();         // random trackside spectators
}
void drawPerson(const Person& p, float time) {
    float jump = sinf(time + p.jumpPhase) * 0.5f; // jump animation

    glPushMatrix();
    glTranslatef(p.x, p.baseY + jump, p.z);

    // Body
    glColor3f(p.r, p.g, p.b);
    glPushMatrix();
    glScalef(0.5f, 1.5f, 0.3f);
    glutSolidCube(1.0f);
    glPopMatrix();

    // Head
    glPushMatrix();
    glTranslatef(0.0f, 1.2f, 0.0f);
    glutSolidSphere(0.4f, 16, 16);
    glPopMatrix();

    // Arms
    glPushMatrix();
    glTranslatef(-0.5f, 0.6f, 0.0f);
    glScalef(0.2f, 1.0f, 0.2f);
    glutSolidCube(1.0f);
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.5f, 0.6f, 0.0f);
    glScalef(0.2f, 1.0f, 0.2f);
    glutSolidCube(1.0f);
    glPopMatrix();

    // Legs
    glPushMatrix();
    glTranslatef(-0.2f, -1.0f, 0.0f);
    glScalef(0.25f, 1.2f, 0.25f);
    glutSolidCube(1.0f);
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.2f, -1.0f, 0.0f);
    glScalef(0.25f, 1.2f, 0.25f);
    glutSolidCube(1.0f);
    glPopMatrix();

    glPopMatrix();
}

void drawAudience() {
    for (auto& p : audience) {
        p.jumpPhase += p.jumpSpeed;  // increment phase for faster jumping
        drawPerson(p,0.1f);
    }
}

void drawStand(const Stand& s) {
    glPushMatrix();
    glTranslatef(s.x, 0.0f, s.z);

    // base structure
    glColor3f(0.7f, 0.7f, 0.7f);
    glPushMatrix();
    glScalef(s.width, s.height, s.depth);
    glutSolidCube(1.0f);
    glPopMatrix();

    // roof
    glColor3f(0.3f, 0.3f, 0.3f);
    glPushMatrix();
    glTranslatef(0.0f, s.height / 2.0f + 0.5f, 0.0f);
    glScalef(s.width + 2, 1.0f, s.depth + 2);
    glutSolidCube(1.0f);
    glPopMatrix();

    glPopMatrix();
}
void drawBuildings() {
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, buildingTexture);

    for (auto& b : buildings) {
        float newHeight = b.height - 5.0f; // reduce height

        glPushMatrix();
        glTranslatef(b.x, newHeight / 2.0f, b.z); // move to center of building
        glScalef(b.width, newHeight, b.depth);    // scale cube to building size
        glColor3f(1.0f, 1.0f, 1.0f);

        glBegin(GL_QUADS);
        // Front face
        glTexCoord2f(0, 0); glVertex3f(-0.5f, -0.5f, 0.5f);
        glTexCoord2f(1, 0); glVertex3f(0.5f, -0.5f, 0.5f);
        glTexCoord2f(1, 1); glVertex3f(0.5f, 0.5f, 0.5f);
        glTexCoord2f(0, 1); glVertex3f(-0.5f, 0.5f, 0.5f);
        // Back face
        glTexCoord2f(0, 0); glVertex3f(-0.5f, -0.5f, -0.5f);
        glTexCoord2f(1, 0); glVertex3f(0.5f, -0.5f, -0.5f);
        glTexCoord2f(1, 1); glVertex3f(0.5f, 0.5f, -0.5f);
        glTexCoord2f(0, 1); glVertex3f(-0.5f, 0.5f, -0.5f);
        // Left face
        glTexCoord2f(0, 0); glVertex3f(-0.5f, -0.5f, -0.5f);
        glTexCoord2f(1, 0); glVertex3f(-0.5f, -0.5f, 0.5f);
        glTexCoord2f(1, 1); glVertex3f(-0.5f, 0.5f, 0.5f);
        glTexCoord2f(0, 1); glVertex3f(-0.5f, 0.5f, -0.5f);
        // Right face
        glTexCoord2f(0, 0); glVertex3f(0.5f, -0.5f, -0.5f);
        glTexCoord2f(1, 0); glVertex3f(0.5f, -0.5f, 0.5f);
        glTexCoord2f(1, 1); glVertex3f(0.5f, 0.5f, 0.5f);
        glTexCoord2f(0, 1); glVertex3f(0.5f, 0.5f, -0.5f);
        // Top face
        glTexCoord2f(0, 0); glVertex3f(-0.5f, 0.5f, -0.5f);
        glTexCoord2f(1, 0); glVertex3f(0.5f, 0.5f, -0.5f);
        glTexCoord2f(1, 1); glVertex3f(0.5f, 0.5f, 0.5f);
        glTexCoord2f(0, 1); glVertex3f(-0.5f, 0.5f, 0.5f);
        // Bottom face
        glTexCoord2f(0, 0); glVertex3f(-0.5f, -0.5f, -0.5f);
        glTexCoord2f(1, 0); glVertex3f(0.5f, -0.5f, -0.5f);
        glTexCoord2f(1, 1); glVertex3f(0.5f, -0.5f, 0.5f);
        glTexCoord2f(0, 1); glVertex3f(-0.5f, -0.5f, 0.5f);
        glEnd();

        glPopMatrix();
    }

    glDisable(GL_TEXTURE_2D);
}



void generateAudienceInStands() {
    for (const auto& s : stands) {
        int numPeople = 20 + rand() % 40;
        for (int i = 0; i < numPeople; i++) {
            Person p;
            p.x = s.x + randf(-s.width / 2, s.width / 2);
            p.z = s.z + randf(-s.depth / 2, s.depth / 2);
            p.baseY = 1.0f; // elevate audience above ground
            p.jumpPhase = randf(0, 6.28f);
            p.r = randf(0.0f, 1.0f);
            p.g = randf(0.0f, 1.0f);
            p.b = randf(0.0f, 1.0f);
            audience.push_back(p);
        }
    }
}

// ==================== LIGHTING ====================
void setupLights() {
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glEnable(GL_LIGHT1);

    // --- Main directional sunlight ---
    GLfloat ambient0[] = { 0.2f, 0.2f, 0.2f, 1.0f };     // subtle ambient
    GLfloat diffuse0[] = { 1.0f, 0.95f, 0.9f, 1.0f };    // warm sunlight
    GLfloat specular0[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    GLfloat position0[] = { 50.0f, 80.0f, 50.0f, 1.0f };
    glLightfv(GL_LIGHT0, GL_AMBIENT, ambient0);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuse0);
    glLightfv(GL_LIGHT0, GL_SPECULAR, specular0);
    glLightfv(GL_LIGHT0, GL_POSITION, position0);

    // --- Secondary soft light for shadows / fill ---
    GLfloat ambient1[] = { 0.1f, 0.1f, 0.15f, 1.0f };    // bluish ambient
    GLfloat diffuse1[] = { 0.4f, 0.45f, 0.5f, 1.0f };    // cool fill light
    GLfloat specular1[] = { 0.3f, 0.3f, 0.3f, 1.0f };
    GLfloat position1[] = { -50.0f, 30.0f, -40.0f, 1.0f };
    glLightfv(GL_LIGHT1, GL_AMBIENT, ambient1);
    glLightfv(GL_LIGHT1, GL_DIFFUSE, diffuse1);
    glLightfv(GL_LIGHT1, GL_SPECULAR, specular1);
    glLightfv(GL_LIGHT1, GL_POSITION, position1);

    // --- Car material (metallic paint) ---
    GLfloat mat_ambient[] = { 0.2f, 0.2f, 0.2f, 1.0f };
    GLfloat mat_diffuse[] = { 0.7f, 0.1f, 0.1f, 1.0f };  // red car
    GLfloat mat_specular[] = { 0.9f, 0.9f, 0.9f, 1.0f }; // strong highlights
    GLfloat mat_shininess = 80.0f;                        // shiny paint
    glMaterialfv(GL_FRONT, GL_AMBIENT, mat_ambient);
    glMaterialfv(GL_FRONT, GL_DIFFUSE, mat_diffuse);
    glMaterialfv(GL_FRONT, GL_SPECULAR, mat_specular);
    glMaterialf(GL_FRONT, GL_SHININESS, mat_shininess);

    // --- Enable color material for other objects ---
    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);

    GLfloat mat_spec[] = { 0.3f, 0.3f, 0.3f, 1.0f }; // mild specular for non-metal
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, mat_spec);
    glMateriali(GL_FRONT_AND_BACK, GL_SHININESS, 32);
}


// ==================== VECTOR MATH ====================
struct Vec2 {
    float x, z;
    Vec2 operator+(const Vec2& o) const { return { x + o.x, z + o.z }; }
    Vec2 operator-(const Vec2& o) const { return { x - o.x, z - o.z }; }
    Vec2 operator*(float s) const { return { x * s, z * s }; }
};

Vec2 normalize(const Vec2& v) {
    float len = sqrtf(v.x * v.x + v.z * v.z);
    if (len == 0.0f) return { 0.0f, 0.0f };
    return { v.x / len, v.z / len };
}


// ==================== DRAW WHEEL ====================
void drawWheel(float radius, float width, int spokes) {
    GLUquadric* q = gluNewQuadric();

    glPushMatrix();
    glRotatef(90, 0, 1, 0); // align tire along X-axis

    // Tire tread (textured)
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, tireTexture);
    gluQuadricTexture(q, GL_TRUE);
    gluQuadricNormals(q, GLU_SMOOTH);
    glColor3f(1.0f, 1.0f, 1.0f);
    gluCylinder(q, radius, radius, width, 32, 1);
    glDisable(GL_TEXTURE_2D);

    // Front disk of tire (black)
    gluQuadricTexture(q, GL_FALSE);
    glColor3f(0.05f, 0.05f, 0.05f);
    gluDisk(q, 0, radius, 32, 1);

    // Back disk of tire (black)
    glPushMatrix();
    glTranslatef(0, 0, width);
    gluDisk(q, 0, radius, 32, 1);
    glPopMatrix();

    // --- Draw alloy hub / rim (white) ---
    float hubRadius = radius * 0.5f;
    float hubThickness = width * 0.2f;
    glColor3f(1.0f, 1.0f, 1.0f); // White rim
    glPushMatrix();
    glTranslatef(0, 0, width / 2.0f); // place in the middle of tire

    // central disk
    gluDisk(q, 0, hubRadius, 32, 1);

    // spokes
    for (int i = 0; i < spokes; i++) {
        float angle = i * 360.0f / spokes;
        glPushMatrix();
        glRotatef(angle, 0, 0, 1); // rotate around hub axis
        glTranslatef(hubRadius * 0.5f, 0, 0);
        glScalef(hubRadius * 0.5f, hubRadius * 0.05f, hubThickness);
        glutSolidCube(1.0f);
        glPopMatrix();
    }
    glPopMatrix(); // end hub

    glPopMatrix();
    gluDeleteQuadric(q);
}



static void drawTexturedBox(float sx, float sy, float sz)
{
    glPushMatrix();
    glScalef(sx, sy, sz);
    glColor3f(1.0f, 1.0f, 1.0f); // don't tint the texture

    glBegin(GL_QUADS);
    // +Z (front)
    glTexCoord2f(0.f, 0.f); glVertex3f(-0.5f, -0.5f, 0.5f);
    glTexCoord2f(1.f, 0.f); glVertex3f(0.5f, -0.5f, 0.5f);
    glTexCoord2f(1.f, 1.f); glVertex3f(0.5f, 0.5f, 0.5f);
    glTexCoord2f(0.f, 1.f); glVertex3f(-0.5f, 0.5f, 0.5f);

    // -Z (back)
    glTexCoord2f(1.f, 0.f); glVertex3f(-0.5f, -0.5f, -0.5f);
    glTexCoord2f(1.f, 1.f); glVertex3f(-0.5f, 0.5f, -0.5f);
    glTexCoord2f(0.f, 1.f); glVertex3f(0.5f, 0.5f, -0.5f);
    glTexCoord2f(0.f, 0.f); glVertex3f(0.5f, -0.5f, -0.5f);

    // +X (right)
    glTexCoord2f(0.f, 0.f); glVertex3f(0.5f, -0.5f, 0.5f);
    glTexCoord2f(1.f, 0.f); glVertex3f(0.5f, -0.5f, -0.5f);
    glTexCoord2f(1.f, 1.f); glVertex3f(0.5f, 0.5f, -0.5f);
    glTexCoord2f(0.f, 1.f); glVertex3f(0.5f, 0.5f, 0.5f);

    // -X (left)
    glTexCoord2f(1.f, 0.f); glVertex3f(-0.5f, -0.5f, 0.5f);
    glTexCoord2f(1.f, 1.f); glVertex3f(-0.5f, 0.5f, 0.5f);
    glTexCoord2f(0.f, 1.f); glVertex3f(-0.5f, 0.5f, -0.5f);
    glTexCoord2f(0.f, 0.f); glVertex3f(-0.5f, -0.5f, -0.5f);

    // +Y (top)
    glTexCoord2f(0.f, 1.f); glVertex3f(-0.5f, 0.5f, 0.5f);
    glTexCoord2f(0.f, 0.f); glVertex3f(0.5f, 0.5f, 0.5f);
    glTexCoord2f(1.f, 0.f); glVertex3f(0.5f, 0.5f, -0.5f);
    glTexCoord2f(1.f, 1.f); glVertex3f(-0.5f, 0.5f, -0.5f);

    // -Y (bottom)
    glTexCoord2f(1.f, 1.f); glVertex3f(-0.5f, -0.5f, 0.5f);
    glTexCoord2f(0.f, 1.f); glVertex3f(0.5f, -0.5f, 0.5f);
    glTexCoord2f(0.f, 0.f); glVertex3f(0.5f, -0.5f, -0.5f);
    glTexCoord2f(1.f, 0.f); glVertex3f(-0.5f, -0.5f, -0.5f);
    glEnd();

    glPopMatrix();
}
void drawF1Car() {
    // --- Car Body ---
    glPushMatrix();
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, carTex);
    glColor3f(1.0f, 1.0f, 1.0f); // ensure texture is not tinted
    glPushMatrix();
    glTranslatef(0.0f, 1.0f, 0.0f);
    drawTexturedBox(1.2f, 0.7f, 4.0f);
    glPopMatrix();
    glBindTexture(GL_TEXTURE_2D, 0);
    glDisable(GL_TEXTURE_2D);
    glPopMatrix();

    // --- Cockpit (textured) ---
    glPushMatrix();
    glTranslatef(0.0f, 1.1f, -0.5f);
    glScalef(0.8f, 0.6f, 1.0f);
    glColor3f(0.1f, 0.1f, 0.1f); // dark color for cockpit
    glutSolidCube(1.0f); // simple dark cube for cockpit
    glPopMatrix();
    // --- Front wing (textured) ---
    glPushMatrix();
    glTranslatef(0.0f, 0.95f, 3.0f);
    glScalef(2.0f, 0.1f, 0.4f);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, carTex);
    glColor3f(1.0f, 1.0f, 1.0f);
    drawTexturedBox(1.0f, 1.0f, 1.0f);
    glBindTexture(GL_TEXTURE_2D, 0);
    glDisable(GL_TEXTURE_2D);
    glPopMatrix();

    // --- Rear wing supports ---
    glPushMatrix();
    glTranslatef(0.3f, 1.5f, -2.2f);
    glScalef(0.1f, 0.8f, 0.1f);
    glColor3f(0.3f, 0.5f, 0.05f);
    glutSolidCube(1.0f);
    glPopMatrix();

    glPushMatrix();
    glTranslatef(-0.3f, 1.5f, -2.2f);
    glScalef(0.1f, 0.8f, 0.1f);
    glutSolidCube(1.0f);
    glPopMatrix();

    // --- Rear wing (textured) ---
    glPushMatrix();
    glTranslatef(0.0f, 1.85f, -2.2f);
    glScalef(1.8f, 0.1f, 0.3f);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, carTex);
    glColor3f(1.0f, 1.0f, 1.0f);
    drawTexturedBox(1.0f, 1.0f, 1.0f);
    glBindTexture(GL_TEXTURE_2D, 0);
    glDisable(GL_TEXTURE_2D);
    glPopMatrix();

    // --- Driver helmet ---
    glPushMatrix();
    glTranslatef(0.0f, 1.5f, -0.6f);
    GLUquadric* helmet = gluNewQuadric();
    gluQuadricTexture(helmet, GL_TRUE);
    gluQuadricNormals(helmet, GLU_SMOOTH);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, helmetTex);
    glColor3f(1.0f, 1.0f, 1.0f);
    gluSphere(helmet, 0.25f, 32, 16);
    glBindTexture(GL_TEXTURE_2D, 0);
    glDisable(GL_TEXTURE_2D);
    gluDeleteQuadric(helmet);
    glPopMatrix();

    // --- Nose cone (textured) ---
    glPushMatrix();
    glTranslatef(0.0f, 1.0f, 2.0f);
    GLUquadric* nose = gluNewQuadric();
    gluQuadricTexture(nose, GL_TRUE);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, carTex);
    glColor3f(1.0f, 1.0f, 1.0f);
    gluCylinder(nose, 0.3f, 0.1f, 1.5f, 16, 16);
    glBindTexture(GL_TEXTURE_2D, 0);
    glDisable(GL_TEXTURE_2D);
    gluDeleteQuadric(nose);
    glPopMatrix();

    // --- Front wing endplates ---
    glPushMatrix();
    glTranslatef(1.05f, 0.95f, 3.0f);
    glScalef(0.05f, 0.5f, 0.3f);
    glColor3f(0.0f, 0.0f, 0.0f);
    glutSolidCube(1.0f);
    glPopMatrix();

    glPushMatrix();
    glTranslatef(-1.05f, 0.95f, 3.0f);
    glScalef(0.05f, 0.5f, 0.3f);
    glutSolidCube(1.0f);
    glPopMatrix();

    // --- Wheels ---
    float wheelY = 0.7f;
    float wheelZFront = 1.7f;
    float wheelZRear = -1.8f;
    float wheelOffsetX = 1.5f;

    auto drawWheelAndArm = [&](float x, float z, float radius, float width) {
        // Suspension/arm
        glPushMatrix();
        glTranslatef(x * 0.7f, wheelY + 0.2f, z * 0.9f);
        glRotatef(90.0f, 0.0f, 1.0f, 0.0f);
        glScalef(0.2f, 0.1f, 0.8f);
        glColor3f(0.1f, 0.1f, 0.1f);
        glutSolidCube(1.0f);
        glPopMatrix();

        // Wheel
        glPushMatrix();
        glTranslatef(x, wheelY, z);
        glRotatef(tireRotation * (carSpeed >= 0 ? 1 : -1), 1, 0, 0);
        drawWheel(radius, width, 6); // 6-spoke alloy wheel
        glPopMatrix();
        };

    drawWheelAndArm(wheelOffsetX - 0.5f, wheelZFront, 0.65f, 0.5f);
    drawWheelAndArm(-wheelOffsetX, wheelZFront, 0.65f, 0.5f);
    drawWheelAndArm(wheelOffsetX - 0.7f, wheelZRear, 0.65f, 0.7f);
    drawWheelAndArm(-wheelOffsetX, wheelZRear, 0.65f, 0.7f);
}

void generateTrees(int numTrees) {
    trees.clear();
    for (int i = 0; i < numTrees; i++) {
        int idx = rand() % innerTrack.size();
        float dx = outerTrack[idx].first - innerTrack[idx].first;
        float dz = outerTrack[idx].second - innerTrack[idx].second;
        float len = sqrtf(dx * dx + dz * dz);
        dx /= len; dz /= len;

        float offset = TRACK_WIDTH * 4.0f + randf(10, 60);
        float side = (rand() % 2 == 0 ? 1 : -1);

        Tree t;
        t.x = (innerTrack[idx].first + outerTrack[idx].first) * 0.5f + side * dx * offset;
        t.z = (innerTrack[idx].second + outerTrack[idx].second) * 0.5f + side * dz * offset;
        t.height = randf(8.0f, 16.0f);
        t.radius = randf(0.5f, 1.2f);
        trees.push_back(t);
    }
}

void generateTrackObjects(int numObjects) {
    trackObjects.clear();
    for (int i = 0; i < numObjects; i++) {
        int idx = rand() % innerTrack.size();

        // Direction vector along track segment
        float dx = outerTrack[(idx + 1) % innerTrack.size()].first - outerTrack[idx].first;
        float dz = outerTrack[(idx + 1) % outerTrack.size()].second - outerTrack[idx].second;
        float len = sqrtf(dx * dx + dz * dz);
        dx /= len; dz /= len;

        // Perpendicular vector to the track segment
        float perpX = -dz;
        float perpZ = dx;

        // Random side (+/-) and offset distance
        float side = (rand() % 2 == 0) ? 1.0f : -1.0f;  // left or right
        float minOffset = TRACK_WIDTH * 1.2f;          // avoid middle of track
        float maxOffset = TRACK_WIDTH * 4.0f;
        float offset = minOffset + randf(0.0f, maxOffset - minOffset);

        TrackObject o;
        o.x = outerTrack[idx].first + perpX * offset * side;
        o.z = outerTrack[idx].second + perpZ * offset * side;
        o.type = rand() % 4; // pick random object type
        trackObjects.push_back(o);
    }
}

void drawTree(const Tree& t, GLuint leafTexture) {
    glPushMatrix();
    glTranslatef(t.x, 0.0f, t.z);  // Move tree to world position

    // --- Draw Trunk ---
    GLUquadric* quad = gluNewQuadric();
    gluQuadricDrawStyle(quad, GLU_FILL);
    gluQuadricNormals(quad, GLU_SMOOTH);

    float trunkHeight = t.height * 0.08f;
    float baseRadius = t.radius * 0.3f;
    float topRadius = t.radius * 0.25f;

    glRotatef(-90.0f, 1.0f, 0.0f, 0.0f);  // Keep your original trunk rotation
    glColor3f(0.55f, 0.27f, 0.07f);
    gluCylinder(quad, baseRadius, topRadius, trunkHeight, 16, 4);

    // Cap the bottom
    glPushMatrix();
    glRotatef(180.0f, 1.0f, 0.0f, 0.0f);
    gluDisk(quad, 0.0f, baseRadius, 16, 4);
    glPopMatrix();

    // --- Draw Canopy with texture ---
    glTranslatef(0.0f, 0.0f, trunkHeight); // move to top of trunk
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, leafTexture);
    glColor3f(1.0f, 1.0f, 1.0f); // White to preserve texture colors

    int layers = 4;
    for (int i = 0; i < layers; ++i) {
        float layerHeight = (t.height * 0.6f) / layers;
        float radius = t.radius * (5.0f - .3f * i);

        glPushMatrix();
        glTranslatef(0.0f, 0.0f, i * layerHeight * 0.8f);

        GLUquadric* canopy = gluNewQuadric();
        gluQuadricTexture(canopy, GL_TRUE);   // enable texturing
        gluQuadricNormals(canopy, GLU_SMOOTH);
        gluCylinder(canopy, radius, 0.0f, layerHeight * 1.4f, 20, 10);
        gluDeleteQuadric(canopy);

        glPopMatrix();
    }

    // --- Add textured top sphere ---
    glPushMatrix();
    glTranslatef(0.0f, 0.0f, layers * (t.height * 0.6f) / layers * 0.8f);

    GLUquadric* topSphere = gluNewQuadric();
    gluQuadricTexture(topSphere, GL_TRUE);
    gluQuadricNormals(topSphere, GLU_SMOOTH);
    gluSphere(topSphere, t.radius * 0.4f, 16, 16);
    gluDeleteQuadric(topSphere);

    glPopMatrix();

    glDisable(GL_TEXTURE_2D);  // disable texture after drawing canopy
    gluDeleteQuadric(quad);
    glPopMatrix();
}




void drawTrackObject(const TrackObject& o) {
    glPushMatrix();
    glTranslatef(o.x, 0.0f, o.z);

    switch (o.type) {
    case 0: // Tire stack
        glColor3f(0.1f, 0.1f, 0.1f);
        for (int i = 0; i < 3; i++) {
            glPushMatrix();
            glTranslatef(0.0f, i * 0.5f, 0.0f);
            glutSolidTorus(0.15f, 0.4f, 16, 16);
            glPopMatrix();
        }
        break;

    case 1: // Barrier
        glColor3f(0.9f, 0.1f, 0.1f);
        glScalef(2.0f, 1.0f, 0.5f);
        glutSolidCube(1.0f);
        break;

    case 2: // Lamp post
        glColor3f(0.3f, 0.3f, 0.3f);
        glPushMatrix();
        glScalef(0.1f, 5.0f, 0.1f);
        glutSolidCube(1.0f);
        glPopMatrix();
        glTranslatef(0.0f, 2.5f, 0.0f);
        glColor3f(1.0f, 1.0f, 0.8f);
        glutSolidSphere(0.3f, 16, 16);
        break;

    case 3: // Banner
        glColor3f(0.0f, 0.0f, 1.0f);
        glPushMatrix();
        glScalef(4.0f, 2.0f, 0.2f);
        glutSolidCube(1.0f);
        glPopMatrix();
        break;
    }
    glPopMatrix();
}

// ==================== DRAW TRACK ====================
void drawMiddleLine(float dashLength = 2.0f, float gapLength = 1.0f) {
    if (innerTrack.empty() || outerTrack.empty()) return;

    glDisable(GL_LIGHTING);
    glColor3f(1.0f, 1.0f, 1.0f); // white line
    glLineWidth(6.0f);

    int n = (int)innerTrack.size();

    for (int i = 0; i < n; i++) {
        // Current and next middle point
        int nextIdx = (i + 1) % n;
        float x0 = (innerTrack[i].first + outerTrack[i].first) * 0.5f;
        float z0 = (innerTrack[i].second + outerTrack[i].second) * 0.5f;
        float x1 = (innerTrack[nextIdx].first + outerTrack[nextIdx].first) * 0.5f;
        float z1 = (innerTrack[nextIdx].second + outerTrack[nextIdx].second) * 0.5f;

        float dx = x1 - x0;
        float dz = z1 - z0;
        float segmentLength = sqrtf(dx * dx + dz * dz);

        // Direction vector normalized
        float dirX = dx / segmentLength;
        float dirZ = dz / segmentLength;

        float traveled = 0.0f;
        bool drawDash = true;

        while (traveled < segmentLength) {
            float start = traveled;
            float end = std::min(traveled + (drawDash ? dashLength : gapLength), segmentLength);

            if (drawDash) {
                glBegin(GL_LINES);
                glVertex3f(x0 + dirX * start, 0.02f, z0 + dirZ * start);
                glVertex3f(x0 + dirX * end, 0.02f, z0 + dirZ * end);
                glEnd();
            }

            traveled = end;
            drawDash = !drawDash;
        }
    }

    glEnable(GL_LIGHTING);
}
void drawTrack() {
    glDisable(GL_LIGHTING);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, asphaltTex);

    glColor3f(1.0f, 1.0f, 1.0f); // let texture color show

    glBegin(GL_QUAD_STRIP);
    int n = (int)innerTrack.size();
    float texRepeat = 40.0f;  // repeat texture along track

    for (int i = 0; i <= n; ++i) {
        int idx = i % n;

        float s = (i / (float)n) * texRepeat;

        glTexCoord2f(s, 0.0f);
        glVertex3f(outerTrack[idx].first, 0.01f, outerTrack[idx].second);

        glTexCoord2f(s, 1.0f);
        glVertex3f(innerTrack[idx].first, 0.01f, innerTrack[idx].second);
    }
    glEnd();

    glDisable(GL_TEXTURE_2D);
    glEnable(GL_LIGHTING);
}
void drawStartLine() {
    int numCols = 16; // across width
    int numRows = 8;  // along track
    float widthScale = 1.1f; // wider than track
    float squareWidth = (TRACK_WIDTH * widthScale) / numCols;
    float rowLength = startLineLength / numRows;

    glPushMatrix();
    glTranslatef(startLineCenter.first-0.7f, 0.31f, startLineCenter.second); // slightly above track
    glRotatef(startLineAngle, 0.0f, 1.0f, 0.0f);

    // --- Draw checkered start line ---
    for (int r = 0; r < numRows; r++) {
        for (int c = 0; c < numCols; c++) {
            if ((r + c) % 2 == 0) glColor3f(1.0f, 1.0f, 1.0f);
            else glColor3f(0.0f, 0.0f, 0.0f);

            glBegin(GL_QUADS);
            glVertex3f(-TRACK_WIDTH * widthScale / 2 + c * squareWidth, 0, -startLineLength / 2 + r * rowLength);
            glVertex3f(-TRACK_WIDTH * widthScale / 2 + (c + 1) * squareWidth, 0, -startLineLength / 2 + r * rowLength);
            glVertex3f(-TRACK_WIDTH * widthScale / 2 + (c + 1) * squareWidth, 0, -startLineLength / 2 + (r + 1) * rowLength);
            glVertex3f(-TRACK_WIDTH * widthScale / 2 + c * squareWidth, 0, -startLineLength / 2 + (r + 1) * rowLength);
            glEnd();
        }
    }

    // --- Draw flag pole ---
    float poleHeight = 3.0f;
    float poleRadius = 0.05f;
    glColor3f(0.3f, 0.3f, 0.3f);

    glPushMatrix();
    glTranslatef(TRACK_WIDTH * widthScale / 2 + 0.2f, 0.0f, 0.0f); // side of track
    glRotatef(-90.0f, 1.0f, 0.0f, 0.0f); // align vertical
    GLUquadric* quad = gluNewQuadric();
    gluCylinder(quad, poleRadius, poleRadius, poleHeight, 12, 3);
    gluDeleteQuadric(quad);
    glPopMatrix();

    // --- Draw small checkered flag ---
    glPushMatrix();
    glTranslatef(TRACK_WIDTH * widthScale / 2 + 0.2f, poleHeight - 0.1f, 0.0f);
    float flagWidth = 1.0f;
    float flagHeight = 0.7f;
    int flagCols = 4, flagRows = 3;
    float fSquareW = flagWidth / flagCols;
    float fSquareH = flagHeight / flagRows;

    for (int r = 0; r < flagRows; r++) {
        for (int c = 0; c < flagCols; c++) {
            if ((r + c) % 2 == 0) glColor3f(1.0f, 1.0f, 1.0f);
            else glColor3f(0.0f, 0.0f, 0.0f);

            glBegin(GL_QUADS);
            glVertex3f(c * fSquareW, r * fSquareH, 0.0f);
            glVertex3f((c + 1) * fSquareW, r * fSquareH, 0.0f);
            glVertex3f((c + 1) * fSquareW, (r + 1) * fSquareH, 0.0f);
            glVertex3f(c * fSquareW, (r + 1) * fSquareH, 0.0f);
            glEnd();
        }
    }
    glPopMatrix();

    glPopMatrix();
}


// ==================== TIRES ====================

void drawTire(float x, float y, float z, float radius = 0.35f, float width = 0.15f) {
    GLUquadric* quad = gluNewQuadric();
    gluQuadricTexture(quad, GL_TRUE);
    gluQuadricNormals(quad, GLU_SMOOTH);

    glPushMatrix();
    glTranslatef(x, y, z);
    glRotatef(90, 1, 0, 0); // align cylinder along X axis

    // Tire tread (textured)
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, tireTexture);
    glColor3f(1.0f, 1.0f, 1.0f);
    gluCylinder(quad, radius, radius, width, 32, 1);

    // Front disk (non-textured)
    glDisable(GL_TEXTURE_2D);
    glColor3f(0.05f, 0.05f, 0.05f);
    gluDisk(quad, 0.0, radius, 32, 1);

    // Back disk
    glPushMatrix();
    glTranslatef(0, 0, width);
    gluDisk(quad, 0.0, radius, 32, 1);
    glPopMatrix();

    // Wheel hub
    glColor3f(0.7f, 0.7f, 0.75f);
    glPushMatrix();
    glTranslatef(0, 0, width / 2.0f);
    gluDisk(quad, 0.0, radius * 0.5f, 32, 1);
    glPopMatrix();

    glPopMatrix();
    gluDeleteQuadric(quad);
}



// ==================== BUILD DISPLAY LISTS ====================
void buildDisplayLists() {
    if (trackList != 0) glDeleteLists(trackList, 1);
    if (tireList != 0) glDeleteLists(tireList, 1);

    trackList = glGenLists(1);
    glNewList(trackList, GL_COMPILE);
    drawTrack();
    glEndList();

    tireList = glGenLists(1);
    glNewList(tireList, GL_COMPILE);
    //placeTires();
    glEndList();
}

// ==================== CAR MOVEMENT ====================
void resetCar() {
    // Place car on the start line
    carX = startLineCenter.first;
    carZ = startLineCenter.second;
    carAngle = startLineAngle; // face along track direction
    carSpeed = 0.0f;

    tireRotation = 0.0f; // reset wheel rotation if needed
}

void idleFunc() {
    int currentTime = glutGet(GLUT_ELAPSED_TIME);
    float deltaTime = (currentTime - lastTime) / 1000.0f;
    lastTime = currentTime;

    if (deltaTime > 0.1f) deltaTime = 0.1f; // clamp deltaTime

    // --- Update car speed ---
    if (moveForward) {
        carSpeed += ACCELERATION * deltaTime;
        if (carSpeed > MAX_SPEED_FW) carSpeed = MAX_SPEED_FW;
    }
    else if (moveBackward) {
        carSpeed -= ACCELERATION * deltaTime;
        if (carSpeed < -MAX_SPEED_BW) carSpeed = -MAX_SPEED_BW;
    }
    else {
        if (carSpeed > 0.0f) {
            carSpeed -= FRICTION * deltaTime;
            if (carSpeed < 0.0f) carSpeed = 0.0f;
        }
        else if (carSpeed < 0.0f) {
            carSpeed += FRICTION * deltaTime;
            if (carSpeed > 0.0f) carSpeed = 0.0f;
        }
    }

    // --- Update car rotation ---
    float turnSpeed = TURN_ANGLE * deltaTime;
    if (moveLeft) carAngle += turnSpeed;
    if (moveRight) carAngle -= turnSpeed;
    if (carAngle > 180.0f) carAngle -= 360.0f;
    if (carAngle < -180.0f) carAngle += 360.0f;

    // --- Update car position ---
    float rad = carAngle * M_PI_F / 180.0f;
    float distance = carSpeed * deltaTime;
    carX += distance * sinf(rad);
    carZ += distance * cosf(rad);

    // --- Update tire rotation ---
    float wheelRadius = 0.3f; // adjust according to your wheel size
    float wheelCircumference = 2.0f * M_PI_F * wheelRadius;
    float rotationDelta = (distance / wheelCircumference) * 360.0f; // degrees
    tireRotation += rotationDelta; // don't multiply by deltaTime or 50
    if (tireRotation > 360.0f) tireRotation -= 360.0f;
    if (tireRotation < -360.0f) tireRotation += 360.0f;

    glutPostRedisplay();
}


// ==================== INPUT ====================
void keyDown(unsigned char key, int, int) {
    switch (key) {
    case 'w': case 'W': moveForward = true; break;
    case 's': case 'S': moveBackward = true; break;
    case 'a': case 'A': moveLeft = true; break;
    case 'd': case 'D': moveRight = true; break;
    case 'r': case 'R': resetCar(); break;
    case 27: exit(0); // ESC
    }
}

void keyUp(unsigned char key, int, int) {
    switch (key) {
    case 'w': case 'W': moveForward = false; break;
    case 's': case 'S': moveBackward = false; break;
    case 'a': case 'A': moveLeft = false; break;
    case 'd': case 'D': moveRight = false; break;
    }
}
void specialKeyDown(int key, int, int) {
    switch (key) {
    case GLUT_KEY_LEFT:  camYawOffset -= 5.0f; break; // was +=
    case GLUT_KEY_RIGHT: camYawOffset += 5.0f; break; // was -=
    case GLUT_KEY_UP:    camPitchOffset += 3.0f; break;
    case GLUT_KEY_DOWN:  camPitchOffset -= 3.0f; break;
    }
    if (camPitchOffset > 80.0f)  camPitchOffset = 80.0f;
    if (camPitchOffset < -30.0f) camPitchOffset = -30.0f;
    glutPostRedisplay();
}


void generateTrackPoints() {
    innerTrack.clear();
    outerTrack.clear();
    std::vector<std::pair<float, float>> centerline;

    const int numSegments = 10;
    const float minStraight = 30.0f;
    const float maxStraight = 80.0f;
    const float minCurveRadius = 30.0f;
    const float maxCurveRadius = 80.0f;

    float angle = 0.0f;
    float x = 0.0f, z = 0.0f;

    // --- Generate rough track path ---
    for (int s = 0; s < numSegments; s++) {
        // Straight
        float straightLen = minStraight + ((float)rand() / RAND_MAX) * (maxStraight - minStraight);
        float dx = cosf(angle * M_PI_F / 180.0f);
        float dz = sinf(angle * M_PI_F / 180.0f);

        int stepsStraight = (int)(straightLen / 5.0f);
        for (int i = 0; i < stepsStraight; i++) {
            x += dx * 5.0f;
            z += dz * 5.0f;
            centerline.push_back({ x, z });
        }

        // Curve
        int turnDir = (rand() % 2 == 0 ? -1 : 1);
        float turnAngle = 45.0f + ((float)rand() / RAND_MAX) * 75.0f; // 45–120°
        float curveRadius = minCurveRadius + ((float)rand() / RAND_MAX) * (maxCurveRadius - minCurveRadius);
        int stepsCurve = 20;

        float arcStep = (M_PI_F * curveRadius * (turnAngle / 360.0f)) / stepsCurve;

        for (int i = 0; i < stepsCurve; i++) {
            angle += turnDir * (turnAngle / stepsCurve);
            dx = cosf(angle * M_PI_F / 180.0f);
            dz = sinf(angle * M_PI_F / 180.0f);
            x += dx * arcStep;
            z += dz * arcStep;
            centerline.push_back({ x, z });
        }
    }

    // --- Close loop smoothly without duplicating the first point ---
    centerline.push_back({
        (centerline.back().first + centerline[0].first) * 0.5f,
        (centerline.back().second + centerline[0].second) * 0.5f
        });

    // --- Smooth with Catmull-Rom ---
    std::vector<std::pair<float, float>> smoothLine;
    for (size_t i = 0; i < centerline.size(); ++i) {
        std::pair<float, float> p0 = (i == 0) ? centerline[i] : centerline[i - 1];
        std::pair<float, float> p1 = centerline[i];
        std::pair<float, float> p2 = (i + 1 < centerline.size()) ? centerline[i + 1] : centerline[0];
        std::pair<float, float> p3 = (i + 2 < centerline.size()) ? centerline[i + 2] : centerline[1];

        int interpSteps = 10;
        for (int t = 0; t < interpSteps; ++t) {
            float alpha = t / (float)interpSteps;
            smoothLine.push_back(catmullRom(p0, p1, p2, p3, alpha));
        }
    }

    // --- Resample evenly for physics/AI ---
    std::vector<std::pair<float, float>> resampled;
    float stepSize = 2.0f; // spacing between samples
    float dist = 0.0f;

    resampled.push_back(smoothLine[0]);
    for (size_t i = 1; i < smoothLine.size(); ++i) {
        float dx = smoothLine[i].first - smoothLine[i - 1].first;
        float dz = smoothLine[i].second - smoothLine[i - 1].second;
        float segLen = sqrtf(dx * dx + dz * dz);
        dist += segLen;

        if (dist >= stepSize) {
            resampled.push_back(smoothLine[i]);
            dist = 0.0f;
        }
    }

    // --- Build inner/outer edges ---
    for (size_t i = 0; i < resampled.size(); i++) {
        size_t j = (i + 1) % resampled.size();
        float dx = resampled[j].first - resampled[i].first;
        float dz = resampled[j].second - resampled[i].second;

        float len = sqrtf(dx * dx + dz * dz);
        if (len == 0) len = 1;
        dx /= len;
        dz /= len;

        float px = -dz;
        float pz = dx;

        float innerX = resampled[i].first - px * (TRACK_WIDTH * 0.5f);
        float innerZ = resampled[i].second - pz * (TRACK_WIDTH * 0.5f);
        float outerX = resampled[i].first + px * (TRACK_WIDTH * 0.5f);
        float outerZ = resampled[i].second + pz * (TRACK_WIDTH * 0.5f);

        innerTrack.push_back({ innerX, innerZ });
        outerTrack.push_back({ outerX, outerZ });
    }

    // --- Choose a clean start/finish line a bit into the track ---
    int spawnIndex = 10; // skip first few to avoid overlap
    startLineCenter = resampled[spawnIndex]; 
    size_t nextIndex = (spawnIndex + 1) % resampled.size();
    float dx = resampled[nextIndex].first - startLineCenter.first;
    float dz = resampled[nextIndex].second - startLineCenter.second;
    startLineAngle = atan2f(dz, dx) * 180.0f / M_PI_F; // degrees for OpenGL// <-- store for car spawn
    float rotationOffset = 90.0f; // degrees
    startLineAngle += rotationOffset;
}




void drawKerbs() {
    if (innerTrack.empty() || outerTrack.empty()) return;

    int n = (int)innerTrack.size();
    float kerbWidth = 2.0f; // how far outside the track
    float kerbHeight = 0.2f;

    for (int i = 0; i < n; i++) {
        int j = (i + 1) % n;

        // Inner edge
        {
            float x1 = innerTrack[i].first;
            float z1 = innerTrack[i].second;
            float x2 = innerTrack[j].first;
            float z2 = innerTrack[j].second;

            // Outward offset for kerb
            float dx = x2 - x1;
            float dz = z2 - z1;
            float len = sqrtf(dx * dx + dz * dz);
            if (len == 0) len = 1;
            dx /= len;
            dz /= len;

            // Perpendicular inward
            float px = dz;
            float pz = -dx;

            float kx1 = x1 - px * kerbWidth;
            float kz1 = z1 - pz * kerbWidth;
            float kx2 = x2 - px * kerbWidth;
            float kz2 = z2 - pz * kerbWidth;

            if (i % 2 == 0) glColor3f(1.0f, 0.0f, 0.0f); // red
            else glColor3f(1.0f, 1.0f, 1.0f);           // white

            glBegin(GL_QUADS);
            glVertex3f(x1, kerbHeight, z1);
            glVertex3f(x2, kerbHeight, z2);
            glVertex3f(kx2, kerbHeight, kz2);
            glVertex3f(kx1, kerbHeight, kz1);
            glEnd();
        }

        // Outer edge
        {
            float x1 = outerTrack[i].first;
            float z1 = outerTrack[i].second;
            float x2 = outerTrack[j].first;
            float z2 = outerTrack[j].second;

            float dx = x2 - x1;
            float dz = z2 - z1;
            float len = sqrtf(dx * dx + dz * dz);
            if (len == 0) len = 1;
            dx /= len;
            dz /= len;

            // Perpendicular outward
            float px = -dz;
            float pz = dx;

            float kx1 = x1 + px * kerbWidth;
            float kz1 = z1 + pz * kerbWidth;
            float kx2 = x2 + px * kerbWidth;
            float kz2 = z2 + pz * kerbWidth;

            if (i % 2 == 0) glColor3f(1.0f, 0.0f, 0.0f); // red
            else glColor3f(1.0f, 1.0f, 1.0f);           // white

            glBegin(GL_QUADS);
            glVertex3f(x1, kerbHeight, z1);
            glVertex3f(x2, kerbHeight, z2);
            glVertex3f(kx2, kerbHeight, kz2);
            glVertex3f(kx1, kerbHeight, kz1);
            glEnd();
        }
    }
}


//void drawFinishLine() {
//    if (innerTrack.empty() || outerTrack.empty()) return;
//
//    float finishX1 = innerTrack[0].first;
//    float finishZ1 = innerTrack[0].second;
//    float finishX2 = outerTrack[0].first;
//    float finishZ2 = outerTrack[0].second;
//
//    int squares = 12;
//    for (int i = 0; i < squares; i++) {
//        float t1 = i / (float)squares;
//        float t2 = (i + 1) / (float)squares;
//        float x1 = finishX1 * (1 - t1) + finishX2 * t1;
//        float z1 = finishZ1 * (1 - t1) + finishZ2 * t1;
//        float x2 = finishX1 * (1 - t2) + finishX2 * t2;
//        float z2 = finishZ1 * (1 - t2) + finishZ2 * t2;
//
//        if (i % 2 == 0) glColor3f(1.0f, 1.0f, 1.0f);
//        else glColor3f(0.0f, 0.0f, 0.0f);
//
//        glBegin(GL_QUADS);
//        glVertex3f(x1, 0.05f, z1);
//        glVertex3f(x2, 0.05f, z2);
//        glVertex3f(x2, 0.1f, z2);
//        glVertex3f(x1, 0.1f, z1);
//        glEnd();
//    }
//}

void placeTreesAndObjects(int numItems, bool isTree) {
    if (isTree) trees.clear();
    else trackObjects.clear();

    int totalSegments = outerTrack.size();

    for (int i = 0; i < numItems; i++) {
        int idx = rand() % totalSegments;
        size_t nextIdx = (idx + 1) % totalSegments;

        // Compute direction of track segment
        float dx = outerTrack[nextIdx].first - outerTrack[idx].first;
        float dz = outerTrack[nextIdx].second - outerTrack[idx].second;
        float len = sqrtf(dx * dx + dz * dz);
        if (len == 0) continue; // skip zero-length segments
        dx /= len; dz /= len;

        // Perpendicular vector
        float px = -dz;
        float pz = dx;

        // Skip sharp curves: compute angle between consecutive segments
        size_t prevIdx = (idx + totalSegments - 1) % totalSegments;
        float pdx = outerTrack[idx].first - outerTrack[prevIdx].first;
        float pdz = outerTrack[idx].second - outerTrack[prevIdx].second;
        float plen = sqrtf(pdx * pdx + pdz * pdz);
        if (plen == 0) plen = 1;
        pdx /= plen; pdz /= plen;

        float dot = dx * pdx + dz * pdz; // cosine of angle
        if (dot < 0.5f) { // skip very sharp curves
            i--;
            continue;
        }

        // Random side
        float side = (rand() % 2 == 0) ? 1.0f : -1.0f;

        // Offset: start just outside track + some random variation
        float minOffset = TRACK_WIDTH * 1.0f;
        float maxOffset = TRACK_WIDTH * 4.0f;
        float offset = minOffset + randf(0.0f, maxOffset - minOffset);
        
        float xPos = outerTrack[idx].first + px * offset * side;
        float zPos = outerTrack[idx].second + pz * offset * side;

        if (isTree) {
            Tree t;
            t.x = xPos;
            t.z = zPos;
            t.height = randf(8.0f, 15.0f);  // keep original heights
            t.radius = randf(0.5f, 1.0f);   // slimmer trees
            trees.push_back(t);
        }
        else {
            TrackObject o;
            o.x = xPos;
            o.z = zPos;
            o.type = rand() % 4;
            trackObjects.push_back(o);
        }
    }
}
void drawTrackTires() {
    GLUquadric* quad = gluNewQuadric();
    float tireRadius = 0.4f;
    float tireWidth = 0.3f;
    int stackHeight = 3;
    float spacing = 2.0f;

    float innerOffset = 0.2f; // smaller, closer to inner edge
    float outerOffset = -0.7f; // larger, leaning toward track

    auto drawStack = [&](float x, float z, float angle, bool leanInward, float offset) {
        float rad = angle * M_PI_F / 180.0f;
        float px = -sinf(rad);
        float pz = cosf(rad);

        if (leanInward) { px *= -1; pz *= -1; }

        x += px * offset;
        z += pz * offset;

        for (int h = 0; h < stackHeight; h++) {
            glPushMatrix();
            glTranslatef(x, tireRadius + h * tireWidth, z);
            glRotatef(angle, 0, 1, 0);
            glRotatef(90.0f, 1, 0, 0);
            if (h % 2 == 0) glColor3f(1, 0, 0);
            else glColor3f(1, 1, 1);
            gluCylinder(quad, tireRadius, tireRadius, tireWidth, 12, 3);
            glPopMatrix();
        }
        };

    // --- Inner track ---
    for (size_t i = 0; i < innerTrack.size(); i++) {
        size_t next = (i + 1) % innerTrack.size();
        float dx = innerTrack[next].first - innerTrack[i].first;
        float dz = innerTrack[next].second - innerTrack[i].second;
        float segLen = sqrtf(dx * dx + dz * dz);
        dx /= segLen; dz /= segLen;
        float angleDeg = atan2f(dz, dx) * 180.0f / M_PI_F;

        int numStacks = (int)(segLen / spacing);
        for (int s = 0; s < numStacks; s++) {
            float x = innerTrack[i].first + dx * s * spacing;
            float z = innerTrack[i].second + dz * s * spacing;
            drawStack(x, z, angleDeg, false, innerOffset); // use inner offset
        }
    }

    // --- Outer track ---
    for (size_t i = 0; i < outerTrack.size(); i++) {
        size_t next = (i + 1) % outerTrack.size();
        float dx = outerTrack[next].first - outerTrack[i].first;
        float dz = outerTrack[next].second - outerTrack[i].second;
        float segLen = sqrtf(dx * dx + dz * dz);
        dx /= segLen; dz /= segLen;
        float angleDeg = atan2f(dz, dx) * 180.0f / M_PI_F;

        int numStacks = (int)(segLen / spacing);
        for (int s = 0; s < numStacks; s++) {
            float x = outerTrack[i].first + dx * s * spacing;
            float z = outerTrack[i].second + dz * s * spacing;
            drawStack(x, z, angleDeg, true, outerOffset); // use outer offset
        }
    }

    gluDeleteQuadric(quad);
}
// ==================== RESHAPE & INIT ====================
void reshape(int w, int h) {
    if (h == 0) h = 1;
    float ratio = (float)w / (float)h;
    glViewport(0, 0, w, h);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(45.0f, ratio, 0.1f, 1000.0f);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}
void display() {
    
    static float t = 0.0f;
    t += 0.05f;
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    // Camera follows behind the car
    float rad = (carAngle - camYawOffset) * M_PI_F / 180.0f;
    float camDist = 12.0f;
    float camX = carX - camDist * sinf(rad);
    float camZ = carZ - camDist * cosf(rad);
    float camY = 6.0f + sinf(camPitchOffset * M_PI_F / 180.0f) * 4.0f;

    gluLookAt(camX, camY, camZ, carX, 1.0f, carZ, 0, 1, 0);

    // Ground plane
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, grassTex);
    glDisable(GL_LIGHTING);

    glColor3f(1.0f, 1.0f, 1.0f);
    glBegin(GL_QUADS);
    float R = 500.0f;
    float texRepeat = 50.0f;
    glTexCoord2f(0.0f, 0.0f); glVertex3f(-R, 0.0f, -R);
    glTexCoord2f(texRepeat, 0.0f); glVertex3f(R, 0.0f, -R);
    glTexCoord2f(texRepeat, texRepeat); glVertex3f(R, 0.0f, R);
    glTexCoord2f(0.0f, texRepeat); glVertex3f(-R, 0.0f, R);
    glEnd();

    glEnable(GL_LIGHTING);
    glDisable(GL_TEXTURE_2D);

    // Draw track display list
    if (trackList != 0) {
        glCallList(trackList);
    }
    drawTrackTires();
    // Draw kerbs and finish line
    drawKerbs();
  //  drawFinishLine();
    drawStartLine();   // <-- draws your black-and-white start line
    drawBuildings();
    // Draw car
    glPushMatrix();
    glTranslatef(carX, 0.0f, carZ);
    glRotatef(carAngle, 0, 1, 0);
    drawF1Car();
    glPopMatrix();

    drawAudience();
    for (const auto& s : stands) drawStand(s);
    for (const auto& t : trees) drawTree(t,treeTexture);
    for (const auto& o : trackObjects) drawTrackObject(o);

    glCallList(trackList);
    drawMiddleLine();

    glutSwapBuffers();
}

void initGL() {
    glClearColor(0.5f, 0.8f, 0.95f, 1.0f);
    glEnable(GL_DEPTH_TEST);
    glShadeModel(GL_SMOOTH);
    glEnable(GL_NORMALIZE);

    grassTex = loadTexture("grass.jpg");
    asphaltTex = loadTexture("asphalt.jpg");
    tireTexture = loadTexture("tire.jpg");
	carTex = loadTexture("car.jpg");
	helmetTex = loadTexture("helmut.jpg");
    treeTexture = loadTexture("trees.jpg");
    buildingTexture = loadTexture("building.jpg");



    setupLights();
    lastTime = glutGet(GLUT_ELAPSED_TIME);

    generateTrackPoints();
    generateAllAudience();
    buildDisplayLists();
    generateTrees(80);


    placeTreesAndObjects(80, true);   // generate 80 trees
    placeTreesAndObjects(40, false);   // about 40 random objects
    float trackMinX = 1e6f, trackMaxX = -1e6f;
    float trackMinZ = 1e6f, trackMaxZ = -1e6f;

    for (auto& p : innerTrack) {
        if (p.first < trackMinX) trackMinX = p.first;
        if (p.first > trackMaxX) trackMaxX = p.first;
        if (p.second < trackMinZ) trackMinZ = p.second;
        if (p.second > trackMaxZ) trackMaxZ = p.second;
    }

    for (auto& p : outerTrack) {
        if (p.first < trackMinX) trackMinX = p.first;
        if (p.first > trackMaxX) trackMaxX = p.first;
        if (p.second < trackMinZ) trackMinZ = p.second;
        if (p.second > trackMaxZ) trackMaxZ = p.second;
    }
    generateBuildings(50, trackMinX, trackMaxX, trackMinZ, trackMaxZ, 10.0f);

}


// ==================== MAIN ====================
int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH);
    glutInitWindowSize(1100, 700);
    glutCreateWindow("F1 Car Circuit (Constant Width)");

    initGL();
    resetCar();
    lastTime = glutGet(GLUT_ELAPSED_TIME);

    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutIdleFunc(idleFunc);
    glutKeyboardFunc(keyDown);
    glutKeyboardUpFunc(keyUp);
    glutSpecialFunc(specialKeyDown);


    glutMainLoop();
    return 0;


}
