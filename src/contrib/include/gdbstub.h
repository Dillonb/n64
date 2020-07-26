//
// gdbstub version 1.0.3
//
// MIT License
//
// Copyright (c) 2020 Stephen Lane-Walsh
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//

#ifndef GDBSTUB_H
#define GDBSTUB_H

#include <stdint.h>
#include <sys/types.h>

typedef void (*gdbstub_start_t)(void * user_data);
typedef void (*gdbstub_stop_t)(void * user_data);
typedef void (*gdbstub_step_t)(void * user_data);
typedef void (*gdbstub_set_breakpoint_t)(void * user_data, uint32_t address);
typedef void (*gdbstub_clear_breakpoint_t)(void * user_data, uint32_t address);
typedef ssize_t (*gdbstub_get_memory_t)(void * user_data, char * buffer, size_t buffer_length, uint32_t address, size_t length);
typedef ssize_t (*gdbstub_get_register_value_t)(void * user_data, char * buffer, size_t buffer_length, int reg);
typedef ssize_t (*gdbstub_get_general_registers_t)(void * user_data, char * buffer, size_t buffer_length);

typedef struct gdbstub_config gdbstub_config_t;

struct gdbstub_config
{
    uint16_t port;

    void * user_data;

    gdbstub_start_t start;

    gdbstub_stop_t stop;

    gdbstub_step_t step;

    gdbstub_set_breakpoint_t set_breakpoint;

    gdbstub_clear_breakpoint_t clear_breakpoint;

    gdbstub_get_memory_t get_memory;
    
    gdbstub_get_register_value_t get_register_value;

    gdbstub_get_general_registers_t get_general_registers;

    const char * target_config;
    size_t target_config_length;

    const char * memory_map;
    size_t memory_map_length;
};

typedef struct gdbstub gdbstub_t;

gdbstub_t * gdbstub_init(gdbstub_config_t config);

void gdbstub_term(gdbstub_t * gdb);

void gdbstub_tick(gdbstub_t * gdb);

#endif // GDBSTUB_H

#ifdef GDBSTUB_IMPLEMENTATION

#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(WIN32)
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment (lib, "Ws2_32.lib")
#define close closesocket
#else
#include <unistd.h>
#endif

#define GDBSTUB_BUFFER_LENGTH 4096
#define GDBSTUB_PACKET_LENGTH 2048

typedef enum
{
    GDB_STATE_NO_PACKET,
    GDB_STATE_IN_PACKET,
    GDB_STATE_IN_CHECKSUM,

} gdbstate_t;

struct gdbstub
{
    gdbstub_config_t config;

    int server;
    int client;

    char buffer[GDBSTUB_BUFFER_LENGTH];
    ssize_t buffer_length;

    gdbstate_t state;
    char packet[GDBSTUB_BUFFER_LENGTH];
    ssize_t packet_length;
    uint8_t packet_checksum;

    char checksum[2];
    ssize_t checksum_length;

};

void _gdbstub_recv(gdbstub_t * gdb);

void _gdbstub_send(gdbstub_t * gdb, const char * data, size_t data_length);

void _gdbstub_send_paged(gdbstub_t * gdb, int offset, int length, const char * data, size_t data_length);

void _gdbstub_process_packet(gdbstub_t * gdb);

