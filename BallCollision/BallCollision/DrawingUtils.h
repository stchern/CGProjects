#ifndef DRAWINGUTILS_H
#define DRAWINGUTILS_H

#include "Ball.h"

constexpr int WINDOW_X = 1024;
constexpr int WINDOW_Y = 768;
constexpr int MAX_BALLS = 300;
constexpr int MIN_BALLS = 100;


namespace DrawingUtils
{

void draw_ball(sf::RenderWindow& window, const Ball& ball);
void move_ball(Ball& ball, float deltaTime);
void draw_fps(sf::RenderWindow& window, float fps);


}

#endif // DRAWINGUTILS_H
