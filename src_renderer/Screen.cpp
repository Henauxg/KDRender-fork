#include "Screen.h"

#include "Consts.h"
#include "KDTreeRenderer.h"

#include <iostream>

Screen::Screen(const KDTreeRenderer &iRenderer) : 
    m_Renderer(iRenderer)
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
    const unsigned char *framebuffer = m_Renderer.GetFrameBuffer();

    if (framebuffer)
    {
        _screenContent.create(WINDOW_WIDTH, WINDOW_HEIGHT, framebuffer);
    }

    _text.loadFromImage(_screenContent);
    _sprite.setTexture(_text, true);
}