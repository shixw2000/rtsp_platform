#ifndef __BITSET_H__
#define __BITSET_H__


class BitSet {
    static const int DEF_MEM_SHIFT_N = 5;
    static const int DEF_MEM_BIT_CNT = (0x1 << DEF_MEM_SHIFT_N);
    static const int DEF_MEM_MASK = DEF_MEM_BIT_CNT -  1;

public:
    BitSet(int base, int maxBit);
    ~BitSet();

    int nextPort();
    void recyclePort(int port);

private:
    unsigned* m_data;
    const int m_base;
    int m_max_bit;
    int m_size;
    int m_last_pos;
    int m_upto;
    int m_bit_pos;
};

#endif

