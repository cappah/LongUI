﻿// web color name to argb32
#include <stdint.h>
#include <assert.h>
#include <util/ui_unimacro.h>

PCN_NOINLINE
/// <summary>
/// hash ignore case. ansi char supported only
/// </summary>
/// <param name="begin">The begin.</param>
/// <param name="end">The end.</param>
/// <param name="step">The step.</param>
/// <returns></returns>
uint32_t ui_hash_ignore_case(const char* begin, const char* end, char step) {
    assert((end - begin) % step == 0 && "bad string");
    const uint32_t seed = 131;
    uint32_t hash = 0;
    while (begin != end) {
        hash = hash * seed + (((*begin) & 0x1f) + 0x40);
        begin += step;
    }
    return hash;
}

// color hash
struct ct { uint32_t hash; uint8_t r, g, b, x; };
// color hash
struct tt { struct ct a, b, c; };

// bstable
static const struct ct ui_hash_color_table[] = {
    { 0x00159c75, 0xff, 0x00, 0x00, 0 }, { 0x00162085, 0xd2, 0xb4, 0x8c, 0 },
    { 0x00e0fc23, 0xf5, 0xde, 0xb3, 0 }, { 0x00e211f1, 0xff, 0xff, 0xff, 0 },
    { 0x016295b9, 0xff, 0xf5, 0xee, 0 }, { 0x06164ca6, 0xda, 0xa5, 0x20, 0 },
    { 0x067fa0ae, 0x55, 0x6b, 0x2f, 0 }, { 0x08cb15f4, 0x00, 0xff, 0xff, 0 },
    { 0x08ec1466, 0x00, 0x00, 0xff, 0 }, { 0x0911bf43, 0x00, 0xff, 0xff, 0 },
    { 0x0963e047, 0x69, 0x69, 0x69, 0 }, { 0x09985cec, 0xff, 0xd7, 0x00, 0 },
    { 0x0999207b, 0x80, 0x80, 0x80, 0 }, { 0x0a424f41, 0x00, 0xff, 0x00, 0 }, 
    { 0x0a84d6de, 0x00, 0x00, 0x80, 0 }, { 0x0aca7c28, 0xcd, 0x85, 0x3f, 0 },
    { 0x0acb8636, 0xff, 0xc0, 0xcb, 0 }, { 0x0acc52e8, 0xdd, 0xa0, 0xdd, 0 },
    { 0x0b231d4e, 0xfa, 0x80, 0x72, 0 }, { 0x0b33bec3, 0xff, 0xfa, 0xfa, 0 },
    { 0x0b53a9d8, 0x00, 0x80, 0x80, 0 }, { 0x0bc722bd, 0x46, 0x82, 0xb4, 0 },
    { 0x0d03e941, 0xff, 0xff, 0xf0, 0 }, { 0x0e254c7d, 0x3c, 0xb3, 0x71, 0 },
    { 0x0e9fcac8, 0xfa, 0xfa, 0xd2, 0 }, { 0x146ea803, 0xd8, 0xbf, 0xd8, 0 },
    { 0x1752ac68, 0xff, 0x8c, 0x00, 0 }, { 0x1795b66d, 0x99, 0x32, 0xcc, 0 },
    { 0x1b16efda, 0x4b, 0x00, 0x82, 0 }, { 0x1df11bb1, 0xff, 0x00, 0xff, 0 },
    { 0x213ae781, 0xff, 0xe4, 0xb5, 0 }, { 0x22491d7a, 0xfd, 0xf5, 0xe6, 0 },
    { 0x2343e297, 0xf0, 0x80, 0x80, 0 }, { 0x234ec770, 0xad, 0xd8, 0xe6, 0 },
    { 0x2374724d, 0xe0, 0xff, 0xff, 0 }, { 0x23fbd385, 0xd3, 0xd3, 0xd3, 0 },
    { 0x24342b57, 0xb2, 0x22, 0x22, 0 }, { 0x252e3940, 0xff, 0xb6, 0xc1, 0 },
    { 0x26d7b050, 0x80, 0x00, 0x00, 0 }, { 0x27bc78a2, 0xdc, 0xdc, 0xdc, 0 },
    { 0x2957a7d4, 0xba, 0x55, 0xd3, 0 }, { 0x2a618a0c, 0xee, 0xe8, 0xaa, 0 },
    { 0x2e3b6946, 0xf0, 0xe6, 0x8c, 0 }, { 0x2fdc90d8, 0xd2, 0x69, 0x1e, 0 },
    { 0x30150e68, 0x2e, 0x8b, 0x57, 0 }, { 0x333c2b69, 0x40, 0xe0, 0xd0, 0 },
    { 0x36737844, 0xff, 0xff, 0x00, 0 }, { 0x3a3f6783, 0x8b, 0x00, 0x8b, 0 },
    { 0x3c47c2e9, 0x48, 0x3d, 0x8b, 0 }, { 0x3cf4cefe, 0x2f, 0x4f, 0x4f, 0 },
    { 0x3d5a764f, 0xff, 0x69, 0xb4, 0 }, { 0x3feed19a, 0xfa, 0xf0, 0xe6, 0 },

    // rebeccapurple
    { 0x455fd72b, 0x66, 0x33, 0x99, 0 },

    { 0x4601f422, 0xff, 0xa5, 0x00, 0 }, { 0x4644fe27, 0xda, 0x70, 0xd6, 0 },
    { 0x48e0676d, 0x1e, 0x90, 0xff, 0 }, { 0x4f0ea139, 0xc7, 0x15, 0x85, 0 },
    { 0x55a3fc2d, 0xff, 0xf0, 0xf5, 0 }, { 0x56313dfc, 0x00, 0x00, 0x8b, 0 },
    { 0x5656e8d9, 0x00, 0x8b, 0x8b, 0 }, { 0x56de4a11, 0xa9, 0xa9, 0xa9, 0 },
    { 0x57040f9a, 0x00, 0xff, 0x7f, 0 }, { 0x578768cf, 0xaf, 0xee, 0xee, 0 },
    { 0x5b89bc21, 0x93, 0x70, 0xdb, 0 }, { 0x5bd50b25, 0x94, 0x00, 0xd3, 0 },
    { 0x61fee746, 0x7b, 0x68, 0xee, 0 }, { 0x64ee7562, 0x7f, 0xff, 0xd4, 0 },
    { 0x6636a70b, 0xff, 0x45, 0x00, 0 }, { 0x68db189b, 0x87, 0xce, 0xeb, 0 },
    { 0x69de3f45, 0x90, 0xee, 0x90, 0 }, { 0x6f03de15, 0xf4, 0xa4, 0x60, 0 },
    { 0x73c0231f, 0x66, 0xcd, 0xaa, 0 }, { 0x73c0e8e9, 0x00, 0x64, 0x00, 0 },
    { 0x74fd96db, 0x80, 0x80, 0x00, 0 }, { 0x75e09e8b, 0x5f, 0x9e, 0xa0, 0 },
    { 0x78700c6f, 0xbc, 0x8f, 0x8f, 0 }, { 0x78771274, 0x80, 0x00, 0x80, 0 },
    { 0x7d4d1b55, 0x64, 0x95, 0xed, 0 }, { 0x7f46c0ad, 0x98, 0xfb, 0x98, 0 },
    { 0x8120ff47, 0xf0, 0xff, 0xff, 0 }, { 0x8152552b, 0x8b, 0x45, 0x13, 0 },
    { 0x82aeed08, 0xb8, 0x86, 0x0b, 0 }, { 0x858f4ca7, 0x8b, 0x00, 0x00, 0 },
    { 0x8a46dffd, 0xff, 0xfa, 0xf0, 0 }, { 0x8a8452df, 0xee, 0x82, 0xee, 0 },
    { 0x8aa4baeb, 0x00, 0x00, 0xcd, 0 }, { 0x8ae2465a, 0x32, 0xcd, 0x32, 0 },
    { 0x8d9616d7, 0xff, 0x00, 0xff, 0 }, { 0x8fdb2dd4, 0xf5, 0xf5, 0xdc, 0 },
    { 0x90c932c3, 0x00, 0x00, 0x00, 0 }, { 0x919ab922, 0xa5, 0x2a, 0x2a, 0 },
    { 0x94a8af53, 0xad, 0xff, 0x2f, 0 }, { 0x96a0f312, 0xa0, 0x52, 0x2d, 0 },
    { 0x9793260d, 0xc0, 0xc0, 0xc0, 0 }, { 0x97b7a913, 0xe6, 0xe6, 0xfa, 0 },
    { 0xa2c24479, 0xff, 0x7f, 0x50, 0 }, { 0xa44e8a1a, 0x77, 0x88, 0x99, 0 },
    { 0xac25dade, 0x8f, 0xbc, 0x8f, 0 }, { 0xaf67d42e, 0x19, 0x19, 0x70, 0 },
    { 0xafd4cbcb, 0x00, 0xce, 0xd1, 0 }, { 0xb34f97c6, 0xff, 0xef, 0xd5, 0 },
    { 0xb375c3c6, 0xff, 0xda, 0xb9, 0 }, { 0xb53e21aa, 0xff, 0xde, 0xad, 0 },
    { 0xb6550387, 0x7c, 0xfc, 0x00, 0 }, { 0xb89fb108, 0xbd, 0xb7, 0x6b, 0 },
    { 0xb9ffb48a, 0x22, 0x8b, 0x22, 0 }, { 0xbce7d9ee, 0xff, 0x14, 0x93, 0 },
    { 0xbe9cd3f2, 0xcd, 0x5c, 0x5c, 0 }, { 0xbfaf2287, 0x6a, 0x5a, 0xcd, 0 },
    { 0xc05c2e9c, 0x70, 0x80, 0x90, 0 }, { 0xc13c28db, 0xf0, 0xff, 0xf0, 0 },
    { 0xc28cf6b0, 0xff, 0xfa, 0xcd, 0 }, { 0xc3fbf62a, 0xf8, 0xf8, 0xff, 0 },
    { 0xcd7706a8, 0xff, 0xa0, 0x7a, 0 }, { 0xcd9c1c0d, 0x41, 0x69, 0xe1, 0 },
    { 0xcdbaa31d, 0xde, 0xb8, 0x87, 0 }, { 0xcf2fb840, 0xf5, 0xff, 0xfa, 0 },
    { 0xd10a19e0, 0xdb, 0x70, 0x93, 0 }, { 0xd2f3dd4a, 0xff, 0xeb, 0xcd, 0 },
    { 0xd58bf028, 0x48, 0xd1, 0xcc, 0 }, { 0xd8dd15fb, 0xff, 0xe4, 0xe1, 0 },
    { 0xd9cb81a9, 0x87, 0xce, 0xfa, 0 }, { 0xdc73d594, 0xe9, 0x96, 0x7a, 0 },
    { 0xe0174796, 0xfa, 0xeb, 0xd7, 0 }, { 0xe035ffae, 0x7f, 0xff, 0x00, 0 },
    { 0xe28793b3, 0x9a, 0xcd, 0x32, 0 }, { 0xe3a02103, 0x00, 0xbf, 0xff, 0 },
    { 0xe4b8fb9d, 0xff, 0xe4, 0xc4, 0 }, { 0xe6ef2dff, 0xdc, 0x14, 0x3c, 0 },
    { 0xe76bf492, 0xf5, 0xf5, 0xf5, 0 }, { 0xe95ca127, 0x00, 0x80, 0x00, 0 },
    { 0xec4ff751, 0x00, 0xfa, 0x9a, 0 }, { 0xee44195a, 0xf0, 0xf8, 0xff, 0 },
    { 0xefb97e3b, 0xb0, 0xc4, 0xde, 0 }, { 0xf1e75f55, 0x8a, 0x2b, 0xe2, 0 },
    { 0xf5656557, 0xb0, 0xe0, 0xe6, 0 }, { 0xf77f805d, 0xff, 0xf8, 0xdc, 0 },
    { 0xf84da4de, 0x6b, 0x8e, 0x23, 0 }, { 0xf8c7619e, 0xff, 0xff, 0xe0, 0 },
    { 0xfb1ad092, 0x20, 0xb2, 0xaa, 0 }, { 0xfc868cee, 0xff, 0x63, 0x47, 0 },
};

