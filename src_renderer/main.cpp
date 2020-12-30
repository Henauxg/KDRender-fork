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

    int playerDir = map.GetPlayerStartDirection();

    renderer.SetPlayerCoordinates(playerPos, playerDir);

    int dr = 10;
    int dtheta = 5;

    sf::RenderWindow app(sf::VideoMode(WINDOW_WIDTH, WINDOW_HEIGHT),
                         "KDTree Map Renderer",
                         sf::Style::Close);
    Screen screen(renderer);
    sf::Clock clock;

    float deltaTMovement = 0.f;
    while (app.isOpen())
    {
        sf::Event event;
        sf::Time elapsedTime = clock.getElapsedTime();

        screen.refresh();
        app.draw(screen);
        app.display();

        float deltaT = (float)(clock.getElapsedTime().asMilliseconds());
        deltaTMovement += deltaT;

        // std::cout << "FPS = " << 1000.f / deltaT << std::endl;
        clock.restart();

        playerPos = renderer.GetPlayerPosition();
        playerDir = renderer.GetPlayerDirection();

        int dPos = 0;
        int dDir = 0;

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
                    dPos = -dr;
                }
                else if (sf::Keyboard::isKeyPressed(sf::Keyboard::D))
                {
                    dPos = dr;
                }
                else if (sf::Keyboard::isKeyPressed(sf::Keyboard::O))
                {
                    dPos = dr;
                }
                else if (sf::Keyboard::isKeyPressed(sf::Keyboard::L))
                {
                    dPos = -dr;
                }
                else if (sf::Keyboard::isKeyPressed(sf::Keyboard::K))
                {
                    dDir = -dtheta;
                }
                else if (sf::Keyboard::isKeyPressed(sf::Keyboard::M))
                {
                    dDir = +dtheta;
                }
                else if (sf::Keyboard::isKeyPressed(sf::Keyboard::S))
                {
                    std::cout << "Dumping player info:" << std::endl;
                    std::cout << "x = " << playerPos.m_X << std::endl;
                    std::cout << "y = " << playerPos.m_Y << std::endl;
                    std::cout << "dir = " << playerDir << std::endl;
                }
                break;
            default:
                break;
            }
        }

        KDTreeRenderer::Vertex look(renderer.GetLook());
        int dx = ((100 * dPos * (look.m_X - playerPos.m_X)) / 100) / 100;
        int dy = ((100 * dPos * (look.m_Y - playerPos.m_Y)) / 100) / 100;
        playerPos.m_X += dx;
        playerPos.m_Y += dy;

        playerDir += dDir;
        while(playerDir < 0)
            playerDir += 360;
        while(playerDir > 360)
            playerDir -= 360;

        renderer.SetPlayerCoordinates(playerPos, playerDir);
        renderer.ClearBuffers();
        renderer.RefreshFrameBuffer();

        if(deltaTMovement > 50)
            deltaTMovement = 0;
    }

    return 0;
}