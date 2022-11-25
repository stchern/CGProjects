#ifndef BALL_H
#define BALL_H

#include "SFML/Graphics.hpp"
#include <math.h>

struct Ball
{
    sf::Vector2f p;
    sf::Vector2f dir;
    float r = 0;
//    float speed = 0.0f;
    float speedX = 0.0f;
    float speedY = 0.0f;

    bool operator==(const Ball& other) const{
        if (p == other.p && dir == other.dir && r == other.r && speedX == other.speedX && speedY == other.speedY)
            return true;
        return false;
    }

    float square() const {
      return r * r * M_PI;
    };

    sf::Vector2f velocity() const {
        return sf::Vector2f(speedX, speedY);
    }

    void setVelocity(const sf::Vector2f& newVelocity) {
        speedX = newVelocity.x;
        speedY = newVelocity.y;
    }
};



#endif // BALL_H
