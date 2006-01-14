/*
 * uoproxy
 * $Id$
 *
 * (c) 2005 Max Kellermann <max@duempel.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; version 2 of the License.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#include <assert.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/wait.h>
#include <sys/signal.h>
#include <signal.h>
#include <netinet/in.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <netdb.h>
#include <time.h>

#include "netutil.h"
#include "ioutil.h"
#include "connection.h"
#include "config.h"
#include "instance.h"
#include "version.h"

static int should_exit = 0;

static void exit_signal_handler(int sig) {
    (void)sig;
    should_exit = 1;
}

static void config_get(struct config *config, int argc, char **argv) {
    const char *home;
    char path[4096];
    int ret;

    memset(config, 0, sizeof(*config));
    config->autoreconnect = 1;

    home = getenv("HOME");
    if (home == NULL) {
        ret = 1;
    } else {
        snprintf(path, sizeof(path), "%s/.uoproxyrc", home);
        ret = config_read_file(config, path);
    }

    if (ret != 0)
        config_read_file(config, "/etc/uoproxy.conf");

    parse_cmdline(config, argc, argv);
}

static void delete_all_connections(struct connection *head) {
    while (head != NULL) {
        struct connection *c = head;
        head = head->next;
        connection_delete(c);
    }
}

static void run_server(struct instance *instance) {
    int ret;
    struct selectx sx;

    instance->tv = (struct timeval){
        .tv_sec = 30,
        .tv_usec = 0,
    };

    while (!should_exit) {
        selectx_clear(&sx);
        selectx_add_read(&sx, instance->server_socket);
        instance_pre_select(instance, &sx);

        ret = selectx(&sx, &instance->tv);
        if (ret == 0) {
            instance->tv = (struct timeval){
                .tv_sec = 30,
                .tv_usec = 0,
            };

            instance_idle(instance, time(NULL));
        } else if (ret > 0) {
            if (FD_ISSET(instance->server_socket, &sx.readfds)) {
                int sockfd2;
                struct sockaddr addr;
                socklen_t addrlen = sizeof(addr);

                sockfd2 = accept(instance->server_socket, &addr, &addrlen);
                if (sockfd2 >= 0) {
                    struct connection *c;

                    ret = connection_new(instance,
                                         sockfd2,
                                         &c);
                    if (ret == 0) {
                        c->next = instance->connections_head;
                        instance->connections_head = c;
                    } else {
                        fprintf(stderr, "connection_new() failed: %s\n",
                                strerror(-ret));
                        close(sockfd2);
                    }
                } else if (errno != EINTR) {
                    fprintf(stderr, "accept failed: %s\n",
                            strerror(errno));
                    exit(1);
                }
            }

            instance_post_select(instance, &sx);
        } else if (errno != EINTR) {
            fprintf(stderr, "select failed: %s\n",
                    strerror(errno));
            exit(1);
        }
    }
}

static void setup_signal_handlers(void) {
    struct sigaction sa;

    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = exit_signal_handler;

    sigaction(SIGTERM, &sa, NULL);
    sigaction(SIGINT, &sa, NULL);
}

int main(int argc, char **argv) {
    struct config config;
    struct instance instance = {
        .config = &config,
        .connections_head = NULL,
    };

    /* configuration */

    config_get(&config, argc, argv);

    if (verbose >= 1)
        printf("uoproxy v" VERSION
               ", http://max.kellermann.name/projects/uoproxy/\n");

    /* set up */

    setup_signal_handlers();

    instance.server_socket = setup_server_socket(instance.config->bind_address);

    instance_daemonize(&instance);

    /* call main loop */

    run_server(&instance);

    /* cleanup */

    delete_all_connections(instance.connections_head);

    config_dispose(&config);

    return 0;
}
