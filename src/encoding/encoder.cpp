#include"encoder.h"
#include"misc.h"
#include"shareheader.h"


static const int DEF_BYTE_CNT = 8;
static const int DEF_BYTE_MASK = 0x7;

Encoder::Encoder() {
    m_data = NULL;
    m_size = 0;
    m_upto = 0;
    m_bit_pos = 0;
}

Encoder::Encoder(void* data, int size) {
    set(data, size);
}

void Encoder::set(void* data, int size) {
    m_data = (uint_8*)data;
    m_size = size;
    m_upto = 0;
    m_bit_pos = 0;
}

bool Encoder::makeSpace(int len) {
    if (0 <= len && m_upto + len <= m_size) {
        return true;
    } else {
        return false;
    }
}

uint_8* Encoder::data(int pos) {
    return &m_data[pos];
}

template<typename T, int x>
bool Encoder::genBit(const T* in, const char* name) {
    uint_8* psz = NULL;
    bool bOk = false;
    int shift = 0;
    uint_8 u = 0;
    uint_8 mask = 0;
    uint_8 hi = 0;

    assert(x < DEF_BYTE_CNT); 
    
    if (m_bit_pos + x <= DEF_BYTE_CNT) {
        if (!makeSpace(1)) {
            LOG_WARN("gen_bit| name=%s| x=%d|"
                " size=%d| upto=%d| bit_pos=%d|"
                " msg=space is full|",
                SAFE_STR(name), x, m_size, m_upto,
                m_bit_pos);
                
            return false;
        }
        
        psz = data(m_upto); 

        if (0 < m_bit_pos) {
            shift = DEF_BYTE_CNT - m_bit_pos;
            u = *psz;
            hi = ((u >> shift) << shift); 
        } else {
            hi = 0;    
        }

        if (NULL != in) {
            mask = (uint_8)(0x1 << x);
            mask -= 1;
            
            u = (uint_8)(*in);
            u &= mask;
        } else {
            u = 0;
        }

        if (m_bit_pos + x < DEF_BYTE_CNT) {
            shift = DEF_BYTE_CNT - (m_bit_pos + x);
            u <<= shift;
            
            *psz = hi | u;
            m_bit_pos += x;
        } else {
            *psz = hi | u;
            m_bit_pos = 0;
            ++m_upto;
        }
        
        bOk = true;
    } else {
        LOG_WARN("gen_bit| name=%s| x=%d|"
            " size=%d| upto=%d| bit_pos=%d|"
            " msg=invalid bit position|",
            SAFE_STR(name), x, m_size, m_upto,
            m_bit_pos);
        bOk = false;
    }

    return bOk;
}

template<typename T, int x>
bool Encoder::genByte(const T* in, const char* name) {
    uint_8* psz = NULL;
    bool bOk = false;
    int cnt = 0;
    uint_32 u = 0;
    
    assert(0 == (x & DEF_BYTE_MASK));

    if (0 == m_bit_pos) {
        cnt = (x >> 3);
        if (!makeSpace(cnt)) {
            LOG_WARN("gen_byte| name=%s| x=%d|"
                " size=%d| upto=%d| bit_pos=%d|"
                " msg=space is full|",
                SAFE_STR(name), x, m_size, m_upto,
                m_bit_pos);
            
            return false;
        }

        if (NULL != in) {
            u = (uint_32)(*in);
        }

        psz = data(m_upto);
        for (int i=cnt - 1; 0 <= i; --i) {
            psz[i] = (u & 0xFF);
            u >>= DEF_BYTE_CNT;
        }

        m_upto += cnt;
        bOk = true;
    } else {
        LOG_WARN("gen_byte| name=%s| x=%d|"
            " size=%d| upto=%d| bit_pos=%d|"
            " msg=invalid bit position|",
            SAFE_STR(name), x, m_size, m_upto,
            m_bit_pos);
    }
    
    return bOk;
}

