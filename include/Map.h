/**
 * Thomas Cormont - 12/23/2020
*/

#ifndef Map_h
#define Map_h

#include <vector>
#include <string>
#include <utility>
#include <map>

class Map
{
public:
    struct Data
    {
        struct Sector
        {
            struct Vertex
            {
                int m_X;
                int m_Y;

                // Convention: texture informations are stored on the first vertex of the wall
                int m_TexId;
                int m_TexUOffset;
                int m_TexVOffset;
            };

            using Polygon = std::vector<Vertex>;

            Polygon m_Outline;
            std::vector<Polygon> m_Holes;
            
            int m_Floor;
            int m_Ceiling;

            int m_FloorTexId;
            int m_CeilingTexId;
        };

        struct Texture
        {
            std::string m_Path;
        };

        std::vector<Texture> m_Textures;
        std::vector<Sector> m_Sectors;
        std::pair<int, int> m_PlayerStartPosition;
        int m_PlayerStartDirection;
    };

public:
    Map();
    virtual ~Map();

public:
    bool ParseFromString(const std::string &iString);

    Data& GetData();
    const Data& GetData() const;

protected:
    Data m_Data;
};

#endif
