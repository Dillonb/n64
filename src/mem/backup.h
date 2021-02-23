#ifndef N64_BACKUP_H
#define N64_BACKUP_H
#include <system/n64system.h>
#include "mem_util.h"

void backup_write_word(word index, word value);
word backup_read_word(word index);

void backup_write_byte(word index, byte value);
byte backup_read_byte(word index);

size_t get_save_size(n64_save_type_t save_type);
void init_savedata(n64_mem_t* mem, const char* rom_path);
void init_mempack(n64_mem_t* mem, const char* rom_path);

void persist_backup();
void force_persist_backup();

#endif //N64_BACKUP_H
