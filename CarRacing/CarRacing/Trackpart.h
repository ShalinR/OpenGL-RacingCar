#ifndef TRACKPART_H
#define TRACKPART_H

#include <vector>
#include <GL/glut.h>
#include <cmath>

// Simple Position struct to hold coordinates and angle
struct Position {
    float x, y, z, angle;
    Position(float X = 0, float Y = 0, float Z = 0, float A = 0)
        : x(X), y(Y), z(Z), angle(A) {}
};

// TrackPart class declaration
class TrackPart {
private:
    float width;
    Position pos;
    std::vector<Position> vertices;

public:
    // Constructor
    TrackPart(float w, Position p);

    // Compute the vertices of the track part
    void computeVertices();

    // Draw the track part
    void draw();

    // Getters
    Position getPosition() const;
    float getWidth() const;
};

#endif // TRACKPART_H
