#include "game_db.h"

static const gamedb_entry_t gamedb[] = {
        {"NNM", "E", SAVE_NONE,        "Namco Museum 64"},
        {"NDM", "E", SAVE_NONE,        "Doom 64"},
        {"NPD", "E", SAVE_EEPROM_16k,  "Perfect Dark"},
        {"NKT", "E", SAVE_EEPROM_4k,   "Mario Kart 64"},
        {"CZL", "E", SAVE_EEPROM_256k, "The Legend of Zelda: Ocarina of Time"},
        {"NSM", "E", SAVE_EEPROM_4k,   "Super Mario 64"},
        {"NPW", "E", SAVE_EEPROM_4k,   "Pilotwings 64"},
        {"NEP", "E", SAVE_EEPROM_16k,  "Star Wars Episode I Racer"},
        {"NDY", "E", SAVE_EEPROM_4k,   "Diddy Kong Racing"},
        {"NW2", "E", SAVE_EEPROM_256k, "WCW/NWO Revenge"},
        {"NSQ", "E", SAVE_EEPROM_1Mb,  "StarCraft 64"},
        {"NK4", "P", SAVE_EEPROM_4k,   "Kirby 64: The Crystal Shards"},
        {"NGE", "E", SAVE_EEPROM_4k,   "GoldenEye 007"},
        {"NZS", "E", SAVE_EEPROM_1Mb,  "The Legend of Zelda: Majora's Mask"},
        {"NJF", "E", SAVE_EEPROM_1Mb,  "Jet Force Gemini"},
        {"NMQ", "E", SAVE_EEPROM_1Mb,  "Paper Mario"},
        {"NPO", "E", SAVE_EEPROM_1Mb,  "Pokémon Stadium"},
        {"NP3", "E", SAVE_EEPROM_1Mb,  "Pokémon Stadium 2"},
        {"CFZ", "E", SAVE_EEPROM_256k, "F-Zero X"},
        {"NK4", "E", SAVE_EEPROM_4k,   "Kirby 64: The Crystal Shards"},
        {"NAL", "E", SAVE_EEPROM_256k, "Super Smash Bros"},
        {"NBM", "E", SAVE_EEPROM_4k,   "Bomberman 64"},
        {"NWR", "E", SAVE_EEPROM_4k,   "Wave Race 64"},
        {"NBK", "E", SAVE_EEPROM_4k,   "Banjo-Kazooie"},
        {"NPF", "E", SAVE_EEPROM_1Mb,  "Pokemon Snap"},
        {"NFX", "E", SAVE_EEPROM_4k,   "Starfox 64"}
};

#define GAMEDB_SIZE (sizeof(gamedb) / sizeof(gamedb_entry_t))

void gamedb_match(n64_system_t* system) {
    n64_rom_t* rom = &system->mem.rom;
    for (int i = 0; i < GAMEDB_SIZE; i++) {
        bool matches_code = strcmp(gamedb[i].code, rom->code) == 0;
        bool matches_region = false;

        for (int j = 0; j < strlen(gamedb[i].regions) && !matches_region; j++) {
            if (gamedb[i].regions[j] == rom->header.country_code[0]) {
                matches_region = true;
            }
        }

        if (matches_code) {
            if (matches_region) {
                system->mem.save_type = gamedb[i].save_type;
                system->mem.rom.game_name_db = gamedb[i].name;
                logalways("Loaded %s", gamedb[i].name);
                return;
            } else {
                logwarn("Matched code for %s, but not region! Game supposedly exists in regions [%s] but this image has region %c",
                        gamedb[i].name, gamedb[i].regions, rom->header.country_code[0]);
            }
        }

    }

    logalways("Did not match any Game DB entries. Code: %s Region: %c", system->mem.rom.code, system->mem.rom.header.country_code[0]);

    system->mem.rom.game_name_db = NULL;
    system->mem.save_type = SAVE_NONE;
}
