#ifndef Screen_h
#define Screen_h

#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Graphics/Drawable.hpp>
#include <SFML/Graphics/Image.hpp>
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/Texture.hpp>
#include <SFML/Window/Event.hpp>
#include <SFML/System.hpp>

class KDTreeRenderer;

class Screen : public sf::Drawable
{
public:
    Screen(const KDTreeRenderer &iRenderer);

    void refresh();

private:
    void draw(sf::RenderTarget &target, sf::RenderStates states) const;

private:
    sf::Image _screenContent;
    sf::Texture _text;
    sf::Sprite _sprite;

private:
    const KDTreeRenderer &m_Renderer;
};

#endif