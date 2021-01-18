#include <iostream>
#include <fstream>
#include <sstream>

#include "Map.h"
#include "KDTreeBuilder.h"
#include "KDTreeMap.h"

int main(int argc, char **argv)
{
    std::string iFilePath;
    std::string oFilePath;
    bool displayHelpAndExit = false;
    
    for(unsigned int i = 1; i < argc; i++)
    {
        if("-i" == std::string(argv[i]))
        {
            if(++i == argc)
            {
                displayHelpAndExit = true;
                break;
            }
            else
            {
                iFilePath = std::string(argv[i]);
            }
        }
        else if("-o" == std::string(argv[i]))
        {
            if(++i == argc)
            {
                displayHelpAndExit = true;
                break;
            }
            else
            {
                oFilePath = std::string(argv[i]);
            }
        }
        else
        {
            displayHelpAndExit = true;
            break;
        }
    }   
    
    if(displayHelpAndExit)
    {
        std::cout << "Usage:" << std::endl;
        std::cout << "mapbuilder -i input_path -o output_path" << std::endl;
        
        return 1;
    }
    
    std::cout << "Input file: " << iFilePath << std::endl;
    std::cout << "Output file: " << oFilePath << std::endl;

    std::ifstream iStream(iFilePath);
    if(!iStream.is_open())
    {
        std::cout << "Error: input file not found" << std::endl;
        return 1;
    }

    std::stringstream mapString;
    std::string currentLine;
    while(getline(iStream, currentLine))
    {
        mapString << currentLine;
    }

    Map map;
    if(!map.ParseFromString(mapString.str()))
    {
        return 1;
    }

    std::cout << "Parsing successful" << std::endl;

    KDTreeBuilder builder(map);
    if(!builder.Build())
    {
        std::cout << "Error: KD-tree construction failed" << std::endl;
        return 1;
    }

    char *pStreamData = nullptr;
    KDTreeMap *pKDTreeMap = builder.GetOutputTree();

    if(pKDTreeMap)
    {
        unsigned outSize;
        pKDTreeMap->Stream(pStreamData, outSize);
        if(pStreamData)
        {
            std::ofstream oStream(oFilePath, std::ios::binary | std::ios::out);
            if(!oStream.is_open())
                std::cout << "Error: output file could not be created" << std::endl;

            oStream.write(pStreamData, outSize);

            delete pStreamData;
        }
        pStreamData = nullptr;

        std::cout << "KD-Tree successfully built (depth = " << pKDTreeMap->ComputeDepth() << ")" << std::endl;
        delete pKDTreeMap;
    }
    pKDTreeMap = nullptr;

    return 0;
}
