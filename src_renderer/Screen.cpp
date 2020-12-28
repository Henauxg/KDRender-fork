#include "Screen.h"

#include "Consts.h"

#include <iostream>

Screen::Screen()
{
    _text.loadFromImage(_screenContent);
    _sprite.setTexture(_text, true);
}

void Screen::draw(sf::RenderTarget &target, sf::RenderStates states) const
{
    target.draw(_sprite, states);
}

void Screen::refresh()
{
    const unsigned char *framebuffer = nullptr;

    if (framebuffer)
    {
        _screenContent.create(WINDOW_WIDTH, WINDOW_HEIGHT, framebuffer);
    }

    _text.loadFromImage(_screenContent);
    _sprite.setTexture(_text, true);
}