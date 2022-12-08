#include "BallUtils.h"
#include "VectorUtils.h"


void BallUtils::resolveCollision(Ball& lhsBall, Ball& rhsBall)
{

    float summRad = lhsBall.r + rhsBall.r;
    sf::Vector2f normalVector = sf::Vector2f(lhsBall.p.x - rhsBall.p.x, lhsBall.p.y - rhsBall.p.y);
    float distance = VectorUtils::lengthVector2(normalVector);
    normalVector = normalVector / distance;

    if (distance < summRad) {
        // в основе вычислений лежат формулы из статьи https://www.vobarian.com/collisions/2dcollisions2.pdf

        float offset = (summRad - distance) / 2.0f;
        lhsBall.p += normalVector * offset;
        rhsBall.p += normalVector * (-1.0f) * offset;

        sf::Vector2f tangentVector = sf::Vector2f((-1.0f) * normalVector.y, normalVector.x);

        float v1n = VectorUtils::dot(normalVector, lhsBall.velocity());
        float v1t = VectorUtils::dot(tangentVector, lhsBall.velocity());
        float v2n = VectorUtils::dot(normalVector, rhsBall.velocity());
        float v2t = VectorUtils::dot(tangentVector, rhsBall.velocity());

        float massDiff = lhsBall.square() - rhsBall.square();
        float reverseMassSumm = 1.0f / (lhsBall.square() + rhsBall.square());
        float newV1n = (v1n * massDiff + 2.0f * rhsBall.square() * v2n) * reverseMassSumm;
        float newV2n = (v2n * (-1.0f) * massDiff + 2.0f * lhsBall.square() * v1n) * reverseMassSumm;

        sf::Vector2f newLhsVelocity = newV1n * normalVector + v1t * tangentVector;
        sf::Vector2f newRhsVelocity = newV2n * normalVector + v2t * tangentVector;

        lhsBall.setVelocity(newLhsVelocity);
        rhsBall.setVelocity(newRhsVelocity);

    }
}


void BallUtils::resolveCollisionForPatrition(const std::vector<sf::Vector2f>& partitioning, std::vector<Ball>& balls)
{

    // в отсортированном массиве шаров, мы находим крайний выходящий за текущую заданную partitioning область
    // и ищем коллизии только в ограниченной области
    auto startIter = balls.begin();
    for (size_t positionIdx = 0; positionIdx < partitioning.size(); ++positionIdx) {
        sf::Vector2f position = partitioning[positionIdx];

        auto findByPosition = [position](const Ball& ball) {
            return ball.p.x > position.x || ball.p.y > position.y;
        };

        auto endIter = std::find_if(startIter, balls.end(), findByPosition);
        for (auto currBallIt = startIter; currBallIt != endIter; ++currBallIt)
            for (auto nextBallIt = currBallIt + 1; nextBallIt != endIter; ++nextBallIt)
                BallUtils::resolveCollision(*currBallIt, *nextBallIt);

        startIter = endIter;
    }

}