gdbstub_t * gdbstub_init(gdbstub_config_t config)
{
    gdbstub_t * gdb = (gdbstub_t *)malloc(sizeof(gdbstub_t));
    if (!gdb) {
        fprintf(stderr, "out of memory\n");
        return NULL;
    }

    gdb->config = config;
    gdb->server = -1;
    gdb->client = -1;
    gdb->state = GDB_STATE_NO_PACKET;
    gdb->packet_length = 0;
    gdb->packet_checksum = 0;
    gdb->checksum_length = 0;

    int result;
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(gdb->config.port);
    addr.sin_addr.s_addr = INADDR_ANY;

    gdb->server = socket(AF_INET, SOCK_STREAM, 0);
    if (gdb->server < 0) {
        perror("socket failed");
        free(gdb);
        return NULL;
    }

    result = fcntl(gdb->server, F_SETFL, fcntl(gdb->server, F_GETFL, 0) | O_NONBLOCK);
    if (result < 0) {
        perror("fcntl O_NONBLOCK failed");
        free(gdb);
        return NULL;
    }

    int reuse = 1;
    result = setsockopt(gdb->server, SOL_SOCKET, SO_REUSEADDR, (const char*)&reuse, sizeof(reuse));
    if (result < 0) {
        perror("setsockopt SO_REUSEADDR failed");
        free(gdb);
        return NULL;
    }

#ifdef SO_REUSEPORT
    result = setsockopt(gdb->server, SOL_SOCKET, SO_REUSEPORT, (const char*)&reuse, sizeof(reuse));
    if (result < 0) {
        perror("setsockopt SO_REUSEPORT failed");
        free(gdb);
        return NULL;
    }
#endif

    result = bind(gdb->server, (struct sockaddr *)&addr, sizeof(addr));
    if (result < 0) {
        perror("bind failed");
        free(gdb);
        return NULL;
    }

    result = listen(gdb->server, 1);
    if (result < 0) {
        perror("listen failed");
        free(gdb);
        return NULL;
    }

    printf("listening for gdb on port %hu\n", gdb->config.port);
    return gdb;
}

void gdbstub_term(gdbstub_t * gdb)
{
    if (gdb) {
        if (gdb->client >= 0) {
            close(gdb->client);
            gdb->client = -1;
        }

        if (gdb->server >= 0) {
            close(gdb->server);
            gdb->server = -1;
        }

        free(gdb);
    }
}

void gdbstub_tick(gdbstub_t * gdb)
{
    struct sockaddr_in addr;
    socklen_t addrlen = sizeof(addr);

    if (gdb->client < 0) {
        gdb->client = accept(gdb->server, (struct sockaddr *)&addr, &addrlen);
        if (gdb->client >= 0) {
            printf("accepted gdb connection\n");
            fcntl(gdb->client, F_SETFL, fcntl(gdb->client, F_GETFL, 0) | O_NONBLOCK);
        }
    }
    else {
        _gdbstub_recv(gdb);
    }
}

void _gdbstub_send(gdbstub_t * gdb, const char * data, size_t data_length)
{
    uint8_t checksum = 0;
    for (size_t i = 0; i < data_length; ++i) {
        checksum += data[i];
    }
    
    gdb->packet_length = snprintf(gdb->packet, GDBSTUB_PACKET_LENGTH, "$%s#%02x", data, checksum);

    int bytes = send(gdb->client, gdb->packet, gdb->packet_length, 0);

#   if defined(GDBSTUB_DEBUG)
        printf("gdbstub sent '%s'\n", gdb->packet);
#   endif

    if (bytes < 0) {
        perror("lost gdb connection");
        close(gdb->client);
        gdb->client = -1;
        gdb->state = GDB_STATE_NO_PACKET;
    }
}

void _gdbstub_send_paged(gdbstub_t * gdb, int offset, int length, const char * data, size_t data_length)
{
    if (length > GDBSTUB_PACKET_LENGTH - 6) {
        length = GDBSTUB_PACKET_LENGTH - 6;
    }

    if (length < (data_length - offset)) {
        // Any page but the last
        gdb->buffer_length = snprintf(gdb->buffer, GDBSTUB_BUFFER_LENGTH, "m%.*s", length, data + offset);
        _gdbstub_send(gdb, gdb->buffer, gdb->buffer_length);
    }
    else if (offset != 0) {
        // The last page
        gdb->buffer_length = snprintf(gdb->buffer, GDBSTUB_BUFFER_LENGTH, "l%.*s", length, data + offset);
        _gdbstub_send(gdb, gdb->buffer, gdb->buffer_length);
    }
    else {
        // The only page
        gdb->buffer_length = snprintf(gdb->buffer, GDBSTUB_BUFFER_LENGTH, "l%s", data);
        _gdbstub_send(gdb, gdb->buffer, gdb->buffer_length);
    }
}

