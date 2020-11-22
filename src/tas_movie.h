#ifndef N64_TAS_MOVIE_H
#define N64_TAS_MOVIE_H

#include <assert.h>
#include <system/n64system.h>

void load_tas_movie(const char* filename);
n64_controller_t tas_next_inputs();
bool tas_movie_loaded();
#endif //N64_TAS_MOVIE_H
