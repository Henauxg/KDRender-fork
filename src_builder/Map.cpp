#include "Map.h"
#include "MapParser.h"

#include <iostream>
#include <utility>

Map::Map()
{
}

Map::~Map()
{
}

bool Map::ParseFromString(const std::string &iString)
{
   MapParser mapParser(iString);
   return mapParser.Parse(m_Data);
}

Map::Data& Map::GetData()
{
   return m_Data;
}

const Map::Data& Map::GetData() const
{
   return m_Data;
}
