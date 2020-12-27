#include "MapParser.h"
#include "Map.h"

#include <boost/bind.hpp>
#include <boost/config/warning_disable.hpp>
#include <boost/spirit/include/qi.hpp>

namespace
{
    class ExpressionAccumulator
    {
    public:
        ExpressionAccumulator(Map::Data &oData):
            m_Data(oData)
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

        void SetSectorCeiling(int iCeiling)
        {
            m_Data.m_Sectors.back().m_Ceiling = iCeiling;
        }

        void SetSectorFloor(int iFloor)
        {
            m_Data.m_Sectors.back().m_Floor = iFloor;
        }

        void PushOutlineVertex(boost::fusion::vector<int, int> &iPosition)
        {
            Map::Data::Sector::Vertex vertex;
            vertex.m_X = boost::fusion::at_c<0>(iPosition);
            vertex.m_Y = boost::fusion::at_c<1>(iPosition);
            m_Data.m_Sectors.back().m_Outline.push_back(vertex);
        }

        void PushNewHole()
        {
            m_Data.m_Sectors.back().m_Holes.push_back(Map::Data::Sector::Polygon());
        }

        void PushHoleVertex(boost::fusion::vector<int, int> &iPosition)
        {
            Map::Data::Sector::Vertex vertex;
            vertex.m_X = boost::fusion::at_c<0>(iPosition);
            vertex.m_Y = boost::fusion::at_c<1>(iPosition);
            m_Data.m_Sectors.back().m_Holes.back().push_back(vertex);
        }

        void SetPlayerStartPosition(boost::fusion::vector<int, int> &iPosition)
        {
            m_Data.m_PlayerStartPosition.first = boost::fusion::at_c<0>(iPosition);
            m_Data.m_PlayerStartPosition.second = boost::fusion::at_c<1>(iPosition);
        }

        void SetPlayerStartDirection(int iDirection)
        {
            m_Data.m_PlayerStartDirection = iDirection;
        }
        
    public:
        Map::Data &m_Data;
    };

    namespace qi = boost::spirit::qi;
    namespace ascii = boost::spirit::ascii;

    template <typename Iterator>
    struct MapGrammar : qi::grammar<Iterator, ascii::space_type>
    {
        MapGrammar(ExpressionAccumulator &iAccumulator) : MapGrammar::base_type(expression)
        {
            expression =
                *(sector) >> 
                player >> 
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
                    elevation) >> 
                    closeBracket
                ;

            outline =
                "outline" >> openBracket >> outlineVertices >> closeBracket
                ;

            outlineVertices =
                "vertices" >> openBracket >> *(outlineVertex) >> closeBracket
                ;

            outlineVertex =
                vertex [boost::bind(&ExpressionAccumulator::PushOutlineVertex, &iAccumulator, _1)]
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
                vertex [boost::bind(&ExpressionAccumulator::PushHoleVertex, &iAccumulator, _1)]
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

            vertex =   
                openBracket >> qi::int_ >> ',' >> qi::int_ >> closeBracket  
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
        qi::rule<Iterator, boost::fusion::vector<int, int>(), ascii::space_type> outlineVertex;
        qi::rule<Iterator, ascii::space_type> hole;
        qi::rule<Iterator, ascii::space_type> holeVertices;
        qi::rule<Iterator, boost::fusion::vector<int, int>(), ascii::space_type> holeVertex;
        qi::rule<Iterator, ascii::space_type> elevation;
        qi::rule<Iterator, int(), ascii::space_type> floor;
        qi::rule<Iterator, int(), ascii::space_type> ceiling;
        
        qi::rule<Iterator, ascii::space_type> openBracket, closeBracket;
        qi::rule<Iterator, boost::fusion::vector<int, int>(), ascii::space_type> vertex;
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