#include "Light.h"

#include "GeomUtils.h"

// ****** Light *******

Light::Light(Type iType):
    m_Type(iType)
{

}

Light::Type Light::GetType() const
{
    return m_Type;
}

unsigned int Light::ComputeStreamSize() const
{
    return sizeof(Light::Type);
}

void Light::Stream(char *&ioData, unsigned int &oNbBytesWritten) const
{
    oNbBytesWritten = Light::ComputeStreamSize();
    *(reinterpret_cast<Light::Type *>(ioData)) = m_Type;
    ioData += oNbBytesWritten;
}

void Light::UnStream(const char *ipData, unsigned int &oNbBytesRead)
{
    oNbBytesRead = Light::ComputeStreamSize();
    m_Type = *reinterpret_cast<const Light::Type *>(ipData);
}

Light *UnstreamLight(const char *ipData, unsigned int &oNbBytesRead)
{
    // First, look up type, DO NOT increment data pointer
    Light::Type type = *(reinterpret_cast<const Light::Type *>(ipData));

    // Instantiate light and unstream it
    oNbBytesRead = 0u;
    Light *pRet = nullptr;
    switch (type)
    {
    case Light::Type::CONSTANT:
        pRet = new ConstantLight(0u);
        pRet->UnStream(ipData, oNbBytesRead);
        break;
    case Light::Type::FLICKERING:
        pRet = new FlickeringLight(0u, 0u);
        pRet->UnStream(ipData, oNbBytesRead);
        break;
    default: // Should never be reached
        break;
    }
    return pRet;
}

CType LightTools::GetMaxInterpolationDist(unsigned int iLightValue)
{
    return std::max<CType>(CType(static_cast<int>(iLightValue)) * CType(3) / POSITION_SCALE, CType(100) / POSITION_SCALE);
}

unsigned int LightTools::GetMinLight(unsigned int iLightValue)
{
    return Clamp<unsigned int>((iLightValue * iLightValue * iLightValue) / (255u * 255u), std::min<unsigned int>(50u, iLightValue), 230u);
}

// ****** ConstantLight *******

ConstantLight::ConstantLight(unsigned int iValue) : Light(Light::Type::CONSTANT),
                                                        m_Value(iValue)
{   
}

ConstantLight::~ConstantLight()
{

}

unsigned int ConstantLight::ComputeStreamSize() const
{
    return Light::ComputeStreamSize() + sizeof(unsigned int);
}

void ConstantLight::Stream(char *&ioData, unsigned int &oNbBytesWritten) const
{
    oNbBytesWritten = 0;
    Light::Stream(ioData, oNbBytesWritten);

    *(reinterpret_cast<unsigned int *>(ioData)) = m_Value;
    oNbBytesWritten += sizeof(unsigned int);
    ioData += sizeof(unsigned int);
}

void ConstantLight::UnStream(const char *ipData, unsigned int &oNbBytesRead)
{
    oNbBytesRead = 0;
    Light::UnStream(ipData, oNbBytesRead);
    ipData += oNbBytesRead;

    oNbBytesRead += sizeof(unsigned int);
    m_Value = *reinterpret_cast<const unsigned int *>(ipData);
}

unsigned int ConstantLight::GetValue() const
{
    return m_Value;
}

// ****** FlickeringLight *******

FlickeringLight::FlickeringLight(unsigned int iLow, unsigned int iHigh):
    Light(Light::Type::FLICKERING),
    m_Low(iLow),
    m_High(iHigh)
{
    m_Clock.restart();
}

FlickeringLight::~FlickeringLight()
{
}

unsigned int FlickeringLight::ComputeStreamSize() const
{
    return Light::ComputeStreamSize() + 2 * sizeof(unsigned int);
}

void FlickeringLight::Stream(char *&ioData, unsigned int &oNbBytesWritten) const
{
    oNbBytesWritten = 0;
    Light::Stream(ioData, oNbBytesWritten);

    *(reinterpret_cast<unsigned int *>(ioData)) = m_Low;
    oNbBytesWritten += sizeof(unsigned int);
    ioData += sizeof(unsigned int);

    *(reinterpret_cast<unsigned int *>(ioData)) = m_High;
    oNbBytesWritten += sizeof(unsigned int);
    ioData += sizeof(unsigned int);
}

void FlickeringLight::UnStream(const char *ipData, unsigned int &oNbBytesRead)
{
    oNbBytesRead = 0;
    Light::UnStream(ipData, oNbBytesRead);
    ipData += oNbBytesRead;

    oNbBytesRead += sizeof(unsigned int);
    m_Low = *reinterpret_cast<const unsigned int *>(ipData);
    ipData += sizeof(unsigned int);

    oNbBytesRead += sizeof(unsigned int);
    m_High = *reinterpret_cast<const unsigned int *>(ipData);
}

unsigned int FlickeringLight::GetValue() const
{
    int elapsedTime = m_Clock.getElapsedTime().asMilliseconds() % 4000;
    unsigned int val = m_Low;

    if (elapsedTime <= 100)
        val = m_High;
    else if (1000 <= elapsedTime && elapsedTime <= 1150)
        val = m_High;
    else if (1500 <= elapsedTime && elapsedTime <= 1600)
        val = m_High;
    else if (3200 <= elapsedTime && elapsedTime <= 3250)
        val = m_High;

    return val;
}