PCN_NOINLINE
/// <summary>
/// UIs the web color name to rgb32(RGBX byte order).
/// </summary>
/// <param name="begin">The begin.</param>
/// <param name="end">The end.</param>
/// <param name="step">The step.</param>
/// <returns></returns>
uint32_t ui_web_color_name_hash_to_rgb32(uint32_t hash) {
    // 二分搜索
    int left = 0, right = sizeof(ui_hash_color_table) / sizeof(ui_hash_color_table[0]);
    while (left < right) {
        const int mid = (left + right) / 2;
        if (ui_hash_color_table[mid].hash < hash) left = mid + 1;
        else if (ui_hash_color_table[mid].hash > hash) right = mid;
        else return *(const uint32_t*)&ui_hash_color_table[mid].r;
    }
    //assert(!"unkown color name");
    return 0;
}


PCN_NOINLINE
/// <summary>
/// UIs the web color name to rgb32(RGBX byte order).
/// </summary>
/// <param name="begin">The begin.</param>
/// <param name="end">The end.</param>
/// <param name="step">The step.</param>
/// <returns></returns>
uint32_t ui_web_color_name_to_rgb32(const char* begin, const char* end, char step) {
    const uint32_t hash = ui_hash_ignore_case(begin, end, step);
    return ui_web_color_name_hash_to_rgb32(hash);
}
