#ifndef CONFIG_H
#define CONFIG_H

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#ifdef _WIN32
#define RAW_DATA_FOLDER "C:/Users/aljoh/CLionProjects/Wavelet/data/raw/"
#define INTERMEDIATE_DATA_FOLDER "C:/Users/aljoh/CLionProjects/Wavelet/data/intermediate/"
#else
#define RAW_DATA_FOLDER "/home/main/CLionProjects/SPS2024/data/raw"
#define INTERMEDIATE_DATA_FOLDER "/home/main/CLionProjects/SPS2024/data/intermediate"
#endif // _WIN32


#define IS_PARALLEL 1
#endif //CONFIG_H

#define NUMBER_OF_STOCK_EXAMPLES 11356