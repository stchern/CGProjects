#include "Ball.h"
#include "BallUtils.h"
#include "DrawingUtils.h"
#include "MiddleAverageFilter.h"

Math::MiddleAverageFilter<float,100> fpscounter;


int main()
{
    sf::RenderWindow window(sf::VideoMode(WINDOW_X, WINDOW_Y), "ball collision demo");
    srand(time(NULL));

    std::vector<Ball> balls;

    // randomly initialize balls
    for (int i = 0; i < (rand() % (MAX_BALLS - MIN_BALLS) + MIN_BALLS); i++)
    {
        Ball newBall;
        newBall.p.x = rand() % WINDOW_X;
        newBall.p.y = rand() % WINDOW_Y;
        newBall.dir.x = (-5 + (rand() % 10)) / 3.;
        newBall.dir.y = (-5 + (rand() % 10)) / 3.;
        newBall.r = 5 + rand() % 5;
        newBall.speed = 30 + rand() % 30;
        balls.push_back(newBall);
    }

   // window.setFramerateLimit(60);

    sf::Clock clock;
    float lastime = clock.restart().asSeconds();

    while (window.isOpen())
    {

        sf::Event event;
        while (window.pollEvent(event))
        {
            if (event.type == sf::Event::Closed)
            {
                window.close();
            }
        }

        float current_time = clock.getElapsedTime().asSeconds();
        float deltaTime = current_time - lastime;
        fpscounter.push(1.0f / (current_time - lastime));
        lastime = current_time;

        /// <summary>
        /// TODO: PLACE COLLISION CODE HERE 
        /// объекты создаются в случайном месте на плоскости со случайным вектором скорости, имеют радиус R
        /// Объекты движутся кинетически. Пространство ограниченно границами окна
        /// Напишите обработчик столкновений шаров между собой и краями окна. Как это сделать эффективно?
        /// Массы пропорцианальны площадям кругов, описывающих объекты 
        /// Как можно было-бы улучшить текущую архитектуру кода?
        /// Данный код является макетом, вы можете его модифицировать по своему усмотрению

//        for (auto& ball : balls)
//        {
//            DrawingUtils::move_ball(ball, deltaTime);
//        }

        for (size_t currBallIdx = 0; currBallIdx < balls.size(); ++currBallIdx) {
            for (size_t collBallIdx = currBallIdx + 1; collBallIdx < balls.size(); ++collBallIdx) {
                if (BallUtils::isCollided(balls[currBallIdx], balls[collBallIdx], deltaTime)) {
                }
            }
            DrawingUtils::move_ball(balls[currBallIdx], deltaTime);
        }

        window.clear();
        for (const auto ball : balls)
        {
            DrawingUtils::draw_ball(window, ball);
        }

		//draw_fps(window, fpscounter.getAverage());
		window.display();
    }
    return 0;
}
