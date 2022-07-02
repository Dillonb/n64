#!/usr/bin/env python
import json
import sys
import os
import subprocess

with open("compatibility.json") as f:
    compat_list = json.load(f)
total_games = len(compat_list)


def launch_emu(game):
    print("Launching emulator: %s" % game["name"])
    emu_process = subprocess.run(["../build/n64", os.path.join("roms", game["path"])])
    print("%s finished with %d" % (game["name"], emu_process.returncode))


def analyze():
    compatibility = {}

    for game in compat_list:
        if game["compatibility"] not in compatibility.keys():
            compatibility[game["compatibility"]] = 0
        compatibility[game["compatibility"]] += 1

    for compat, count in compatibility.items():
        print("%s: %d/%d games (%0.2f%%)" % (compat, count, total_games, (count / total_games) * 100))


def missing_roms():
    missing_path_count = 0
    missing_file_count = 0
    for game in compat_list:
        if game["path"] == "":
            print("%s missing path" % game["name"])
            missing_path_count += 1
        elif not os.path.exists(os.path.join("roms", game["path"])):
            print("%s has path (%s), but it doesn't exist!" % (game["name"], game["path"]))
            missing_file_count += 1

    print("%d games missing path." % missing_path_count)
    print("%d games missing file." % missing_file_count)


def can_launch(game):
    return game["path"] != "" and os.path.exists(os.path.join("roms", game["path"]))


def test():
    for g in compat_list:
        if not can_launch(g):
            continue
        if g["compatibility"] != "UNTESTED":
            continue
        launch_emu(g)
        return


COMMANDS = {
    "analyze": analyze,
    "missing_roms": missing_roms,
    "test": test
}

if len(sys.argv) < 2 or sys.argv[1] not in COMMANDS.keys():
    print("Usage: %s [command]" % sys.argv[0])
    print("Available commands:")
    for command in COMMANDS.keys():
        print("\t%s" % command)
else:
    COMMANDS[sys.argv[1]]()
