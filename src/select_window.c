// vim:ts=4:sw=4:expandtab
#include "all.h"

static bool window_is_client(xcb_connection_t *conn, xcb_window_t window) {
    xcb_get_property_cookie_t pcookie = xcb_get_property(conn, false, window, A_WM_STATE,
            XCB_GET_PROPERTY_TYPE_ANY, 0, UINT32_MAX);
    xcb_get_property_reply_t *preply = xcb_get_property_reply(conn, pcookie, NULL);

    bool result = (preply != NULL && xcb_get_property_value_length(preply) != 0);
    FREE(preply);

    return result;
}

static xcb_window_t descend_window(xcb_connection_t *conn, xcb_window_t window) {
    xcb_grab_server(conn);

    xcb_query_tree_cookie_t tcookie = xcb_query_tree(conn, window);
    xcb_query_tree_reply_t *treply = xcb_query_tree_reply(conn, tcookie, NULL);
    if (treply == NULL)
        return XCB_NONE;

    if (xcb_query_tree_children_length(treply) == 0)
        return XCB_NONE;
    xcb_window_t *children = xcb_query_tree_children(treply);

    xcb_window_t client = XCB_NONE;
    for (int i = 0; i < xcb_query_tree_children_length(treply); i++) {
        xcb_get_window_attributes_cookie_t acookie = xcb_get_window_attributes(conn, children[i]);
        xcb_get_window_attributes_reply_t *areply = xcb_get_window_attributes_reply(conn, acookie, NULL);
        if (areply == NULL || areply->map_state != XCB_MAP_STATE_VIEWABLE) {
            children[i] = XCB_NONE;
            continue;
        }
        FREE(areply);

        if (!window_is_client(conn, children[i]))
            continue;

        client = children[i];
        goto done;
    }

    for (int i = 0; i < xcb_query_tree_children_length(treply); i++) {
        if (children[i] == XCB_NONE)
            continue;
        client = descend_window(conn, children[i]);
        if (client != XCB_NONE)
            break;
    }

done:
    FREE(treply);
    xcb_ungrab_server(conn);

    return client;
}

/*
 * Let the user select a window.
 *
 * This is an almost 1:1 translation of xprop's code for XCB.
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

    if (window != root_screen->root && !window_is_client(conn, window)) {
        xcb_window_t child = descend_window(conn, window);
        if (child != XCB_NONE)
            window = child;
    }

    xcb_disconnect(conn);
    return window;
}
