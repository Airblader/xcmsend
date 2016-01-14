// vim:ts=4:sw=4:expandtab
#include "all.h"

/*
 * Sends a _NET_WM_DESKTOP client message.
 *
 */
void message_send_net_wm_desktop(xcb_window_t window, uint32_t desktop, source_indicator_t source) {
    if (window == XCB_NONE)
        window = select_window();

    xcb_client_message_event_t event;
    memset(&event, 0, sizeof(event));

    event.response_type = XCB_CLIENT_MESSAGE;
    event.format = 32;
    event.type = A__NET_WM_DESKTOP;
    event.window = window;
    event.data.data32[0] = desktop;
    event.data.data32[1] = source;

    xcb_send_event(connection, 0, root, XCB_EVENT_MASK_NO_EVENT, (const char *)&event);
}
