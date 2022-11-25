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

    float dx = ball.velocity().x * deltaTime;
    float dy = ball.velocity().y * deltaTime;

    //correct code should be without multiptying or division by radius, but it looks strange

    if ( ball.p.x + 2.0f *  ball.r + dx  >= WINDOW_X ) {
        ball.speedX = -ball.speedX;
        dx = -ball.p.x + WINDOW_X - 2.0f * ball.r - 1;
    }

    if ( ball.p.y + 2.0f * ball.r + dy >= WINDOW_Y ) {
        ball.speedY = -ball.speedY;
        dy = -ball.p.y + WINDOW_Y - 2.0f * ball.r - 1;
    }

    if ( ball.p.x - ball.r / 4.0f + dx < 0 ) {
        ball.speedX = -ball.speedX;
        dx = -(ball.p.x - ball.r / 4.0f);
    }

    if ( ball.p.y - ball.r / 4.0f + dy < 0 ) {
        ball.speedY = -ball.speedY;
        dy = -(ball.p.y - ball.r / 4.0f );
    }

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

std::vector<sf::Vector2f> DrawingUtils::partitioning(size_t splitFrequency, int offset)
{
    std::vector<sf::Vector2f> endPositions;

    for (size_t y_part = 0; y_part < splitFrequency; ++y_part)
        for (size_t x_part = 0; x_part < splitFrequency; ++x_part) {
            sf::Vector2f firstSectionEnd(WINDOW_X * (x_part + 1) / splitFrequency - 1 - offset, WINDOW_Y * (y_part + 1) / splitFrequency - 1 - offset);
            endPositions.push_back(firstSectionEnd);
        }

    return endPositions;
}

