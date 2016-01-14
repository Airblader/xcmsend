// vim:ts=4:sw=4:expandtab
#pragma once

/**
 * Sends a _NET_WM_DESKTOP client message.
 *
 */
void message_send_net_wm_desktop(xcb_window_t window, uint32_t desktop, source_indicator_t source);
