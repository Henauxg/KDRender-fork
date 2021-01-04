#include "Screen.h"

#include "Consts.h"
#include "KDTreeMap.h"
#include "KDTreeRenderer.h"

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
    
    KDTreeRenderer::Vertex playerPos;
    playerPos.m_X = map.GetPlayerStartX();
    playerPos.m_Y = map.GetPlayerStartY();
    unsigned posPlayerShift = 3u;
    playerPos.LShift(posPlayerShift);

    int playerDir = map.GetPlayerStartDirection();

    renderer.SetPlayerCoordinates(playerPos.RShift(posPlayerShift), playerDir);

    int dr = 3600 << posPlayerShift;
    int dtheta = 180 << ANGLE_SHIFT;

    sf::RenderWindow app(sf::VideoMode(WINDOW_WIDTH, WINDOW_HEIGHT),
                         "KDTree Map Renderer",
                         sf::Style::Close);
    Screen screen(renderer);
    sf::Clock clock;

    while (app.isOpen())
    {
        sf::Event event;
        sf::Time elapsedTime = clock.getElapsedTime();

        screen.refresh();
        app.draw(screen);
        app.display();

        float deltaT = (float)(clock.getElapsedTime().asMilliseconds());
        int dPos = 0;
        int dDir = 0;

        // 60 FPS cap
        if(deltaT < (1000.f/60.f))
            std::this_thread::sleep_for(std::chrono::milliseconds(static_cast<int>(1000.f / 60.f - deltaT)));

        deltaT = (float)(clock.getElapsedTime().asMilliseconds());
        // std::cout << "FPS = " << 1000.f / deltaT << std::endl;

        clock.restart();

        while (app.pollEvent(event))
        {
            switch (event.type)
            {
            case sf::Event::Closed:
                app.close();
                break;
            case sf::Event::KeyPressed:
                if (sf::Keyboard::isKeyPressed(sf::Keyboard::Q))
                {
                    dPos -= dr;
                }
                else if (sf::Keyboard::isKeyPressed(sf::Keyboard::D))
                {
                    dPos += dr;
                }
                else if (sf::Keyboard::isKeyPressed(sf::Keyboard::O))
                {
                    dPos += dr;
                }
                else if (sf::Keyboard::isKeyPressed(sf::Keyboard::L))
                {
                    dPos -= dr;
                }
                else if (sf::Keyboard::isKeyPressed(sf::Keyboard::K))
                {
                    dDir -= dtheta;
                }
                else if (sf::Keyboard::isKeyPressed(sf::Keyboard::M))
                {
                    dDir += dtheta;
                }
                else if (sf::Keyboard::isKeyPressed(sf::Keyboard::S))
                {
                    std::cout << "Dumping player info:" << std::endl;
                    std::cout << "x = " << (playerPos.m_X >> posPlayerShift) << std::endl;
                    std::cout << "y = " << (playerPos.m_Y >> posPlayerShift) << std::endl;
                    std::cout << "dir = " << playerDir << std::endl;
                }
                break;
            default:
                break;
            }
        }

        dPos = (dPos * static_cast<int>(deltaT)) / 1000;
        dDir = (dDir * static_cast<int>(deltaT)) / 1000;

        // if(dPos > 0)
        //     std::cout << "dPos = " << dPos << std::endl;

        KDTreeRenderer::Vertex look(renderer.GetLook());
        int dx = ARITHMETIC_SHIFT(((dPos * (look.m_X - (playerPos.m_X >> posPlayerShift)))), DECIMAL_SHIFT);
        int dy = ARITHMETIC_SHIFT((dPos * (look.m_Y - (playerPos.m_Y >> posPlayerShift))), DECIMAL_SHIFT);
        playerPos.m_X += dx;
        playerPos.m_Y += dy;

        playerDir += dDir;
        while (playerDir < 0)
            playerDir += 360 << ANGLE_SHIFT;
        while (playerDir > (360 << ANGLE_SHIFT))
            playerDir -= 360 << ANGLE_SHIFT;

        renderer.SetPlayerCoordinates(playerPos.RShift(posPlayerShift), playerDir);

        renderer.ClearBuffers();
        renderer.RefreshFrameBuffer();
    }

    return 0;
}