template<typename T, int x>
bool Encoder::write(const T* in, int offset) {
    uint_8* psz = NULL;
    bool bOk = false;
    int cnt = 0;
    uint_32 u = 0;
    
    assert(0 == (x & DEF_BYTE_MASK));

    cnt = (x >> 3);
    if (0 <= offset && offset + cnt <= m_size) { 
        if (NULL != in) {
            u = (uint_32)(*in);
        }

        psz = data(offset);
        for (int i=cnt - 1; 0 <= i; --i) {
            psz[i] = (u & 0xFF);
            u >>= DEF_BYTE_CNT;
        }

        bOk = true;
    } else {
        LOG_WARN("encoder_write| x=%d|"
            " size=%d| upto=%d| offset=%d|"
            " msg=invalid offset|",
            x, m_size, m_upto,
            offset);
    }
    
    return bOk;
}

template bool Encoder::genBit<bool, 1>(
    const bool* in, const char* name);
template bool Encoder::genBit<uint_8, 2>(
    const uint_8* in, const char* name);
template bool Encoder::genBit<uint_8, 4>(
    const uint_8* in, const char* name);
template bool Encoder::genBit<uint_8, 5>(
    const uint_8* in, const char* name);
template bool Encoder::genBit<uint_8, 7>(
    const uint_8* in, const char* name);

template bool Encoder::genByte<uint_8, 8>(
    const uint_8* in, const char* name);
template bool Encoder::genByte<uint_16, 16>(
    const uint_16* in, const char* name);
template bool Encoder::genByte<int_32, 24>(
    const int_32* in, const char* name);
template bool Encoder::genByte<uint_32, 32>(
    const uint_32* in, const char* name);

template bool Encoder::write<uint_8, 8>(
    const uint_8* in, int offset);
template bool Encoder::write<uint_16, 16>(
    const uint_16* in, int offset);
template bool Encoder::write<int_32, 24>(
    const int_32* in, int offset);
template bool Encoder::write<uint_32, 32>(
    const uint_32* in, int offset);

int Encoder::curPos() {
    return m_upto;
}

int Encoder::size() {
    return m_size;
}

bool Encoder::generate(const EncRule rules[], 
    int cnt, const void* in) {
    const char* origin = NULL;
    const char* psz = NULL;
    const EncRule* rule = NULL;
    bool bOk = true;

    origin = (const char*)in;
    for (int i=0; i<cnt && bOk; ++i) {
        rule = &rules[i];
        psz = origin + rule->m_offset;
        
        switch (rule->m_type) {
        case U_INT_1:
            bOk = genBit<bool, 1>(
                (const bool*)psz, rule->m_name);
            break;

        case U_INT_2:
            bOk = genBit<uint_8, 2>(
                (const uint_8*)psz, rule->m_name);
            break;
            
        case U_INT_4:
            bOk = genBit<uint_8, 4>(
                (const uint_8*)psz, rule->m_name);
            break;
            
        case U_INT_5:
            bOk = genBit<uint_8, 5>(
                (const uint_8*)psz, rule->m_name);
            break;

        case U_INT_7:
            bOk = genBit<uint_8, 7>(
                (const uint_8*)psz, rule->m_name);
            break;

        case U_INT_8:
            bOk = genByte<uint_8, 8>(
                (const uint_8*)psz, rule->m_name);
            break;

        case U_INT_16:
            bOk = genByte<uint_16, 16>(
                (const uint_16*)psz, rule->m_name);
            break;

        case INT_24:
            bOk = genByte<int_32, 24>(
                (const int_32*)psz, rule->m_name);
            break;

        case U_INT_32:
            bOk = genByte<uint_32, 32>(
                (const uint_32*)psz, rule->m_name);
            break;

        default:
            bOk = false;
            break;
        }
    }

    return bOk;
}

