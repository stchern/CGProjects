#ifndef BALLUTILS_H
#define BALLUTILS_H

#include "Ball.h"

namespace BallUtils {

void resolveCollision(Ball& lhsBall, Ball& rhsBall);
void resolveCollisionForPatrition(const std::vector<sf::Vector2f>& partitioning, std::vector<Ball>& balls);

}

#endif // BALLUTILS_H
