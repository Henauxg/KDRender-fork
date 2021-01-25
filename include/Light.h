#ifndef Light_h
#define Light_h

// Abstract light class
class Light
{
public:
    enum class Type
    {
        CONSTANT
    };

public:
    Type GetType() const;

public:
    Light(Type iType);
    virtual ~Light() {}

public:
    virtual unsigned int ComputeStreamSize() const;
    virtual void Stream(char *&ioData, unsigned int &oNbBytesWritten) const;
    virtual void UnStream(const char *ipData, unsigned int &oNbBytesRead);

public:
    virtual unsigned int GetValue() const = 0;

protected:
    Type m_Type;
};

// Constant light
// Implements Light
class ConstantLight : public Light
{
public:
    ConstantLight(unsigned int iValue);
    virtual ~ConstantLight();

public:
    virtual unsigned int ComputeStreamSize() const override;
    virtual void Stream(char *&ioData, unsigned int &oNbBytesWritten) const override;
    virtual void UnStream(const char *ipData, unsigned int &oNbBytesRead) override;

public:
    virtual unsigned int GetValue() const override;

protected:
    unsigned int m_Value;
};

#endif