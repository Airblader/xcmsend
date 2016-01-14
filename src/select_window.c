// vim:ts=4:sw=4:expandtab
#include "all.h"

/*
 * Let the user select a window.
 *
 */
xcb_window_t select_window(void) {
    /* We use our own connection because we need to select events on the root window. */
    int screen;
    xcb_connection_t *conn = xcb_connect(NULL, &screen);
    if (conn == NULL || xcb_connection_has_error(conn)) {
        errx(EXIT_FAILURE, "Failed to connect to X.\n");
    }
    xcb_screen_t *root_screen = xcb_aux_get_screen(conn, screen);

    /* Select button events so we know when the user clicks on a window. */
    xcb_change_window_attributes(conn, root, XCB_CW_EVENT_MASK,
            (uint32_t[]) { XCB_EVENT_MASK_BUTTON_PRESS | XCB_EVENT_MASK_BUTTON_RELEASE });

    /* Try to load a crosshair cursor to give the user visual indication. */
    xcb_cursor_context_t *ctx;
    xcb_cursor_t cursor = XCB_NONE;
    if (xcb_cursor_context_new(conn, root_screen, &ctx) >= 0) {
        cursor = xcb_cursor_load_cursor(ctx, "crosshair");
    }

    xcb_grab_pointer_cookie_t cookie = xcb_grab_pointer(conn,
            0, root, XCB_EVENT_MASK_BUTTON_PRESS | XCB_EVENT_MASK_BUTTON_RELEASE,
            XCB_GRAB_MODE_SYNC, XCB_GRAB_MODE_ASYNC, root, cursor, XCB_CURRENT_TIME);

    xcb_cursor_context_free(ctx);
 
    xcb_generic_error_t *error;
    xcb_grab_pointer_reply_t *reply = xcb_grab_pointer_reply(conn, cookie, &error);
    if (reply == NULL) {
        FREE(error);
        xcb_disconnect(conn);
        errx(EXIT_FAILURE, "Failed to grab pointer.\n");
    }
    FREE(reply);

    xcb_window_t window = XCB_NONE;
    int buttons_pressed = 0;
    while (window == XCB_NONE || buttons_pressed != 0) {
        xcb_allow_events(conn, XCB_ALLOW_SYNC_POINTER, XCB_CURRENT_TIME);
        xcb_flush(conn);

        xcb_generic_event_t *event = xcb_wait_for_event(conn);
        switch (event->response_type & ~0x80) {
            case XCB_BUTTON_PRESS:
                if (window == XCB_NONE) {
                    xcb_button_press_event_t *ev = (xcb_button_press_event_t *)event;
                    window = ev->child;
                    if (window == XCB_NONE)
                        window = root;
                }

                buttons_pressed++;
                break;
            case XCB_BUTTON_RELEASE:
                if (buttons_pressed > 0)
                    buttons_pressed--;
                break;
        }

        FREE(event);
    }

    xcb_ungrab_pointer(conn, XCB_CURRENT_TIME);
    xcb_flush(conn);
    xcb_disconnect(conn);
    return window;
}
