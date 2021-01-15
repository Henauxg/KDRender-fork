#include "MapParser.h"

#include "Map.h"
#include "Consts.h"

#include <boost/bind.hpp>
#include <boost/config/warning_disable.hpp>
#include <boost/spirit/include/qi.hpp>

namespace
{
    class ExpressionAccumulator
    {
    public:
        ExpressionAccumulator(Map::Data &oData):
            m_Data(oData),
            m_CurrentSectorDefaultWallTexId(-1)
        {
        }
        
        virtual ~ExpressionAccumulator()
        {
        }
        
    public:
        void PushNewSector()
        {
            m_Data.m_Sectors.push_back(Map::Data::Sector());
        }

        void EndNewSector()
        {
            if(!m_CurrentSectorDefaultWallTexId != -1)
            {
                Map::Data::Sector &sector = m_Data.m_Sectors.back();

                for (unsigned int i = 0; i < sector.m_Outline.size(); i++)
                {
                    Map::Data::Sector::Vertex &vertex = sector.m_Outline[i];
                    if(vertex.m_TexId == -1)
                        vertex.m_TexId = m_CurrentSectorDefaultWallTexId;
                }

                for (unsigned int j = 0; j < sector.m_Holes.size(); j++)
                {
                    for (unsigned int i = 0; i < sector.m_Holes[j].size(); i++)
                    {
                        Map::Data::Sector::Vertex &vertex = sector.m_Holes[j][i];
                        if (vertex.m_TexId == -1)
                            vertex.m_TexId = m_CurrentSectorDefaultWallTexId;
                    }
                }
            }

            m_CurrentSectorDefaultWallTexId = -1;
        }

        void SetDefaultWallTexture(std::string &iName)
        {
            auto found = m_MapTextureToTexId.find(iName);
            if(found != m_MapTextureToTexId.end())
                m_CurrentSectorDefaultWallTexId = found->second;
            else // Texture wasn't found
                m_CurrentSectorDefaultWallTexId = -1;
        }

        void SetSectorCeiling(int iCeiling)
        {
            m_Data.m_Sectors.back().m_Ceiling = iCeiling;
        }

        void SetSectorFloor(int iFloor)
        {
            m_Data.m_Sectors.back().m_Floor = iFloor;
        }

        void SetCurrentVertexCoordinates(boost::fusion::vector<int, int> &iPosition)
        {
            m_CurrentVertex.m_X = boost::fusion::at_c<0>(iPosition);
            m_CurrentVertex.m_Y = boost::fusion::at_c<1>(iPosition);
            m_CurrentVertex.m_TexId = -1;
            m_CurrentVertex.m_TexUOffset = 0;
            m_CurrentVertex.m_TexVOffset = 0;
        }

        void SetCurrentVertexTexture(std::string &iName)
        {
            auto found = m_MapTextureToTexId.find(iName);
            if(found != m_MapTextureToTexId.end())
                m_CurrentVertex.m_TexId = found->second;
        }

        void SetCurrentVertexUVOffsets(boost::fusion::vector<int, int> &iPosition)
        {
            m_CurrentVertex.m_TexUOffset = boost::fusion::at_c<0>(iPosition);
            m_CurrentVertex.m_TexVOffset = boost::fusion::at_c<1>(iPosition);
        }

        void PushOutlineVertex()
        {
            m_Data.m_Sectors.back().m_Outline.push_back(m_CurrentVertex);
        }

        void PushNewHole()
        {
            m_Data.m_Sectors.back().m_Holes.push_back(Map::Data::Sector::Polygon());
        }

        void PushHoleVertex()
        {
            m_Data.m_Sectors.back().m_Holes.back().push_back(m_CurrentVertex);
        }

        void SetPlayerStartPosition(boost::fusion::vector<int, int> &iPosition)
        {
            m_Data.m_PlayerStartPosition.first = boost::fusion::at_c<0>(iPosition);
            m_Data.m_PlayerStartPosition.second = boost::fusion::at_c<1>(iPosition);
        }

