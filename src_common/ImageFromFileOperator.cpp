#include "ImageFromFileOperator.h"

// I was originally planning to use boost::gil, but this is too cumbersome and wouldn't
// work anyway (need to use an antiquated version of libpng + cryptic compile errors)
#include <SFML/Graphics.hpp>

ImageFromFileOperator::ImageFromFileOperator(std::map<unsigned int, unsigned char> &ioPalette) : 
    m_Height(0),
    m_Width(0),
    m_pData(nullptr),
    m_Palette(ioPalette)
{
}

ImageFromFileOperator::~ImageFromFileOperator()
{
    // If GetData() has been called prior to destructor call, m_pData is null
    if(m_pData)
        delete[] m_pData;
    m_pData = nullptr;
}

void ImageFromFileOperator::SetRelativePath(const std::string &iPath)
{
    m_RelativePath = iPath;
}

KDBData::Error ImageFromFileOperator::Run(bool iIsTexture)
{
    // Reset prior runs (if any)
    if (m_pData)
        delete[] m_pData;
    m_pData = nullptr;

    KDBData::Error ret = KDBData::Error::OK;

    sf::Image image;
    if (image.loadFromFile(m_RelativePath) &&
        image.getSize().x > 0 && image.getSize().y > 0)
    {
        m_Height = image.getSize().y;
        m_Width = image.getSize().x;

        m_pData = new unsigned char[m_Width * m_Height];
        if(m_pData)
        {
            for (unsigned int x = 0; x < m_Width; x++)
            {
                for (unsigned int y = 0; y < m_Height; y++)
                {
                    // The texture is flipped before being stored into the map.
                    // This avoids cache-misses when texturing the walls (for the floors/ceiling,
                    // the orientation doesn't matter)
                    sf::Color c = image.getPixel(x, y);
                    unsigned int cint32 = 0;
                    unsigned char *pColPtr = reinterpret_cast<unsigned char *>(&cint32);
                    *pColPtr++ = c.r;
                    *pColPtr++ = c.g;
                    *pColPtr++ = c.b;
                    *pColPtr = 255u;

                    unsigned char cint8;
                    auto found = m_Palette.find(cint32);
                    if(found != m_Palette.end())
                        cint8 = found->second;
                    else
                    {
                        if(m_Palette.size() >= 256)
                            cint8 = 0; // TODO find closest color in palette instead of returning 0
                        else
                        {
                            cint8 = m_Palette.size();
                            m_Palette[cint32] = cint8;
                        }
                    }

                    m_pData[x * m_Height + y] = cint8;
                }
            }
        }
        else
            ret = KDBData::Error::UNKNOWN_FAILURE;

        if (iIsTexture)
        {
            unsigned int heightLog2 = 0;
            while (m_Height = m_Height >> 1u)
                heightLog2++;
            m_Height = heightLog2;

            unsigned int widthLog2 = 0;
            while (m_Width = m_Width >> 1u)
                widthLog2++;
            m_Width = widthLog2;
        }
    }
    else
        ret = KDBData::Error::CANNOT_LOAD_TEXTURE;

    return ret;
}

unsigned int ImageFromFileOperator::GetHeight() const
{
    return m_Height;
}

unsigned int ImageFromFileOperator::GetWidth() const
{
    return m_Width;
}

unsigned char* ImageFromFileOperator::GetData()
{
    unsigned char *ret = m_pData;
    m_pData = nullptr;
    return ret;
}