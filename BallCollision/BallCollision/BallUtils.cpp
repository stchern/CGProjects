#include "BallUtils.h"
#include "VectorUtils.h"

//using namespace BallUtils;


bool BallUtils::isCollided(Ball& lhsBall, Ball& rhsBall, float deltaTime)
{
    if (lhsBall == rhsBall)
        return false;

      sf::Vector2f collisionVector = lhsBall.p - rhsBall.p;
      float distance = VectorUtils::lengthVector2(collisionVector);

        if (distance == 0.0) {
            collisionVector = sf::Vector2f(1.0, 0.0);
            distance = 1.0;
        }
//        if (distance > 1.0)
//            return false;

        // Get the components of the velocity vectors which are parallel to the collision.
        // The perpendicular component remains the same for both fish

//        collisionVector = collisionVector / distance;
//        float aci = VectorUtils::dot(lhsBall.dir * lhsBall.speed * deltaTime, collisionVector);
//        float bci = VectorUtils::dot(rhsBall.dir * rhsBall.speed * deltaTime, collisionVector);

//        // Solve for the new velocities using the 1-dimensional elastic collision equations.
//        // Turns out it's really simple when the masses are the same.
//        float acf = bci;
//        float bcf = aci;

        sf::Vector2f velocityVector = lhsBall.dir * lhsBall.speed - rhsBall.dir * rhsBall.speed;
        float aci = VectorUtils::dot(velocityVector, collisionVector);
        float bci = VectorUtils::dot(-velocityVector, -collisionVector);

        // Replace the collision velocity components with the new ones

//        sf::Vector2f newLhsVelocity = collisionVector * (acf - aci);
//        sf::Vector2f newRhsVelocity = collisionVector * (bcf - bci);
        sf::Vector2f newLhsVelocity = collisionVector * aci / distance;
        sf::Vector2f newRhsVelocity = -collisionVector * bci / distance;

        lhsBall.dir.x = (newLhsVelocity.x >= 0)? lhsBall.dir.x : -lhsBall.dir.x;
        lhsBall.dir.y = (newLhsVelocity.y >= 0)? lhsBall.dir.y : -lhsBall.dir.y;
        rhsBall.dir.x = (newRhsVelocity.x >= 0)? rhsBall.dir.x : -rhsBall.dir.x;
        rhsBall.dir.y = (newRhsVelocity.y >= 0)? rhsBall.dir.y : -rhsBall.dir.y;

        lhsBall.speed -= VectorUtils::lengthVector2(newLhsVelocity) * 2 * rhsBall.square() / rhsBall.square() + lhsBall.square();
        rhsBall.speed -= VectorUtils::lengthVector2(newRhsVelocity) * 2 * lhsBall.square() / rhsBall.square() + lhsBall.square();

        return true;
}
