#include <sys/socket.h>
#include <err.h>
#include <stdlib.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <stdio.h>

static int gdbstub_server_socket = -1;
static int gdbstub_client_socket = -1;
static struct sockaddr_in server_addr;
static struct sockaddr_in client_addr;
static socklen_t client_addr_len = 0;

uint8_t gdbstub_buf[0x800];

#define GDBSTUB_PORT 1337
#define CONNECTION_BACKLOG 5

void gdbstub_init() {
    gdbstub_server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (gdbstub_server_socket < 0) {
        errx(EXIT_FAILURE, "Unable to open socket!\n");
    }

    if (fcntl(gdbstub_server_socket, F_SETFL, fcntl(gdbstub_server_socket, F_GETFL, 0) | O_NONBLOCK) < 0) {
        errx(EXIT_FAILURE, "Unable to set socket to non-blocking!\n");
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(GDBSTUB_PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(gdbstub_server_socket, (const struct sockaddr*) &server_addr, sizeof(server_addr)) < 0) {
        errx(EXIT_FAILURE, "Unable to bind to 0.0.0.0:%d!", GDBSTUB_PORT);
    }

    if (listen(gdbstub_server_socket, CONNECTION_BACKLOG) < 0) {
        errx(EXIT_FAILURE, "Unable to listen on 0.0.0.0:%d!", GDBSTUB_PORT);
    }
}

void gdbstub_accept() {
    gdbstub_client_socket = accept(gdbstub_server_socket, (struct sockaddr *) &client_addr, &client_addr_len);
    if (gdbstub_client_socket < 0) {
        return;
    }
}

void gdbstub_tick() {
    if (gdbstub_client_socket < 0) {
        gdbstub_accept();
    }

    if (gdbstub_client_socket >= 0) {
        if (recv(gdbstub_client_socket, &gdbstub_buf, sizeof(gdbstub_buf), MSG_DONTWAIT)) {
            printf("They sent us: %s\n", gdbstub_buf);
        }
         // do stuff
    }
}