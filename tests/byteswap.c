#include <stdint.h>
#include <assert.h>

#include "../src/multiplexers.h"


int main(void)
{
    uint16_t word = 0xbeef;
    uint32_t dword = 0xdeadbeef;
    uint64_t qword = 0xdeadbeeff00dfeed;
    uint8_t _24bits[3] = {0xde, 0xad, 0xbe};

    _bmo_swap_16(&word);
    assert(word == 0xefbe);

    _bmo_swap_24(_24bits);
    assert(_24bits[0] == 0xbe && _24bits[1] == 0xad && _24bits[2] == 0xde);

    _bmo_swap_32(&dword);
    assert(dword == 0xefbeadde);

    _bmo_swap_64(&qword);
    assert(qword == 0xedfe0df0efbeadde);

    return 0;
}
