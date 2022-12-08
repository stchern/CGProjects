#include "Ball.h"
#include "BallUtils.h"
#include "DrawingUtils.h"
#include "MiddleAverageFilter.h"
#include <iostream>

Math::MiddleAverageFilter<float,100> fpscounter;


int main()
{
    sf::RenderWindow window(sf::VideoMode(WINDOW_X, WINDOW_Y), "ball collision demo");
    srand(time(NULL));


    int splitFrequency = 2;
    int offset = 10 + rand() % 10;
    std::vector<sf::Vector2f> partitioning = DrawingUtils::partitioning(splitFrequency);
    std::vector<sf::Vector2f> partitioningWithOffset = DrawingUtils::partitioning(splitFrequency, offset);
    std::vector<Ball> balls;

    // randomly initialize balls
    for (int i = 0; i < (rand() % (MAX_BALLS - MIN_BALLS) + MIN_BALLS); i++)
    {
        Ball newBall;
        newBall.p.x = rand() % WINDOW_X;
        newBall.p.y = rand() % WINDOW_Y;
        newBall.dir.x = (-5 + (rand() % 10)) / 3.;
        newBall.dir.y = (-5 + (rand() % 10)) / 3.;
        newBall.r = 15 + rand() % 10;
//        newBall.speed = 30 + rand() % 30;
        newBall.speedX = (30 + rand() % 30) * newBall.dir.x;
        newBall.speedY = (30 + rand() % 30) * newBall.dir.y;
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


        auto sortByPosition = [](const Ball& lhsBall, const Ball& rhsBall){
            if (lhsBall.p.x == rhsBall.p.x )
                return lhsBall.p.y < rhsBall.p.y;
            return lhsBall.p.x < rhsBall.p.x;
        };


        // добавление сортировки и разбиения окна, для ограничения области просчета столкновений для каждого мяча
        // разбиение позволяет уменьшить количество сравнений, исскать колизии для мяча только в заданной области, а не по всему окну
        // здесь используется два разбиения, одно из которых смещенно, для того, что бы по второму разу пройтись по сетке разбиения и заметить коллизии, которые могли
        // быть пропущены при работе с первым разбиением
        std::sort(balls.begin(), balls.end(), sortByPosition);
        BallUtils::resolveCollisionForPatrition(partitioning, balls);
        BallUtils::resolveCollisionForPatrition(partitioningWithOffset, balls);

    // вычисления коллизий без использования сортировки и разбиения
//        float begin_time = clock.getElapsedTime().asSeconds();
//        for (auto currBallIt = 0; currBallIt < balls.size(); ++currBallIt)
//            for (auto nextBallIt = currBallIt + 1; nextBallIt < balls.size(); ++nextBallIt)
//                BallUtils::resolveCollision(balls[currBallIt], balls[nextBallIt]);



        for (auto& ball : balls)
        {
            DrawingUtils::move_ball(ball, deltaTime);
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
