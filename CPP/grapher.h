#ifndef GRAPHER_H
#define GRAPHER_H

#include "../C/ranks.h"

#ifdef __cplusplus
extern "C" {
#else
#endif

void convert_and_plot(SymbolToRanksHashMap* all);

#ifdef __cplusplus
}
#endif

#endif //GRAPHER_H
