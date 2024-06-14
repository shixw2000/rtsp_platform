#include"bitset.h"
#include"cache.h"
#include"misc.h"


BitSet::BitSet(int base, int maxBit)
    : m_base(base) { 
    m_size = maxBit >> DEF_MEM_SHIFT_N;
    m_max_bit = m_size << DEF_MEM_SHIFT_N;
    m_upto = 0;
    m_bit_pos = 0;

    m_data = (unsigned*)CacheUtil::callocAlign(
        m_size, sizeof(unsigned));

    MiscTool::bzero(m_data, m_size * sizeof(unsigned));
}

BitSet::~BitSet() {
    CacheUtil::freeAlign(m_data);
}

int BitSet::nextPort() {
    unsigned u = 0;
    int port = 0;
    bool found = false;
    
    if (2 < m_max_bit) {
        while (!found) {
            u = m_data[m_upto];
            
            if (!((u >> m_bit_pos) & 0x11)) {
                port = (m_upto << 5) + m_bit_pos; 

                u |= (0x11 << m_bit_pos);
                m_data[m_upto] = u;
                m_max_bit -= 2;
                found = true;
            } 

            m_bit_pos += 2;
            if (m_bit_pos >= DEF_MEM_BIT_CNT) {
                m_bit_pos = 0;
                ++m_upto;
                if (m_upto >= m_size) {
                    m_upto = 0;
                }
            }
        }

        return port + m_base;
    } else {
        return -1;
    }
}

void BitSet::recyclePort(int port) {
    int n = 0;
    int cnt = 0;
    int pos = 0;
    unsigned u = 0;

    n = port - m_base;
    n &= ~0x1;
    
    cnt = n >> DEF_MEM_SHIFT_N;
    pos = n & DEF_MEM_MASK;

    u = m_data[cnt];
    u &= ~(0x11 << pos);
    m_data[cnt] = u;
    m_max_bit += 2;
}