        void SetPlayerStartDirection(int iDirection)
        {
            m_Data.m_PlayerStartDirection = iDirection << ANGLE_SHIFT;
        }
        
        void PushNewTexture()
        {
            Map::Data::Texture newTexture;
            m_Data.m_Textures.push_back(newTexture);
        }

        void SetTextureName(std::string &iName)
        {
            m_MapTextureToTexId[iName] = m_Data.m_Textures.size() - 1;
        }

        void SetTexturePath(std::string &iPath)
        {
            m_Data.m_Textures.back().m_Path = iPath;
        }

    public:
        Map::Data &m_Data;

        std::map<std::string, int> m_MapTextureToTexId;
        int m_CurrentSectorDefaultWallTexId;
        
        Map::Data::Sector::Vertex m_CurrentVertex;
    };

    namespace qi = boost::spirit::qi;
    namespace ascii = boost::spirit::ascii;

    template <typename Iterator>
    struct MapGrammar : qi::grammar<Iterator, ascii::space_type>
    {
        MapGrammar(ExpressionAccumulator &iAccumulator) : MapGrammar::base_type(expression)
        {
            expression =
                player >> 
                *(texture) >>
                *(sector)
                ;
            
            player =
                "player" >> openBracket >> start >> closeBracket
                ;
                
            start =
                "start" >> openBracket >> 
                *(position [boost::bind(&ExpressionAccumulator::SetPlayerStartPosition, &iAccumulator, _1)] |
                direction [boost::bind(&ExpressionAccumulator::SetPlayerStartDirection, &iAccumulator, _1)]) 
                >> closeBracket
                ;
                
            position =
                "position" >> vertex
                ;
                
            direction =
                "direction" >> openBracket >> qi::int_ >> closeBracket
                ;

            sector =
                "sector" >> 
                openBracket [boost::bind(&ExpressionAccumulator::PushNewSector, &iAccumulator)] >>
                    *(hole | 
                    outline | 
                    elevation |
                    defaultWallTexture) >> 
                    closeBracket [boost::bind(&ExpressionAccumulator::EndNewSector, &iAccumulator)]
                ;

            outline =
                "outline" >> openBracket >> outlineVertices >> closeBracket
                ;

            outlineVertices =
                "vertices" >> openBracket >> *(outlineVertex) >> closeBracket
                ;

            outlineVertex =
                vertex [boost::bind(&ExpressionAccumulator::PushOutlineVertex, &iAccumulator)]
                ;

            hole =
                "hole" >> 
                openBracket [boost::bind(&ExpressionAccumulator::PushNewHole, &iAccumulator)] >> 
                holeVertices >> 
                closeBracket
                ;

            holeVertices =
                "vertices" >> openBracket >> *(holeVertex) >> closeBracket
                ;

            holeVertex =
                vertex [boost::bind(&ExpressionAccumulator::PushHoleVertex, &iAccumulator)]
                ;

            elevation =
                "elevation" >> 
                openBracket >> 
                *(ceiling [boost::bind(&ExpressionAccumulator::SetSectorCeiling, &iAccumulator, _1)] |
                    floor [boost::bind(&ExpressionAccumulator::SetSectorFloor, &iAccumulator, _1)]) >> 
                    closeBracket
                ;

            ceiling =
                "ceiling" >> openBracket >> qi::int_ >> closeBracket
                ;

            floor =
                "floor" >> openBracket >> qi::int_ >> closeBracket
                ;

            defaultWallTexture =
                "defaultWallTexture" >>
                openBracket >>
                bracketedString [boost::bind(&ExpressionAccumulator::SetDefaultWallTexture, &iAccumulator, _1)]>>
                closeBracket
                ;

            vertex =   
                openBracket >>
                vertexCoordinates [boost::bind(&ExpressionAccumulator::SetCurrentVertexCoordinates, &iAccumulator, _1)] >>
                -(vertexTexture) >> // Optional
                closeBracket  
                ;

            vertexCoordinates %=
                qi::int_ >> "," >> qi::int_
                ;

            vertexTexture =
                openBracket >>
                openBracket >>
                bracketedString [boost::bind(&ExpressionAccumulator::SetCurrentVertexTexture, &iAccumulator, _1)] >>
                closeBracket >>
                openBracket >>
                vertexCoordinates [boost::bind(&ExpressionAccumulator::SetCurrentVertexUVOffsets, &iAccumulator, _1)] >> // recycling
                closeBracket >>
                closeBracket
                ;

            texture =
                "texture" >>  
                openBracket [boost::bind(&ExpressionAccumulator::PushNewTexture, &iAccumulator)] >>
                textureName >>
                texturePath >>
                closeBracket
                ;

            textureName =
                "name" >> 
                openBracket >>
                bracketedString [boost::bind(&ExpressionAccumulator::SetTextureName, &iAccumulator, _1)] >> 
                closeBracket
                ;

            texturePath =
                "path" >>
                openBracket >>
                bracketedString [boost::bind(&ExpressionAccumulator::SetTexturePath, &iAccumulator, _1)] >>
                closeBracket
                ;

            bracketedString %=
                +(qi::char_ - '}')
                ;

            openBracket = '{';
            closeBracket = '}';
        }

