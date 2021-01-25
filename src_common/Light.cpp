#include "Light.h"

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
    default: // Should never be reached 
        break;
    }
    return pRet;
}

CType GetMaxInterpolationDist(unsigned int iLightValue)
{
    return CType(static_cast<int>(iLightValue)) * CType(3) / POSITION_SCALE;
}

// ****** ConstantLight *******

ConstantLight::ConstantLight(unsigned int iValue):
    Light(Light::Type::CONSTANT),
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
