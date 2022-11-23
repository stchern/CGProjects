#ifndef BALL_H
#define BALL_H

#include "SFML/Graphics.hpp"
#include <math.h>

struct Ball
{
    sf::Vector2f p;
    sf::Vector2f dir;
    float r = 0;
    float speed = 0;

    bool operator==(const Ball& other) const{
        if (p == other.p && dir == other.dir && r == other.r && speed == other.speed)
            return true;
        return false;
    }

    float square() const {
      return r * r * M_PI;
    };
};



#endif // BALL_H
