#ifndef N64_GAME_DB_H
#define N64_GAME_DB_H

#include <mem/n64rom.h>
#include <mem/backup.h>

typedef struct gamedb_entry {
    const char* code;
    const char* regions;
    n64_save_type_t save_type;
    const char* name;
} gamedb_entry_t;

void gamedb_match(n64_system_t* system);

#endif //N64_GAME_DB_H
