#include "crop_ranges.h"

static const CropRange cropRanges[] = {
    {"apple", 7570, 9100, 2104, 2400, 9003, 9492, 551, 650},
    {"banana", 7860, 9010, 2501, 2991, 7503, 8498, 551, 649},
    {"blackgram", 6420, 7570, 2510, 3495, 6007, 6996, 650, 778},
    {"chickpea", 5810, 7440, 1702, 2100, 1426, 1997, 599, 887},
    {"coconut", 7770, 9690, 2501, 2987, 9002, 9998, 550, 647},
    {"coffee", 7880, 9300, 2306, 2792, 5005, 6995, 602, 749},
    {"cotton", 4980, 7440, 2200, 2599, 7501, 8488, 580, 799},
    {"grapes", 7060, 8100, 883, 4195, 8002, 8398, 551, 650},
    {"jute", 7880, 9440, 2309, 2699, 7088, 8989, 600, 749},
    {"kidneybeans", 6240, 8600, 1533, 2492, 1809, 2497, 550, 600},
    {"lentil", 5630, 6840, 1806, 2994, 6009, 6992, 592, 784},
    {"maize", 5730, 8010, 1804, 2655, 5528, 7483, 551, 700},
    {"mango", 6500, 7810, 2700, 3599, 4502, 5496, 451, 697},
    {"mothbeans", 5790, 7380, 2402, 3200, 4001, 6496, 350, 994},
    {"mungbean", 6160, 7510, 2701, 2991, 8003, 9000, 622, 720},
    {"muskmelon", 6690, 7580, 2702, 2994, 9002, 9496, 600, 678},
    {"orange", 7630, 8990, 1001, 3491, 9001, 9496, 601, 800},
    {"papaya", 6220, 9780, 2301, 4368, 9004, 9494, 650, 699},
    {"pigeonpeas", 6980, 9000, 1832, 3698, 3040, 6969, 455, 745},
    {"pomegranate", 7430, 8870, 1807, 2496, 8513, 9500, 556, 720},
    {"rice", 9340, 9820, 2005, 2693, 8012, 8497, 501, 787},
    {"watermelon", 6650, 7850, 2404, 2699, 8003, 8998, 600, 696},
};

static const int cropCount = sizeof(cropRanges) / sizeof(cropRanges[0]);

const CropRange* getCropRange(const char* crop)
{
    if (crop == 0) return 0;

    for (int i = 0; i < cropCount; ++i)
    {
        const char* a = cropRanges[i].crop;
        const char* b = crop;
        while (*a && *b && *a == *b) { ++a; ++b; }
        if (*a == *b) return &cropRanges[i];
    }

    return 0;
}

const CropRange* getCropRangeByIndex(int index)
{
    if (index < 0 || index >= cropCount) return 0;
    return &cropRanges[index];
}

int getCropCount()
{
    return cropCount;
}