void _gdbstub_recv(gdbstub_t * gdb)
{
    gdb->buffer_length = recv(gdb->client, gdb->buffer, GDBSTUB_BUFFER_LENGTH, 0);
    if (gdb->buffer_length < 0 && errno != EAGAIN) {
        perror("lost gdb connection");
        close(gdb->client);
        gdb->client = -1;
        gdb->state = GDB_STATE_NO_PACKET;
        return;
    }

    for (ssize_t i = 0; i < gdb->buffer_length; ++i) {
        char c = gdb->buffer[i];

        switch (gdb->state)
        {
        case GDB_STATE_NO_PACKET:
            if (c == '$') {
                gdb->state = GDB_STATE_IN_PACKET;
                gdb->packet_length = 0;
                gdb->packet_checksum = 0;
            }
            else if (c == 3) {
                // TODO: investigate
            }
            break;
        case GDB_STATE_IN_PACKET:
            if (c == '#') {
                gdb->state = GDB_STATE_IN_CHECKSUM;
                gdb->checksum_length = 0;
            }
            else {
                gdb->packet[gdb->packet_length++] = c;
                gdb->packet_checksum += c;
            }
            break;
        case GDB_STATE_IN_CHECKSUM:
            gdb->checksum[gdb->checksum_length++] = c;
            if (gdb->checksum_length == 2) {
                int checksum;
                sscanf(gdb->checksum, "%2x", &checksum);
                if (gdb->packet_checksum != checksum) {
                    // freak out?
                }

                send(gdb->client, "+", 1, 0);

                gdb->packet[gdb->packet_length] = '\0';
#               if defined(GDBSTUB_DEBUG)
                    printf("gdbstub received '$%s#%c%c'\n", gdb->packet, gdb->checksum[0], gdb->checksum[1]);
#                endif

                _gdbstub_process_packet(gdb);

                gdb->state = GDB_STATE_NO_PACKET;
            }
        }
    }
}