bool Encoder::genU32(const uint_32* u32) {
    bool bOk = false;

    bOk = genByte<uint_32, 32>(u32, NULL);
    return bOk;
}

bool Encoder::genU16(const uint_16* u16) {
    bool bOk = false;

    bOk = genByte<uint_16, 16>(u16, NULL);
    return bOk;
}

bool Encoder::genU8(const uint_8* u8) {
    bool bOk = false;

    bOk = genByte<uint_8, 8>(u8, NULL);
    return bOk;
}

bool Encoder::genStr(const AVal* val) {
    uint_8* psz = NULL;
    bool bOk = false;

    if (0 < val->m_size) {
        bOk = makeSpace(val->m_size);
        if (bOk) {
            psz = data(m_upto);
            MiscTool::bcopy(psz, val->m_data, val->m_size);
            m_upto += val->m_size;
        }
    } else {
        bOk = true;
    }
    
    return bOk;
}

bool Encoder::genStrU8(const AVal* val) {
    bool bOk = false;
    uint_8 u8 = 0;

    if (0 > val->m_size || 0xFF < val->m_size) {
        return false;
    }
    
    u8 = (uint_8)val->m_size;
    bOk = genU8(&u8);
    if (bOk) {
        bOk = genStr(val);
    }

    return bOk;
}

bool Encoder::genStrU16(const AVal* val) {
    bool bOk = false;
    uint_16 u16 = 0;

    if (0 > val->m_size || 0xFFFF < val->m_size) {
        return false;
    }
    
    u16 = (uint_16)val->m_size;
    bOk = genU16(&u16);
    if (bOk) {
        bOk = genStr(val);
    }

    return bOk;
}

bool Encoder::genChar(int cnt, uint_8 ch) {
    uint_8* psz = NULL;
    bool bOk = false;

    if (0 < cnt) {
        bOk = makeSpace(cnt);
        if (bOk) {
            psz = data();
            while (0 < cnt--) {
                psz[m_upto++] = ch;
            }
        }
    } else {
        bOk = true;
    }
    
    return bOk;
}

bool Encoder::writeU16(const uint_16* u16, int offset) {
    return write<uint_16, 16>(u16, offset);
}

bool Encoder::writeU32(const uint_32* u32, int offset) {
    return write<uint_32, 32>(u32, offset);
}

Decoder::Decoder() {
    set(NULL, 0);
}

Decoder::Decoder(const void* data, int size) {
    set(data, size);
}

void Decoder::set(const void* data, int size) {
    m_data = (const uint_8*)data;
    m_size = size;
    m_upto = 0;
    m_bit_pos = 0;
}

bool Decoder::isEnd() {
    return !(m_upto < m_size) && 0 == m_bit_pos;
}

int Decoder::leftLen() {
    if (m_upto < m_size) {
        return m_size - m_upto;
    } else {
        return 0;
    }
}

int Decoder::size() {
    return m_size;
}

int Decoder::curPos() {
    return m_upto;
}

const uint_8* Decoder::data(int pos) {
    return &m_data[pos];
}

template<typename T, int x>
bool Decoder::parseByte(T* out, const char* name) {
    bool bOk = false;
    uint_32 u = 0;
    int cnt = 0;
    
    assert(0 == (x & DEF_BYTE_MASK));
    
    cnt = x >> 3;
    if (m_upto + cnt <= m_size && 0 == m_bit_pos) {
        for (int i=0; i<cnt; ++i) {
            u <<= 8;
            u += m_data[m_upto++];
        }
        
        bOk = true;
    } else {
        LOG_WARN("parse_byte| name=%s| x=%d|"
            " size=%d| upto=%d| bit_pos=%d|"
            " msg=parse error|",
            SAFE_STR(name), x, m_size, m_upto,
            m_bit_pos);
    }
    
    if (NULL != out) {
        *out = (T)u;
    }

    return bOk;
}

