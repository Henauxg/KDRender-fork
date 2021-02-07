#include "Screen.h"

#include "Consts.h"
#include "KDTreeMap.h"
#include "KDTreeRenderer.h"
#include "FP32.h"


#ifdef __EXPERIMENGINE__
#include <engine/utils/Timer.hpp>
#include <engine/utils/Utils.hpp>
#include <engine/Engine.hpp>
#else
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Graphics/Drawable.hpp>
#include <SFML/Graphics/Image.hpp>
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/Texture.hpp>
#include <SFML/Window/Event.hpp>
#include <SFML/System.hpp>
#endif

#include <iostream>
#include <fstream>
#include <thread>

namespace {
	const std::string APPLICATION_NAME = "KDTree Map Renderer";
#ifdef __EXPERIMENGINE__
	const uint32_t APPLICATION_VERSION = EXPENGINE_MAKE_VERSION(0, 0, 1);
#endif
}

class MapRenderer {

public:
	MapRenderer() = default;
	int run(int argc, char** argv);
	void tick(float deltaT);

private:
	KDTreeMap m_Map;
	std::unique_ptr<KDTreeRenderer> m_Renderer = nullptr;
	std::unique_ptr<Screen> m_Screen = nullptr;
	KDRData::Vertex m_PlayerPos;
	int m_playerDir = 0;
	int64_t m_FrameCount = 0;

#ifdef __EXPERIMENGINE__
	std::unique_ptr<experim::Engine> m_Engine;
#else
	sf::Clock m_FpsClock;
	sf::Clock m_Clock;
#endif

};

int main(int argc, char** argv)
{
	MapRenderer mapRenderer;
	return mapRenderer.run(argc, argv);
}

int MapRenderer::run(int argc, char** argv)
{
	if (argc != 3)
	{
		std::cout << "Usage : maprenderer -i path/to/map.kdm" << std::endl;
		return EXIT_FAILURE;
	}
	else
	{
		char* pData = nullptr;
		std::ifstream mapStream(argv[2], std::ios::binary | std::ios::in);
		if (!mapStream.is_open())
		{
			std::cout << "Error: could not open input map" << std::endl;
		}

		mapStream.seekg(0, mapStream.end);
		int length = mapStream.tellg();
		mapStream.seekg(0, mapStream.beg);
		pData = new char[length];
		mapStream.read(pData, length);

		if (pData)
		{
			unsigned int dummy;
			m_Map.UnStream(pData, dummy);

			delete pData;
			pData = nullptr;
		}
	}

	m_Renderer = std::make_unique<KDTreeRenderer>(m_Map);
	m_Screen = std::make_unique<Screen>(*m_Renderer);

	m_PlayerPos.m_X = m_Map.GetPlayerStartX();
	m_PlayerPos.m_Y = m_Map.GetPlayerStartY();
	m_playerDir = m_Map.GetPlayerStartDirection();

	m_Renderer->SetPlayerCoordinates(m_PlayerPos, m_playerDir);

#ifdef __EXPERIMENGINE__
	m_Engine = std::make_unique<experim::Engine>(APPLICATION_NAME, APPLICATION_VERSION);
	m_Engine->onTick(this);
	experim::Timer totalClock;

	// Main loop
	m_Engine->run();

	double totalElapsedMs = totalClock.getElapsedTime<experim::Milliseconds>();
#else
	sf::RenderWindow app(sf::VideoMode(WINDOW_WIDTH, WINDOW_HEIGHT),
		APPLICATION_NAME,
		sf::Style::Close);

	sf::Clock totalClock;
	totalClock.restart();

	// Main loop
	while (app.isOpen())
	{
		tick();
	}

	double totalElapsedMs = totalClock.getElapsedTime().asMilliseconds();
#endif

	// Shitty average, should be weighted by time and not framecount
	double averageFps = static_cast<double>(m_FrameCount) / totalElapsedMs * 1000.0;
	std::cout << "Average FPS = " << averageFps << std::endl;

	return EXIT_SUCCESS;
}

void MapRenderer::tick(float deltaT)
{
	static unsigned int showFPS = 0;
	static const CType dr(180.f / POSITION_SCALE);
	static const int dtheta = 140 << ANGLE_SHIFT;

	int directionFront = 0;
	int directionBack = 0;
	int directionStrafeLeft = 0;
	int directionStrafeRight = 0;
	int directionLeft = 0;
	int directionRight = 0;
	int slowDown = 0;
	unsigned int poolEvent = 0;

	sf::Event event;

	m_Screen->refresh();
	app.draw(m_Screen);
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

	// if(poolEvent++ % 20 == 0)
	{
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
					std::cout << "x = " << m_PlayerPos.m_X * POSITION_SCALE << std::endl;
					std::cout << "y = " << m_PlayerPos.m_Y * POSITION_SCALE << std::endl;
					std::cout << "dir = " << m_playerDir << " (= " << static_cast<float>(m_playerDir) / (1 << ANGLE_SHIFT) << " degres)" << std::endl;
				}
				break;
			default:
				break;
			}
		}
	}

#ifndef __EXPERIMENGINE__
	float deltaT = (float)(m_Clock.getElapsedTime().asMilliseconds());
	m_Clock.restart();
#endif

#if 0
	// 60 FPS cap
	if (deltaT < (1000.f / 60.f))
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(static_cast<int>(1000.f / 60.f - deltaT)));
		deltaT += (float)(m_Clock.getElapsedTime().asMilliseconds());
	}
#endif

#ifndef __EXPERIMENGINE__
	// if (1000.f / deltaT < 60.f)
	const int fpsStep = 100;
	if (showFPS++ % fpsStep == 0)
	{
		float deltaTFps = (float)(m_FpsClock.getElapsedTime().asMilliseconds());
		std::cout << "FPS = " << (1000.f * static_cast<float>(fpsStep)) / deltaTFps << std::endl;
		m_FpsClock.restart();
	}
#endif

	CType dPos = ((directionFront + directionBack) * dr * static_cast<CType>(deltaT)) / 1000;
	CType dPosOrtho = ((directionStrafeLeft + directionStrafeRight) * dr * static_cast<CType>(deltaT)) / 1000;
	CType dDir = ((directionLeft + directionRight) * dtheta * (deltaT)) / 1000;

	dPos = dPos / (slowDown + 1);
	dDir = dDir / (slowDown + 1);

	KDRData::Vertex look(m_Renderer->GetLook());

	CType dx = dPos * (look.m_X - m_PlayerPos.m_X);
	CType dy = dPos * (look.m_Y - m_PlayerPos.m_Y);

	m_PlayerPos.m_X += dx;
	m_PlayerPos.m_Y += dy;

	m_playerDir = m_playerDir + dDir;
	while (m_playerDir < 0)
		m_playerDir += 360 << ANGLE_SHIFT;
	while (m_playerDir > (360 << ANGLE_SHIFT))
		m_playerDir -= 360 << ANGLE_SHIFT;

	m_Renderer->SetPlayerCoordinates(m_PlayerPos, m_playerDir);

	m_Renderer->ClearBuffers();
	m_Renderer->RefreshFrameBuffer();

	m_FrameCount++;
}
