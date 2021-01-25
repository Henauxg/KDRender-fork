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
    oNbBytesWritten = ComputeStreamSize();
    *(reinterpret_cast<Light::Type *>(ioData)) = m_Type;
    ioData += oNbBytesWritten;
}

void Light::UnStream(const char *ipData, unsigned int &oNbBytesRead)
{
    oNbBytesRead = ComputeStreamSize();
    m_Type = *reinterpret_cast<const Light::Type *>(ipData);
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
