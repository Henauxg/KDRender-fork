#include "ImageFromFileOperator.h"

ImageFromFileOperator::ImageFromFileOperator() : 
    m_pData(nullptr)
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

KDBData::Error ImageFromFileOperator::Run()
{
    KDBData::Error ret = KDBData::Error::OK;

    return ret;
}

char* ImageFromFileOperator::GetData()
{
    char *ret = m_pData;
    m_pData = nullptr;
    return ret;
}