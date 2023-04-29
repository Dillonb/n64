#ifndef N64_TAS_MOVIE_H
#define N64_TAS_MOVIE_H

#include <assert.h>
#include "device.h"

void load_tas_movie(const char* filename);
n64_controller_t tas_next_inputs();
bool tas_movie_loaded();
void start_tas_recording(const char* movie_path);
bool tas_movie_recording();
void tas_record_inputs(n64_controller_t* inputs);
#endif //N64_TAS_MOVIE_H
