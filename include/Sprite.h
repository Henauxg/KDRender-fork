#ifndef Sprite_h
#define Sprite_h

#include "Light.h"

#include <vector>

class Sprite
{
public:
    Sprite();
    virtual ~Sprite();

public:
    class State
    {
    public:
        State();
        virtual ~State();

    public:
        enum Name
        {
            IDLE
        };

        class ImageSet
        {
        public:
            ImageSet();
            virtual ~ImageSet();

        public:
            void AddImage(const std::string &iPath, const unsigned int iDuration);
            bool LoadAllFromPaths(unsigned int &oWidth, unsigned int &oHeight);

        public:
            const unsigned char* GetImgData(unsigned int iTime) const;

        protected:
            std::vector<std::string> m_InputPaths;
            std::vector<unsigned char *> m_pData;
            std::vector<unsigned int> m_Durations;
            unsigned int m_TotalDuration;
        };

    public:
        void AddImageSet(const ImageSet &iImageSet);
        bool LoadAllFromPaths(unsigned int &oWidth, unsigned int &oHeight);

    public:
        const ImageSet *GetImageSet(CType iObjectX, CType iObjectY, CType iPlayerX, CType iPlayerY) const;
        Name GetName() const;

    protected:
        Name m_Name;
        std::vector<ImageSet> m_ImageSets;
        std::vector<int> m_ViewDirections;
    };

public:
    void AddState(State *ipState);
    const State* GetState(State::Name iState) const;

public:
    bool LoadAllFromPaths();

protected:
    Light *m_pLight;
    std::vector<State*> m_States;

    unsigned int m_Height;
    unsigned int m_Width;
};

#endif
