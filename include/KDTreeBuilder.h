#ifndef KDTreeBuilder_h
#define KDTreeBuilder_h

class Map;
class KDTreeMap;

class KDTreeBuilder
{
public:
    KDTreeBuilder(const Map &iMap);
    virtual ~KDTreeBuilder();

public:
    bool Build();
    KDTreeMap* GetOutputTree();

protected:
    const Map &m_Map;
    KDTreeMap *m_pKDTree;
};

#endif