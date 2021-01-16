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
    KDBData::Error Run();
    char* GetData();

protected:
    std::string m_RelativePath;

    // Output data
    char *m_pData;
};

#endif
