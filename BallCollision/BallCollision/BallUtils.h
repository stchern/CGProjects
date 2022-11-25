#ifndef BALLUTILS_H
#define BALLUTILS_H

#include "Ball.h"

namespace BallUtils {

void resolveCollision(Ball& lhsBall, Ball& rhsBall, float deltaTime, int lIdx, int rIdx);

}

#endif // BALLUTILS_H
