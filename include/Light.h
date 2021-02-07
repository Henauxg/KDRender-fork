#ifndef Light_h
#define Light_h

#include "Consts.h"

#ifdef __EXPERIMENGINE__
#include <engine/utils/Timer.hpp>
#else
#include <SFML/System.hpp>
#endif

// Abstract light class
class Light
{
public:
	enum class Type
	{
		CONSTANT,
		FLICKERING
	};

public:
	Type GetType() const;

public:
	Light(Type iType);
	virtual ~Light() {}

public:
	virtual unsigned int ComputeStreamSize() const;
	virtual void Stream(char*& ioData, unsigned int& oNbBytesWritten) const;
	virtual void UnStream(const char* ipData, unsigned int& oNbBytesRead);

public:
	virtual unsigned int GetValue() const = 0;

protected:
	Type m_Type;
};

// Quite ugly workaround :/
Light* UnstreamLight(const char* ipData, unsigned int& oNbBytesRead);

namespace LightTools
{
	CType GetMaxInterpolationDist(unsigned int iLightValue);
	unsigned int GetMinLight(unsigned int iLightValue);
} // namespace LightTools

// Constant light
// Implements Light
class ConstantLight : public Light
{
public:
	ConstantLight(unsigned int iValue);
	virtual ~ConstantLight();

public:
	virtual unsigned int ComputeStreamSize() const override;
	virtual void Stream(char*& ioData, unsigned int& oNbBytesWritten) const override;
	virtual void UnStream(const char* ipData, unsigned int& oNbBytesRead) override;

public:
	virtual unsigned int GetValue() const override;

protected:
	unsigned int m_Value;
};

// FlickeringLight
// Implement
class FlickeringLight : public Light
{
public:
	FlickeringLight(unsigned int iLow, unsigned int iHigh);
	virtual ~FlickeringLight();

public:
	virtual unsigned int ComputeStreamSize() const override;
	virtual void Stream(char*& ioData, unsigned int& oNbBytesWritten) const override;
	virtual void UnStream(const char* ipData, unsigned int& oNbBytesRead) override;

public:
	virtual unsigned int GetValue() const override;

protected:
	unsigned int m_Low;
	unsigned int m_High;

#ifdef __EXPERIMENGINE__
	experim::Timer m_Clock;
#else
	sf::Clock m_Clock;
#endif
};

#endif