#ifndef ImageFromFileOperator_h
#define ImageFromFileOperator_h

#include "KDTreeBuilderData.h"

#include <string>

class ImageFromFileOperator
{
public:
    ImageFromFileOperator();
    virtual ~ImageFromFileOperator();

public:
    void SetRelativePath(const std::string &iPath);
    
public:
    // If iIsTexture, height and width will be stored as a power of two
    KDBData::Error Run(bool iIsTexture = true);

public:
    unsigned int GetHeight() const;
    unsigned int GetWidth() const;
    unsigned char* GetData();

protected:
    std::string m_RelativePath;

    // Output data
    unsigned int m_Height;
    unsigned int m_Width;
    unsigned char *m_pData;
};

#endif
