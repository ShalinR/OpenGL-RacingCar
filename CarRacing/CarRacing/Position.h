#ifndef POSITION_H
#define POSITION_H

class Position {
public:
    float x, y, z, angle; // angle in degrees

    Position(float x = 0, float y = 0, float z = 0, float angle = 0)
        : x(x), y(y), z(z), angle(angle) {
    }
};

#endif