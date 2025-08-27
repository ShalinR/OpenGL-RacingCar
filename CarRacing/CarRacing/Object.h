#ifndef OBJECT_H
#define OBJECT_H

#include "Position.h"
#include <GL/glut.h>

class Object {
private:
    float width, height, depth;
    Position* pos;

public:
    bool isColliding;

    Object(float w, float h, float d, Position* p)
        : width(w), height(h), depth(d), pos(p), isColliding(false) {
    }

    void draw() {
        if (isColliding) glColor3f(1.0, 0.0, 0.0); // Red if colliding
        else glColor3f(0.5, 0.5, 0.5); // Gray

        glPushMatrix();
        glTranslatef(pos->x, pos->y, pos->z);
        glRotatef(pos->angle, 0, 1, 0);

        // Draw cube as barrier
        glutSolidCube(1.0);

        glPopMatrix();
    }

    void resetIsColliding() {
        isColliding = false;
    }
};

#endif