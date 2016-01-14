// vim:ts=4:sw=4:expandtab
#include "all.h"
#include <getopt.h>

#ifndef __VERSION
#define __VERSION "unknown"
#endif

xcb_connection_t *connection;
xcb_window_t root;

/* Forward declarations */
static void at_exit_cb(void);
static void parse_args(int argc, char *argv[]);
static void print_usage(void);

int main(int argc, char *argv[]) {
    atexit(at_exit_cb);
    parse_args(argc, argv);

    int screen;
    connection = xcb_connect(NULL, &screen);
    if (connection == NULL || xcb_connection_has_error(connection)) {
        fprintf(stderr, "Could not connect to X.\n");
        exit(EXIT_FAILURE);
    }

    xcb_screen_t *root_screen = xcb_aux_get_screen(connection, screen);
    root = root_screen->root;

    /* Intern all atoms we require. */
#define xmacro(name)                                                                               \
    xcb_intern_atom_cookie_t name##_cookie = xcb_intern_atom(connection, 0, strlen(#name), #name); \
    do {                                                                                           \
        xcb_intern_atom_reply_t *reply = xcb_intern_atom_reply(connection, name##_cookie, NULL);   \
        if (reply == NULL) {                                                                       \
            fprintf(stderr, "Failed to intern atom.\n");                                           \
            exit(EXIT_FAILURE);                                                                    \
        }                                                                                          \
        A_##name = reply->atom;                                                                    \
        free(reply);                                                                               \
    } while (0);

    xmacro(_NET_WM_DESKTOP);
#undef xmacro

    message_send_net_wm_desktop(XCB_NONE, 3, SI_NORMAL);
}

static void at_exit_cb(void) {
    if (connection != NULL) {
        xcb_disconnect(connection);
    }
}

static void parse_args(int argc, char *argv[]) {
    int c,
        opt_index = 0;
    static struct option long_options[] = {
        { "version", no_argument, 0, 'v' },
        { "help", no_argument, 0, 'h' },
        { 0, 0, 0, 0}
    };

    while ((c = getopt_long(argc, argv, "vh", long_options, &opt_index)) != -1) {
        switch (c) {
            case 0:
                /* Example for a long-named option.
                if (strcmp(long_options[opt_index].name, "parameter") == 0) {
                    break;
                }
                */

                print_usage();
                break;
            case 'v':
                fprintf(stderr, "xcmsend version %s\n", __VERSION);
                exit(EXIT_SUCCESS);
                break;
            case 'h':
            default:
                print_usage();
                break;
        }
    }
}

static void print_usage(void) {
    fprintf(stderr, "Usage: xcmsend [-v|--version] [-h|--help]");
    fprintf(stderr, "\n");
    exit(EXIT_FAILURE);
}
