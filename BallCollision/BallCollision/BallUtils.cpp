#include "BallUtils.h"
#include "VectorUtils.h"
#include "iostream"
#include <math.h>



void BallUtils::resolveCollision(Ball& lhsBall, Ball& rhsBall, float deltaTime, int lIdx, int rIdx)
{

//    std::cout<< "lhs ball: " << lIdx << " position: " << lhsBall.p.x << "  " << lhsBall.p.y << " radius: " << lhsBall.r <<
//                " dir: " << lhsBall.dir.x << " " << lhsBall.dir.x << " speed: " << lhsBall.speedX << " " << lhsBall.speedY << std::endl;

//    std::cout<< "rhs ball: "<< rIdx << " position: " << rhsBall.p.x << "  " << rhsBall.p.y << " radius: " << rhsBall.r <<
//                " dir: " << rhsBall.dir.x << " " << rhsBall.dir.x << " speed: " << rhsBall.speedX << " " << rhsBall.speedY << std::endl;
    if (lhsBall == rhsBall)
        return;

    float summRad = lhsBall.r + rhsBall.r;
//    if (std::abs(rhsBall.p.x - lhsBall.p.x) <= summRad &&  std::abs(rhsBall.p.y - lhsBall.p.y) <= summRad) {

        sf::Vector2f normalVector = sf::Vector2f(lhsBall.p.x - rhsBall.p.x, lhsBall.p.y - rhsBall.p.y);
        float distance = VectorUtils::lengthVector2(normalVector);
        normalVector = normalVector / distance;

    if (distance < summRad) {

        float dr = (summRad - distance) / 2.0f;
        lhsBall.p += normalVector * dr;
        rhsBall.p += normalVector * (-1.0f) * dr;

        sf::Vector2f tangentVector = sf::Vector2f((-1.f) * normalVector.y, normalVector.x);

        float v1n = VectorUtils::dot(normalVector, lhsBall.velocity());
        float v1t = VectorUtils::dot(tangentVector, lhsBall.velocity());
        float v2n = VectorUtils::dot(normalVector, rhsBall.velocity());
        float v2t = VectorUtils::dot(tangentVector, rhsBall.velocity());

        float massDiff = lhsBall.square() - rhsBall.square();
        float reverseMassSumm = 1.0f / (lhsBall.square() + rhsBall.square());
        float newV1n = (v1n * massDiff + 2.0f * rhsBall.square() * v2n) * reverseMassSumm;
        float newV2n = (v2n * (-1.0f) * massDiff + 2.0f * lhsBall.square() * v1n) * reverseMassSumm;

//        sf::Vector2f newLhsVelocity = v1n * normalVector + v1t * tangentVector;
//        sf::Vector2f newRhsVelocity = v2n * normalVector + v2t * tangentVector;
        sf::Vector2f newLhsVelocity = newV1n * normalVector + v1t * tangentVector;
        sf::Vector2f newRhsVelocity = newV2n * normalVector + v2t * tangentVector;

        lhsBall.setVelocity(newLhsVelocity);
        rhsBall.setVelocity(newRhsVelocity);


//        std::cout<< "lhs ball: " << lIdx << " dir: " << lhsBall.dir.x << " " << lhsBall.dir.x << " speed: " << lhsBall.speedX << " " << lhsBall.speedY << std::endl;
//        std::cout<< "rhs ball: " << rIdx << " dir: " << rhsBall.dir.x << " " << rhsBall.dir.x << " speed: " <<  rhsBall.speedX << " " << rhsBall.speedY<< std::endl;

    }
}

