#ifndef DRAWINGUTILS_H
#define DRAWINGUTILS_H

#include "Ball.h"

constexpr int WINDOW_X = 1024;
constexpr int WINDOW_Y = 768;
constexpr int MAX_BALLS = 100;
constexpr int MIN_BALLS = 30;


namespace DrawingUtils
{

void draw_ball(sf::RenderWindow& window, const Ball& ball);
void move_ball(Ball& ball, float deltaTime);
void draw_fps(sf::RenderWindow& window, float fps);
std::vector<sf::Vector2f> partitioning(size_t splitFrequency, int offset = 0);


}

#endif // DRAWINGUTILS_H
