#include "DrawingUtils.h"

void DrawingUtils::draw_ball(sf::RenderWindow& window, const Ball& ball)
{
    sf::CircleShape gball;
    gball.setRadius(ball.r);
    gball.setPosition(ball.p.x, ball.p.y);
    window.draw(gball);
}

void DrawingUtils::move_ball(Ball& ball, float deltaTime)
{

    ball.speedX = (ball.p.x + ball.r < WINDOW_X - 1 && (ball.p.x - ball.r) )? ball.speedX : -ball.speedX;
    ball.speedY = (ball.p.y + ball.r < WINDOW_Y - 1 && (ball.p.y - ball.r) )? ball.speedY : -ball.speedY;

//    ball.dir.x = (ball.p.x + ball.r < WINDOW_X - 1 && (ball.p.x - ball.r) )? ball.dir.x : -ball.dir.x;
//    ball.dir.y = (ball.p.y + ball.r < WINDOW_Y - 1 && (ball.p.y - ball.r) )? ball.dir.y : -ball.dir.y;

//    float dx = ball.dir.x * ball.speedX * deltaTime;
//    float dy = ball.dir.y * ball.speedY * deltaTime;

    float dx = ball.velocity().x * deltaTime;
    float dy = ball.velocity().y * deltaTime;

//    float dx = ball.velocity().x;
//    float dy = ball.velocity().y;

//    dx = (ball.p.x + ball.r + dx < WINDOW_X)? dx : WINDOW_X - ball.p.x - ball.r - 1;
//    dy = (ball.p.y + ball.r + dy < WINDOW_Y)? dy : WINDOW_Y - ball.p.y - ball.r - 1;

//    dx = (ball.p.x + ball.r + dx < WINDOW_X)? dx : WINDOW_X - ball.p.x - ball.r - 1;
//    dy = (ball.p.y + ball.r + dy < WINDOW_Y)? dy : WINDOW_Y - ball.p.y - ball.r - 1;

//    dx = (ball.p.x - ball.r + dx >= 0)? dx : -(ball.p.x - ball.r);
//    dy = (ball.p.y - ball.r + dy >= 0)? dy : -(ball.p.y - ball.r);

//    dx = (ball.p.x + ball.r + dx < WINDOW_X)? dx : WINDOW_X - ball.p.x - ball.r - 1;
//    dy = (ball.p.y + ball.r + dy < WINDOW_Y)? dy : WINDOW_Y - ball.p.y - ball.r - 1;


    dx = (ball.p.x + ball.r + dx < WINDOW_X)? dx : WINDOW_X - ball.p.x - ball.r - 1;
    dy = (ball.p.y + ball.r + dy < WINDOW_Y)? dy : WINDOW_Y - ball.p.y - ball.r - 1;

    dx = (ball.p.x - ball.r + dx >= 0)? dx : -(ball.p.x - ball.r);
    dy = (ball.p.y - ball.r + dy >= 0)? dy : -(ball.p.y - ball.r);


    ball.p.x += dx;
    ball.p.y += dy;
}

void DrawingUtils::draw_fps(sf::RenderWindow& window, float fps)
{
    char c[32];
    snprintf(c, 32, "FPS: %f", fps);
    std::string string(c);
    sf::String str(c);
    window.setTitle(str);
}



