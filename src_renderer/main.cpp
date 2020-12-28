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
    renderer.SetPlayerPosition(map.GetPlayerStartX(), map.GetPlayerStartY(), map.GetPlayerStartDirection());

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

        std::cout << "FPS = " << 1000.f / (float)(clock.getElapsedTime().asMilliseconds()) << std::endl;
        clock.restart();

        renderer.ClearBuffers();
        renderer.RefreshFrameBuffer();

        while (app.pollEvent(event))
        {
            switch (event.type)
            {
            case sf::Event::Closed:
                app.close();
                break;
            case sf::Event::KeyPressed:
                if (sf::Keyboard::isKeyPressed(sf::Keyboard::Up))
                {  

                }
                else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Down))
                {
                    
                }
                else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Q))
                {
                    
                }
                else if (sf::Keyboard::isKeyPressed(sf::Keyboard::D))
                {
                    
                }
                else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Z))
                {
                    
                }
                else if (sf::Keyboard::isKeyPressed(sf::Keyboard::S))
                {
                    
                }
                break;
            default:
                break;
            }
        }
    }

    return 0;
}