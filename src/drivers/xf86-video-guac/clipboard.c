/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

#include "agent.h"
#include "clipboard.h"
#include "common/clipboard.h"
#include "user.h"
#include "xclient.h"

#include <guacamole/client.h>
#include <guacamole/stream.h>
#include <guacamole/user.h>

#include <xcb/xcb.h>
#include <xcb/xfixes.h>

int guac_drv_clipboard_handler(guac_user* user, guac_stream* stream,
        char* mimetype) {

    /* Clear clipboard and prepare for new data */
    guac_drv_user_data* user_data = (guac_drv_user_data*) user->data;
    guac_common_clipboard_reset(user_data->display->clipboard, mimetype);

    /* Set handlers for clipboard stream */
    stream->blob_handler = guac_drv_clipboard_blob_handler;
    stream->end_handler = guac_drv_clipboard_end_handler;

    return 0;
}

int guac_drv_clipboard_blob_handler(guac_user* user, guac_stream* stream,
        void* data, int length) {

    /* Append new data */
    guac_drv_user_data* user_data = (guac_drv_user_data*) user->data;
    guac_common_clipboard_append(user_data->display->clipboard, data, length);

    return 0;
}

int guac_drv_clipboard_end_handler(guac_user* user, guac_stream* stream) {

    guac_drv_user_data* user_data = (guac_drv_user_data*) user->data;
    guac_drv_agent* agent = user_data->agent;
    guac_common_clipboard* clipboard = user_data->display->clipboard;
    xcb_connection_t* connection = agent->connection;
    xcb_generic_error_t* error;

    xcb_atom_t utf8_string = guac_drv_get_atom(connection, "UTF8_STRING");
    if (utf8_string == XCB_ATOM_NONE) {
        guac_user_log(agent->user, GUAC_LOG_WARNING, "X server does not "
                "support the UTF8_STRING atom. Clipboard will not work.");
        return 0;
    }

    xcb_atom_t xa_clipboard = guac_drv_get_atom(connection, "CLIPBOARD");
    if (xa_clipboard == XCB_ATOM_NONE) {
        guac_user_log(agent->user, GUAC_LOG_WARNING, "X server does not "
                "support the CLIPBOARD atom. Pasting into clipboard will not "
                "work.");
        return 0;
    }

    // Declare the dummy window as owner of the clipboard
    xcb_void_cookie_t sel_owner_cookie = xcb_set_selection_owner_checked(
        connection, agent->dummy, xa_clipboard, XCB_CURRENT_TIME);
    if ((error = xcb_request_check(connection, sel_owner_cookie))) {
        guac_user_log(agent->user, GUAC_LOG_ERROR, "Failed to set clipbaord "
                "owner.");
        return 0;
    }

    // Set the clipboard value
    xcb_void_cookie_t change_prop_cookie = xcb_change_property_checked(
        connection, XCB_PROP_MODE_REPLACE, agent->dummy,
        xa_clipboard, utf8_string, 8,
        clipboard->length, clipboard->buffer);
    if ((error = xcb_request_check(connection, change_prop_cookie))) {
        guac_user_log(agent->user, GUAC_LOG_ERROR, "Failed to change "
            "clipboard property.");
        return 0;
    }

    return 0;
}

