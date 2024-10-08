#include <iostream>
#include <algorithm>
using namespace std;

namespace esphome {
int main()
{
    uint8_t first[] = {0x31, 0x32, 0x33};
    uint8_t second[] = {0x34, 0x35};

    int f = sizeof(first) / sizeof(*first);
    int s = sizeof(second) / sizeof(*second);

    std::cout << f << ' ' << s << "\n";

    uint8_t result[
    f + s
    ];
    std::copy_n(
        second, s, std::copy_n(
            first, f, result
        )
    );

    for (uint8_t &i: result) {
        std::cout << i << ' ';
    }

    return 0;
}
}