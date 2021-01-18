#include "Screen.h"

#include "Consts.h"
#include "KDTreeMap.h"
#include "KDTreeRenderer.h"
#include "FP32.h"

#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Graphics/Drawable.hpp>
#include <SFML/Graphics/Image.hpp>
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/Texture.hpp>
#include <SFML/Window/Event.hpp>
#include <SFML/System.hpp>

#include <iostream>
#include <fstream>
#include <thread>

int main(int argc, char **argv)
{
    KDTreeMap map;
    if (argc != 3)
    {
        std::cout << "Usage : maprenderer -i path/to/map.kdm" << std::endl;
        return 1;
    }
    else
    {
        char *pData = nullptr;
        std::ifstream mapStream(argv[2], std::ios::binary | std::ios::in);
        if(!mapStream.is_open())
        {
            std::cout << "Error: could not open input map" << std::endl;
        }

        mapStream.seekg(0, mapStream.end);
        int length = mapStream.tellg();
        mapStream.seekg(0, mapStream.beg);
        pData = new char[length];
        mapStream.read(pData, length);

        if(pData)
        {
            unsigned int dummy;
            map.UnStream(pData, dummy);

            delete pData;
            pData = nullptr;
        }
    }

    KDTreeRenderer renderer(map);

    KDRData::Vertex playerPos;
    playerPos.m_X = map.GetPlayerStartX();
    playerPos.m_Y = map.GetPlayerStartY();

    int playerDir = map.GetPlayerStartDirection();

    renderer.SetPlayerCoordinates(playerPos, playerDir);

    CType dr(180.f / POSITION_SCALE);
    int dtheta = 140 << ANGLE_SHIFT;

    sf::RenderWindow app(sf::VideoMode(WINDOW_WIDTH, WINDOW_HEIGHT),
                         "KDTree Map Renderer",
                         sf::Style::Close);

    Screen screen(renderer);
    sf::Clock clock;

    int directionFront = 0;
    int directionBack = 0;
    int directionStrafeLeft = 0;
    int directionStrafeRight = 0;
    int directionLeft = 0;
    int directionRight = 0;
    int slowDown = 0;

    unsigned int poolEvent = 0;

    while (app.isOpen())
    {
        sf::Event event;

        screen.refresh();
        app.draw(screen);
        app.display();

        if (sf::Keyboard::isKeyPressed(sf::Keyboard::O))
        {
            directionFront = 1;
        }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::L))
        {
            directionBack = -1;
        }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::K))
        {
            directionRight = -1;
        }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::M))
        {
            directionLeft = 1;
        }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Q))
        {
            directionStrafeRight = -1;
        }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::D))
        {
            directionStrafeLeft = 1;
        }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::LShift))
        {
            slowDown = 8;
        }

        if (!sf::Keyboard::isKeyPressed(sf::Keyboard::O))
        {
            directionFront = 0;
        }
        if (!sf::Keyboard::isKeyPressed(sf::Keyboard::L))
        {
            directionBack = 0;
        }
        if (!sf::Keyboard::isKeyPressed(sf::Keyboard::K))
        {
            directionRight = 0;
        }
        if (!sf::Keyboard::isKeyPressed(sf::Keyboard::M))
        {
            directionLeft = 0;
        }
        if (!sf::Keyboard::isKeyPressed(sf::Keyboard::Q))
        {
            directionStrafeRight = 0;
        }
        if (!sf::Keyboard::isKeyPressed(sf::Keyboard::D))
        {
            directionStrafeLeft = 0;
        }
        if (!sf::Keyboard::isKeyPressed(sf::Keyboard::LShift))
        {
            slowDown = 0;
        }

        if(poolEvent++ % 20 == 0)
        {
            poolEvent = 0;
            while (app.pollEvent(event))
            {
                switch (event.type)
                {
                case sf::Event::Closed:
                    app.close();
                    break;
                case sf::Event::KeyPressed:
                    if (sf::Keyboard::isKeyPressed(sf::Keyboard::S))
                    {
                        std::cout << "Dumping player info:" << std::endl;
                        std::cout << "x = " << playerPos.m_X * POSITION_SCALE << std::endl;
                        std::cout << "y = " << playerPos.m_Y * POSITION_SCALE << std::endl;
                        std::cout << "dir = " << playerDir << " (= " << static_cast<float>(playerDir) / (1 << ANGLE_SHIFT) << " degres)" << std::endl;
                    }
                    break;
                default:
                    break;
                }
            }
        }

        float deltaT = (float)(clock.getElapsedTime().asMilliseconds());
        clock.restart();

        // 60 FPS cap
        // if (deltaT < (1000.f / 60.f))
        // {
        //     std::this_thread::sleep_for(std::chrono::milliseconds(static_cast<int>(1000.f / 60.f - deltaT)));
        //     deltaT += (float)(clock.getElapsedTime().asMilliseconds());
        // }

        if (1000.f / deltaT < 60.f)
            std::cout << "FPS = " << 1000.f / deltaT << std::endl;

        CType dPos = ((directionFront + directionBack) * dr * static_cast<CType>(deltaT)) / 1000;
        CType dPosOrtho = ((directionStrafeLeft + directionStrafeRight) * dr * static_cast<CType>(deltaT)) / 1000;
        CType dDir = ((directionLeft + directionRight) * dtheta * (deltaT)) / 1000;

        dPos = dPos / (slowDown + 1);
        dDir = dDir / (slowDown + 1);

        // if(dPos > 0)
        //     std::cout << "dPos = " << dPos << std::endl;

        KDRData::Vertex look(renderer.GetLook());

        CType dx = dPos * (look.m_X - playerPos.m_X);
        CType dy = dPos * (look.m_Y - playerPos.m_Y);

        playerPos.m_X += dx;
        playerPos.m_Y += dy;

        playerDir = playerDir + dDir;
        while (playerDir < 0)
            playerDir += 360 << ANGLE_SHIFT;
        while (playerDir > (360 << ANGLE_SHIFT))
            playerDir -= 360 << ANGLE_SHIFT;

        renderer.SetPlayerCoordinates(playerPos, playerDir);

        renderer.ClearBuffers();
        renderer.RefreshFrameBuffer();
    }

    return 0;
}