        qi::rule<Iterator, ascii::space_type> expression;
        
        qi::rule<Iterator, ascii::space_type> player;
        qi::rule<Iterator, ascii::space_type> start;
        qi::rule<Iterator, boost::fusion::vector<int, int>(), ascii::space_type> position;
        qi::rule<Iterator, int(), ascii::space_type> direction;
        
        
        qi::rule<Iterator, ascii::space_type> sector;
        qi::rule<Iterator, ascii::space_type> outline;
        qi::rule<Iterator, ascii::space_type> outlineVertices;
        qi::rule<Iterator, ascii::space_type> outlineVertex;
        qi::rule<Iterator, ascii::space_type> hole;
        qi::rule<Iterator, ascii::space_type> holeVertices;
        qi::rule<Iterator, ascii::space_type> holeVertex;
        qi::rule<Iterator, ascii::space_type> elevation;
        qi::rule<Iterator, int(), ascii::space_type> floor;
        qi::rule<Iterator, int(), ascii::space_type> ceiling;
        qi::rule<Iterator, ascii::space_type> defaultWallTexture;

        qi::rule<Iterator, ascii::space_type> openBracket, closeBracket;
        qi::rule<Iterator, ascii::space_type> vertex;
        qi::rule<Iterator, boost::fusion::vector<int, int>(), ascii::space_type> vertexCoordinates;
        qi::rule<Iterator, ascii::space_type> vertexTexture;

        qi::rule<Iterator, ascii::space_type> texture;
        qi::rule<Iterator, ascii::space_type> textureName;
        qi::rule<Iterator, ascii::space_type> texturePath;

        qi::rule<Iterator, std::string(), ascii::space_type> bracketedString;
    };
}

MapParser::MapParser(const std::string &iString):
    m_String(iString)
{
}

MapParser::~MapParser()
{
}

bool MapParser::Parse(Map::Data &oData)
{
    ExpressionAccumulator expressionAccumulator(oData);
    MapGrammar<std::string::const_iterator> mapGrammar(expressionAccumulator);

    std::string::const_iterator iter = m_String.begin();
    std::string::const_iterator end = m_String.end();
    boost::spirit::ascii::space_type space;
    bool ret = phrase_parse(iter, end, mapGrammar, space);

    if(iter != end)
    {
        ret = false;
        std::string rest(iter, end);
        std::cout << "Error: parsing stopped at \"" << rest << "\"" << std::endl;
    }

    return ret;
}