void _gdbstub_process_packet(gdbstub_t * gdb)
{
    switch (gdb->packet[0])
    {
    case 'c':
        // Continue execution
        if (gdb->config.start) {
            gdb->config.start(gdb->config.user_data);
        }
        return;
    case 'D':
        // Disconnect
        printf("gdb disconnected\n");
        _gdbstub_send(gdb, "OK", 2);
        close(gdb->client);
        gdb->client = -1;
        gdb->state = GDB_STATE_NO_PACKET;
        return;
    case 'g':
        // Get general registers
        if (gdb->config.get_general_registers) {
            gdb->buffer_length = gdb->config.get_general_registers(gdb->config.user_data, gdb->buffer, GDBSTUB_BUFFER_LENGTH);
            _gdbstub_send(gdb, gdb->buffer, gdb->buffer_length);
        }
        else {
            _gdbstub_send(gdb, "", 0);
        }
        return;
    case 'H':
        // Set active thread
        _gdbstub_send(gdb, "OK", 2);
        return;
    case 'm':
        // Read memory
        if (gdb->config.get_memory) {
            int address, length;
            sscanf(gdb->packet + 1, "%x,%x", &address, &length);
            gdb->buffer_length = gdb->config.get_memory(gdb->config.user_data, gdb->buffer, GDBSTUB_BUFFER_LENGTH, address, length);
            _gdbstub_send(gdb, gdb->buffer, gdb->buffer_length);
        }
        else {
            _gdbstub_send(gdb, "", 0);
        }
        return;
    case 'p':
        // Read the value of register n
        if (gdb->config.get_register_value) {
            int reg;
            sscanf(gdb->packet + 1, "%x", &reg);
            gdb->buffer_length = gdb->config.get_register_value(gdb->config.user_data, gdb->buffer, GDBSTUB_BUFFER_LENGTH, reg);
            _gdbstub_send(gdb, gdb->buffer, gdb->buffer_length);
        }
        else {
            _gdbstub_send(gdb, "", 0);
        }
        return;
    case 'q':
        // Check for available features
        if (strncmp(gdb->packet, "qSupported", 10) == 0) {
            strcpy(gdb->buffer, "PacketSize=1024");
            gdb->buffer_length = 15;

            if (gdb->config.target_config) {
                strcpy(gdb->buffer + gdb->buffer_length, ";qXfer:features:read+");
                gdb->buffer_length += 21;
            }

            if (gdb->config.memory_map) {
                strcpy(gdb->buffer + gdb->buffer_length, ";qXfer:memory-map:read+");
                gdb->buffer_length += 23;
            }

            _gdbstub_send(gdb, gdb->buffer, gdb->buffer_length);
            return;
        }
        // We have no thread ID
        else if (gdb->packet[1] == 'C') {
            _gdbstub_send(gdb, "QC00", 4);
            return;
        }
        // We are always "attached" to an existing process
        else if (strncmp(gdb->packet, "qAttached", 9) == 0) {
            _gdbstub_send(gdb, "1", 1);
            return;
        }
        // There is no trace running
        else if (strncmp(gdb->packet, "qTStatus", 8) == 0) {
            _gdbstub_send(gdb, "T0", 2);
            return;
        }
        // Target configuration XML
        else if (strncmp(gdb->packet, "qXfer:features:read:target.xml:", 31) == 0) {
            int offset, length;
            sscanf(gdb->packet + 31, "%x,%x", &offset, &length);
            _gdbstub_send_paged(gdb, offset, length, gdb->config.target_config, gdb->config.target_config_length);
            return;
        }
        // Memory map XML
        else if (strncmp(gdb->packet, "qXfer:memory-map:read::", 23) == 0) {
            int offset, length;
            sscanf(gdb->packet + 23, "%x,%x", &offset, &length);
            _gdbstub_send_paged(gdb, offset, length, gdb->config.memory_map, gdb->config.memory_map_length);
            return;
        }
        // Trace control operations
        else if (strncmp(gdb->packet, "qTfP", 4) == 0) {
            _gdbstub_send(gdb, "", 0);
            return;
        }
        else if (strncmp(gdb->packet, "qTfV", 4) == 0) {
            _gdbstub_send(gdb, "", 0);
            return;
        }
        else if (strncmp(gdb->packet, "qTsP", 4) == 0) {
            _gdbstub_send(gdb, "", 0);
            return;
        }
        // Thread #0
        else if (strncmp(gdb->packet, "qfThreadInfo", 12) == 0) {
            _gdbstub_send(gdb, "lm0", 3);
            return;
        }
        break;
    case 's':
        // Single execution step
        if (gdb->config.step) {
            gdb->config.step(gdb->config.user_data);
        }
        _gdbstub_send(gdb, "OK", 2);
        return;
    case 'v':
        // Various remote operations, not supported
        _gdbstub_send(gdb, "", 0);
        return;
    case 'z':
        // Remove breakpoint
        if (gdb->config.clear_breakpoint) {
            uint32_t address;
            sscanf(gdb->packet, "z0,%x", &address);
            gdb->config.clear_breakpoint(gdb->config.user_data, address);
        }
        _gdbstub_send(gdb, "OK", 2);
        break;
    case 'Z':
        // Add breakpoint
        if (gdb->config.set_breakpoint) {
            uint32_t address;
            sscanf(gdb->packet, "Z0,%x", &address);
            gdb->config.set_breakpoint(gdb->config.user_data, address);
        }
        _gdbstub_send(gdb, "OK", 2);
        break;
    case '?':
        // Break immediately
        if (gdb->config.stop) {
            gdb->config.stop(gdb->config.user_data);
        }
        // TODO: improve
        _gdbstub_send(gdb, "S00", 3);
        return;
    }

    fprintf(stderr, "unknown gdb command '%s'\n", gdb->packet);
}

#endif // GDBSTUB_IMPLEMENTATION