/**
 * Map file parser. Didn't try to make it robust or elegant.
 * 
 * Thomas Cormont - 12/24/2020
*/

#ifndef MapParser_h
#define MapParser_h

#include <string>
#include <utility>
#include <vector>
#include <memory>

#include "Map.h"

class MapParser
{
public:
    MapParser(const std::string &iString);
    virtual ~MapParser();

public:
    bool Parse(Map::Data &oData);

private:
    const std::string &m_String;
};

#endif