template<typename T, int x>
bool Decoder::parseBit(T* out, const char* name) {
    bool bOk = false;
    int shift = 0;
    uint_8 u = 0;
    uint_8 mask = 0;
    
    assert(x < DEF_BYTE_CNT);
    
    if (m_upto < m_size && 
        m_bit_pos + x <= DEF_BYTE_CNT) {
        u = m_data[m_upto];

        if (m_bit_pos + x < DEF_BYTE_CNT) {
            shift = DEF_BYTE_CNT - (m_bit_pos + x);
            u >>= shift;
            
            m_bit_pos += x;
        } else {
            m_bit_pos = 0;
            ++m_upto;
        }
        
        mask = (uint_8)(0x1 << x);
        mask -= 1;
        u &= mask;
        bOk = true;
    } else {
        LOG_WARN("parse_bit| name=%s| x=%d|"
            " size=%d| upto=%d| bit_pos=%d|"
            " msg=parse error|",
            SAFE_STR(name), x, m_size, m_upto,
            m_bit_pos);
    }
    
    if (NULL != out) {
        *out = (T)u;
    }

    return bOk;
}

template<>
bool Decoder::parseByte<int_32, 24>(int_32* out, const char* name) {
    bool bOk = false;
    uint_32 u = 0;
    union {int_32 v:24, r:8;} tmp;
    
    if (m_upto + 3 <= m_size && 0 == m_bit_pos) {
        for (int i=0; i<3; ++i) {
            u <<= 8;
            u += m_data[m_upto++];
        }
        
        tmp.v = (int_32)u;
        bOk = true;
    } else {
        LOG_WARN("parse_int_24| name=%s|"
            " size=%d| upto=%d| bit_pos=%d|"
            " msg=parse error|",
            SAFE_STR(name), m_size, m_upto,
            m_bit_pos);
    }
    
    if (NULL != out) {
        *out = tmp.v;
    }

    return bOk;
}

template<typename T, int x>
bool Decoder::read(T* out, int offset) {
    const uint_8* psz = NULL;
    bool bOk = false;
    int cnt = 0;
    uint_32 u = 0;
    
    assert(0 == (x & DEF_BYTE_MASK));

    cnt = (x >> 3);
    if (0 <= offset && offset + cnt <= m_size) { 
        psz = data(offset);
        for (int i=0; i<cnt; ++i) {
            u <<= 8;
            u += *(psz++);
        } 

        bOk = true;
    } else {
        LOG_WARN("decoder_read| x=%d|"
            " size=%d| upto=%d| offset=%d|"
            " msg=invalid offset|",
            x, m_size, m_upto,
            offset);
    }

    if (NULL != out) {
        *out = (T)u;
    }
    
    return bOk;
}

template<>
bool Decoder::read<int_32, 24>(int_32* out, int offset) {
    const uint_8* psz = NULL;
    bool bOk = false; 
    uint_32 u = 0;
    union {int_32 v:24, r:8;} tmp;
 
    if (0 <= offset && offset + 3 <= m_size) { 
        psz = data(offset);
        for (int i=0; i<3; ++i) {
            u <<= 8;
            u += *(psz++);
        }

        tmp.v = (int_32)u;
        bOk = true;
    } else {
        LOG_WARN("decoder_read_int_24|"
            " size=%d| upto=%d| offset=%d|"
            " msg=invalid offset|",
            m_size, m_upto, offset);
    }

    if (NULL != out) {
        *out = tmp.v;
    }
    
    return bOk;
}

template bool Decoder::parseBit<bool, 1>(
    bool* out, const char* name);
template bool Decoder::parseBit<uint_8, 2>(
    uint_8* out, const char* name);
template bool Decoder::parseBit<uint_8, 4>(
    uint_8* out, const char* name);
template bool Decoder::parseBit<uint_8, 5>(
    uint_8* out, const char* name);
template bool Decoder::parseBit<uint_8, 7>(
    uint_8* out, const char* name);

