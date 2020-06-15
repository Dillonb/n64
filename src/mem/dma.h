#ifndef N64_DMA_H
#define N64_DMA_H

#include <stdbool.h>
#include "../common/util.h"
#include "../system/n64system.h"

bool is_dma_active();
void run_dma(n64_system_t* system, word source, word dest, word length, const char* direction);

#endif //N64_DMA_H
