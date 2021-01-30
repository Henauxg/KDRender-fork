#include "Sprite.h"

#include "ImageFromFileOperator.h"

Sprite::Sprite():
    m_pLight(nullptr)
{

}

Sprite::~Sprite()
{
    if(m_pLight)
        delete m_pLight;
    m_pLight = nullptr;

    for (unsigned int i = 0; i < m_States.size(); i++)
    {
        if (m_States[i])
            delete m_States[i];
    }
}

bool Sprite::LoadAllFromPaths()
{
    bool everyThingWentFine = false;

    for (unsigned int i = 0; i < m_States.size(); i++)
    {
        if(!m_States[i] || !m_States[i]->LoadAllFromPaths(m_Width, m_Height))
            everyThingWentFine = false;
    }

    return everyThingWentFine;
}

void Sprite::AddState(State *ipState)
{
    m_States.push_back(ipState);
}

const Sprite::State* Sprite::GetState(State::Name iName) const
{
    // Linear search, not so good
    const State *pFound = nullptr;

    for (unsigned int i = 0; i < m_States.size(); i++)
    {
        if (m_States[i] && m_States[i]->GetName() == iName)
            pFound = m_States[i];
    }

    return pFound;
}

Sprite::State::State()
{
}

Sprite::State::~State()
{
}

void Sprite::State::AddImageSet(const ImageSet &iImageSet)
{
    m_ImageSets.push_back(iImageSet);
}

bool Sprite::State::LoadAllFromPaths(unsigned int &oWidth, unsigned int &oHeight)
{
    bool everyThingWentFine = true;
    oWidth = 0;
    oHeight = 0;

    for (unsigned int i = 0; i < m_ImageSets.size(); i++)
    {
        unsigned int localWidth;
        unsigned int localHeight;

        if(!m_ImageSets[i].LoadAllFromPaths(localWidth, localHeight))
            everyThingWentFine = false;

        if (i == 0)
        {
            oWidth = localWidth;
            oHeight = localHeight;
        }
        else if (localHeight != oHeight || localWidth != oWidth)
            everyThingWentFine = false;
    }

    return everyThingWentFine;
}

const Sprite::State::ImageSet* Sprite::State::GetImageSet(CType iObjectX, CType iObjectY, CType iPlayerX, CType iPlayerY) const
{
    // Direction not implemented yet
    if (!m_ImageSets.empty())
        return &m_ImageSets[0];
    else
        return nullptr;
}

Sprite::State::Name Sprite::State::GetName() const
{
    return m_Name;
}

Sprite::State::ImageSet::ImageSet():
    m_TotalDuration(0u)
{
}

Sprite::State::ImageSet ::~ImageSet()
{
    for (unsigned int i = 0; i < m_pData.size(); i++)
    {
        if (m_pData[i])
            delete m_pData[i];
    }
}

void Sprite::State::ImageSet::AddImage(const std::string &iPath, const unsigned int iDuration)
{
    m_InputPaths.push_back(iPath);
    m_Durations.push_back(iDuration);
    m_TotalDuration += iDuration;
}

bool Sprite::State::ImageSet::LoadAllFromPaths(unsigned int &oWidth, unsigned int &oHeight)
{
    if(m_Durations.size() != m_InputPaths.size())
        return false; // Should never happen

    bool everythingWentFine = true;
    oWidth = 0;
    oHeight = 0;

    for(unsigned int i = 0; i < m_InputPaths.size(); i++)
    {
        ImageFromFileOperator imageFromFileOper;
        imageFromFileOper.SetRelativePath(m_InputPaths[i]);
        
        if (imageFromFileOper.Run(false) != KDBData::Error::OK)
            everythingWentFine = false;

        m_pData.push_back(imageFromFileOper.GetData());

        if (i == 0)
        {
            oHeight = imageFromFileOper.GetHeight();
            oWidth = imageFromFileOper.GetWidth();
        }
        else if(oHeight != imageFromFileOper.GetHeight() || oWidth != imageFromFileOper.GetWidth())
            everythingWentFine = false;
    }

    return everythingWentFine;
}

const unsigned char* Sprite::State::ImageSet::GetImgData(unsigned int iTime) const
{
    if(m_Durations.size() != m_pData.size())
        return nullptr; // Should never happen

    unsigned int timeMod = iTime % m_TotalDuration;
    unsigned int i = 0;
    while ((i + 1) < m_Durations.size() && iTime <= m_Durations[i + 1])
        i++;

    return m_pData[i];
}