template bool Decoder::parseByte<uint_8, 8>(
    uint_8* out, const char* name);
template bool Decoder::parseByte<uint_16, 16>(
    uint_16* out, const char* name);
template bool Decoder::parseByte<uint_32, 32>(
    uint_32* out, const char* name);

template bool Decoder::read<uint_8, 8>(
    uint_8* out, int offset);
template bool Decoder::read<uint_16, 16>(
    uint_16* out, int offset);
template bool Decoder::read<uint_32, 32>(
    uint_32* out, int offset);

bool Decoder::parse(const EncRule rules[],
    int cnt, void* out) {
    char* origin = NULL;
    char* psz = NULL;
    const EncRule* rule = NULL;
    bool bOk = true;

    origin = (char*)out;
    for (int i=0; i<cnt && bOk; ++i) {
        rule = &rules[i];
        psz = origin + rule->m_offset;
        
        switch (rule->m_type) {
        case U_INT_1:
            bOk = parseBit<bool, 1>((bool*)psz, rule->m_name);
            break;

        case U_INT_2:
            bOk = parseBit<uint_8, 2>((uint_8*)psz, rule->m_name);
            break;
            
        case U_INT_4:
            bOk = parseBit<uint_8, 4>((uint_8*)psz, rule->m_name);
            break;
            
        case U_INT_5:
            bOk = parseBit<uint_8, 5>((uint_8*)psz, rule->m_name);
            break;

        case U_INT_7:
            bOk = parseBit<uint_8, 7>((uint_8*)psz, rule->m_name);
            break;

        case U_INT_8:
            bOk = parseByte<uint_8, 8>((uint_8*)psz, rule->m_name);
            break;

        case U_INT_16:
            bOk = parseByte<uint_16, 16>((uint_16*)psz, rule->m_name);
            break;

        case INT_24:
            bOk = parseByte<int_32, 24>((int_32*)psz, rule->m_name);
            break;

        case U_INT_32:
            bOk = parseByte<uint_32, 32>((uint_32*)psz, rule->m_name);
            break;

        default:
            bOk = false;
            break;
        }
    }

    return bOk;
}


bool Decoder::parseU32(uint_32* u32) {
    return parseByte<uint_32, 32>(u32, NULL);
}

bool Decoder::parseU16(uint_16* u16) {
    return parseByte<uint_16, 16>(u16, NULL);
}

bool Decoder::parseU8(uint_8* u8) {
    return parseByte<uint_8, 8>(u8, NULL);
}

bool Decoder::readU16(uint_16* u16, int offset) {
    return read<uint_16, 16>(u16, offset);
}

bool Decoder::readU32(uint_32* u32, int offset) {
    return read<uint_32, 32>(u32, offset);
}

bool Decoder::parseStr(AVal* val, int len) {
    if (NULL != val) {
        val->m_data = NULL;
        val->m_size = 0;
    }

    if (0 == m_bit_pos && m_upto + len <= m_size) {
        if (0 < len) {
            if (NULL != val) {
                val->m_data = &m_data[m_upto];
                val->m_size = len;
            }

            m_upto += len;
        }
        
        return true;
    } else { 
        return false;
    }
}

bool Decoder::parseStrU8(AVal* val) {
    bool bOk = false;
    uint_8 u8 = 0;

    if (NULL != val) {
        val->m_data = NULL;
        val->m_size = 0;
    }
    
    bOk = parseU8(&u8);
    if (bOk) {
        bOk = parseStr(val, u8);
    }

    return bOk;
}

bool Decoder::parseStrU16(AVal* val) {
    bool bOk = false;
    uint_16 u16 = 0;

    if (NULL != val) {
        val->m_data = NULL;
        val->m_size = 0;
    }
    
    bOk = parseU16(&u16);
    if (bOk) {
        bOk = parseStr(val, u16);
    }

    return bOk;
}

