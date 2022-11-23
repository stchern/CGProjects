#ifndef BALLUTILS_H
#define BALLUTILS_H

#include "Ball.h"

namespace BallUtils {

bool isCollided(Ball& lhsBall, Ball& rhsBall, float deltaTime);

}

#endif // BALLUTILS_H
