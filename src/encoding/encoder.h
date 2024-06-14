#ifndef __ENCODER_H__
#define __ENCODER_H__
#include"rtpproto.h"


enum EnumEncType {
    U_INT_1,
    U_INT_2,
    U_INT_4,
    U_INT_5,
    U_INT_7,

    U_INT_8,
    U_INT_16,
    INT_24,
    U_INT_32,
};

struct EncRule {
    EnumEncType m_type;
    int m_offset;
    const char* m_name;
};

class Encoder { 
public:
    Encoder(); 

    Encoder(void* data, int size);

    void set(void* data, int size);

    int size();
    int curPos();
    uint_8* data(int pos = 0);

    bool generate(const EncRule rules[], 
        int cnt, const void* in);

    bool genU32(const uint_32* u32);
    bool genU16(const uint_16* u16);
    bool genU8(const uint_8* u8);
    bool genStr(const AVal* val);
    bool genStrU8(const AVal* val);
    bool genStrU16(const AVal* val);
    bool genChar(int cnt, uint_8 ch);
    bool writeU16(const uint_16* u16, int offset);
    bool writeU32(const uint_32* u32, int offset);

private:
    bool makeSpace(int len);

    template<typename T, int x>
    bool genBit(const T* in, const char* name);

    template<typename T, int x>
    bool genByte(const T* in, const char* name);

    template<typename T, int x>
    bool write(const T* in, int offset);
    
private:
    uint_8* m_data;
    int m_size;
    int m_upto;
    int m_bit_pos;
};

class Decoder {
public:
    Decoder(); 
    Decoder(const void* data, int size);

    void set(const void* data, int size);

    bool isEnd(); 

    int leftLen();
    int size();
    int curPos();

    const uint_8* data(int pos = 0);

    bool parse(const EncRule rules[], int cnt, void* out);

    bool parseU32(uint_32* u32); 
    bool parseU16(uint_16* u16); 
    bool parseU8(uint_8* u8);
    bool parseStr(AVal* val, int len);
    bool parseStrU8(AVal* val);
    bool parseStrU16(AVal* val);

    bool readU16(uint_16* u16, int offset);
    bool readU32(uint_32* u32, int offset);

private:
    template<typename T, int x>
    bool parseBit(T* out, const char* name);

    template<typename T, int x>
    bool parseByte(T* out, const char* name);

    template<typename T, int x>
    bool read(T* out, int offset);

private:
    const uint_8* m_data;
    int m_size;
    int m_upto;
    int m_bit_pos;
};

#endif

