
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

#include "config.h"
#include "cursor.h"
#include "display.h"
#include "log.h"
#include "screen.h"
#include "user.h"

#include <xorg-server.h>
#include <xf86.h>
#include <xf86Cursor.h>
#include <cursorstr.h>

static void guac_drv_cursor_render(guac_drv_cursor* guac_cursor,
        CursorPtr cursor, Bool use_argb) {

    int x;
    int y;

    /* Calculate foreground color */
    uint32_t fg = 0xFF000000
        | ((0xFF & cursor->foreRed) << 16)
        | ((0xFF & cursor->foreGreen) << 8)
        | (0xFF & cursor->foreBlue);

    /* Calculate background color */
    uint32_t bg = 0xFF000000
        | ((0xFF & cursor->backRed) << 16)
        | ((0xFF & cursor->backGreen) << 8)
        | (0xFF & cursor->backBlue);

    CARD32* argb = cursor->bits->argb;

    /* Get source and destination image data */
    uint32_t* src_row = (uint32_t*) cursor->bits->source;
    uint32_t* mask_row = (uint32_t*) cursor->bits->mask;
    uint32_t* dst_row = guac_cursor->image;

    /* For each row of image data */
    for (y = 0; y < guac_cursor->height; y++) {

        /* Get first pixel/bit in current row */
        uint32_t* dst = dst_row;
        uint32_t src = *src_row;
        uint32_t mask = *mask_row;

        /* For each pixel within row */
        for (x = 0; x < guac_cursor->width; x++) {

            /* Draw pixel only if mask is set. */
            if (mask & 0x1) {

                if (use_argb == TRUE) {

                    *dst = (*argb | 0xFF000000);
                } else {

                    /* Select foreground/background depending on source bit */
                    *dst = (src & 0x1) ? fg : bg;
                }

            } else {

                /* A transparent pixel */
                *dst = 0x0;
            }

            /* Next pixel */
            dst++;
            argb++;
            src >>= 1;
            mask >>= 1;

        }

        /* Next row */
        dst_row += GUAC_DRV_CURSOR_MAX_WIDTH;
        src_row++;
        mask_row++;

    }

}

static void guac_drv_set_cursor_colors(ScrnInfoPtr screen_info, int bg, int fg) {
    /* Do nothing */
}

static void __guac_drv_set_cursor_position(ScreenPtr screen, int x, int y) {

    guac_drv_screen* guac_screen =
        (guac_drv_screen*) dixGetPrivate(&(screen->devPrivates),
                GUAC_SCREEN_PRIVATE);

    guac_common_cursor* common_cursor = guac_screen->display->display->cursor;
    guac_user* user = common_cursor->user;

    /* The user may be null during initialization. */
    if (user != NULL) {
        /* Do nothing for now. This can incorrect reset the cursor
         * image in situations where the application is rendering its
         * own cursor and the cursor position is also being set. */
        //guac_drv_user_mouse_handler(user, x, y, common_cursor->button_mask);
    }
}

static void guac_drv_set_cursor_position(ScrnInfoPtr screen_info, int x, int y) {
    __guac_drv_set_cursor_position(screen_info->pScreen, x, y);
}

static Bool guac_drv_screen_set_cursor_position(DeviceIntPtr device,
    ScreenPtr screen,
    int x, int y,
    Bool generateEvent) {

    Bool ret = TRUE;

    __guac_drv_set_cursor_position(screen, x, y);

    /* Get guac_drv_screen */
    guac_drv_screen* guac_screen =
        (guac_drv_screen*) dixGetPrivate(&(screen->devPrivates),
                GUAC_SCREEN_PRIVATE);

    if (guac_screen->wrapped_set_cursor_pos) {
        ret = guac_screen->wrapped_set_cursor_pos(device, screen, x, y,
            generateEvent);
    }

    return ret;
}

static void __guac_drv_load_cursor_image(guac_drv_screen* guac_screen,
    guac_drv_cursor* guac_cursor) {

    /* Set cursor of display */
    guac_common_cursor_set_argb(guac_screen->display->display->cursor,
            guac_cursor->hotspot_x, guac_cursor->hotspot_y,
            (unsigned char*) guac_cursor->image, guac_cursor->width,
            guac_cursor->height, GUAC_DRV_CURSOR_STRIDE);

    guac_drv_display_touch(guac_screen->display);
}

static void guac_drv_load_cursor_image(ScrnInfoPtr pScrn,
        unsigned char* bits) {

    ScreenPtr screen = pScrn->pScreen;

    guac_drv_screen* guac_screen =
        (guac_drv_screen*) dixGetPrivate(&(screen->devPrivates),
                GUAC_SCREEN_PRIVATE);

    guac_drv_cursor* guac_cursor = guac_screen->display->cursor;

    __guac_drv_load_cursor_image(guac_screen, guac_cursor);
}

static void guac_drv_hide_cursor(ScrnInfoPtr screen_info) {
    /**
     * Do nothing. The cursor is never rendered on the server but instead we
     * transmit the cursor image to the client for rendering. The guacamole
     * protocol does not support hide/show of the cursor.
     */
}

static void guac_drv_show_cursor(ScrnInfoPtr screen_info) {
    /**
     * Do nothing. The cursor is never rendered on the server but instead we
     * transmit the cursor image to the client for rendering. The guacamole
     * protocol does not support hide/show of the cursor.
     */
}

static Bool guac_drv_use_hw_cursor(ScreenPtr screen, CursorPtr cursor) {
    return TRUE;
}

static guac_drv_cursor* __guac_drv_realize_cursor(guac_drv_screen* guac_screen,
    CursorPtr cursor) {

    guac_drv_cursor* guac_cursor = guac_screen->display->cursor;

    /* Assign dimensions */
    guac_cursor->width = cursor->bits->width;
    guac_cursor->height = cursor->bits->height;

    /* Assign hotspot */
    guac_cursor->hotspot_x = cursor->bits->xhot;
    guac_cursor->hotspot_y = cursor->bits->yhot;

    if (cursor->bits->argb != NULL) {
        /* Use ARGB cursor image if available */
        guac_drv_cursor_render(guac_cursor, cursor, TRUE);
    } else {
        /* Otherwise, use glyph cursor */
        guac_drv_cursor_render(guac_cursor, cursor, FALSE);
    }

    return guac_cursor;
}

static Bool guac_drv_screen_realize_cursor(DeviceIntPtr device,
    ScreenPtr screen,
    CursorPtr cursor) {

    Bool ret = TRUE;

    /* Get guac_drv_screen */
    guac_drv_screen* guac_screen =
        (guac_drv_screen*) dixGetPrivate(&(screen->devPrivates),
                                         GUAC_SCREEN_PRIVATE);

    guac_drv_cursor* guac_cursor = __guac_drv_realize_cursor(guac_screen,
        cursor);

    __guac_drv_load_cursor_image(guac_screen, guac_cursor);

    if (guac_screen->wrapped_realize_cursor) {
        ret = guac_screen->wrapped_realize_cursor(device, screen, cursor);
    }

    return ret;
}

static unsigned char* guac_drv_realize_cursor(xf86CursorInfoPtr cursor_info,
    CursorPtr cursor) {

    /* Get guac_drv_screen */
    ScreenPtr screen = cursor_info->pScrn->pScreen;
    guac_drv_screen* guac_screen =
        (guac_drv_screen*) dixGetPrivate(&(screen->devPrivates),
                                         GUAC_SCREEN_PRIVATE);

    guac_drv_cursor* guac_cursor = __guac_drv_realize_cursor(guac_screen,
        cursor);

    guac_drv_load_cursor_image(cursor_info->pScrn,
                (unsigned char*) guac_cursor);

    return (unsigned char*) guac_cursor;

}

/**
 * TODO: Investigate whether an implementation of UnrealizeCursor is necessary.
 * With this registered, even with a "do-nothing" implementation this results
 * in a segfault when windows are destroyed. Further investigation for why
 * should compare against the default UnrealizeCursor implementation.
 */
/*
static Bool guac_drv_screen_unrealize_cursor(DeviceIntPtr device,
    ScreenPtr screen, CursorPtr cursor) {

    Bool ret = TRUE;

    // Get guac_drv_screen //
    guac_drv_screen* guac_screen =
        (guac_drv_screen*) dixGetPrivate(&(screen->devPrivates),
                                         GUAC_SCREEN_PRIVATE);

    guac_drv_cursor* guac_cursor = __guac_drv_realize_cursor(guac_screen,
        cursor);

    __guac_drv_load_cursor_image(screen, guac_cursor);

    if (guac_screen->wrapped_unrealize_cursor) {
        ret = guac_screen->wrapped_unrealize_cursor(device, screen, cursor);
    }

    return ret;
}
*/

static Bool guac_drv_screen_display_cursor(DeviceIntPtr device,
    ScreenPtr screen, CursorPtr cursor) {

    Bool ret = TRUE;

    /* Get guac_drv_screen */
    guac_drv_screen* guac_screen =
        (guac_drv_screen*) dixGetPrivate(&(screen->devPrivates),
                                         GUAC_SCREEN_PRIVATE);

    /* Cursor may be null during initialization */
    if (cursor) {
        guac_drv_cursor* guac_cursor = __guac_drv_realize_cursor(guac_screen,
            cursor);

        __guac_drv_load_cursor_image(guac_screen, guac_cursor);
    }

    if (guac_screen->wrapped_display_cursor) {
        ret = guac_screen->wrapped_display_cursor(device, screen, cursor);
    }

    return ret;
}

Bool guac_drv_init_cursor(ScreenPtr screen) {

    /* Get cursor info struct */
    xf86CursorInfoPtr cursor_info = xf86CreateCursorInfoRec();
    if (!cursor_info)
        return FALSE;

    /* Init cursor info */
    cursor_info->MaxHeight = GUAC_DRV_CURSOR_MAX_WIDTH;
    cursor_info->MaxWidth = GUAC_DRV_CURSOR_MAX_HEIGHT;
    cursor_info->Flags =
          HARDWARE_CURSOR_ARGB
        | HARDWARE_CURSOR_UPDATE_UNHIDDEN
        | HARDWARE_CURSOR_SOURCE_MASK_INTERLEAVE_1;

    /* Set handlers */
    cursor_info->RealizeCursor = guac_drv_realize_cursor;
    cursor_info->SetCursorPosition = guac_drv_set_cursor_position;
    cursor_info->HideCursor = guac_drv_hide_cursor;
    cursor_info->ShowCursor = guac_drv_show_cursor;

    /* Glyph cursors (ARGB data is stored within the cursor data by our
     * implementation of RealizeCursor) */
    cursor_info->SetCursorColors = guac_drv_set_cursor_colors;
    cursor_info->UseHWCursor = guac_drv_use_hw_cursor;
    cursor_info->LoadCursorImage = guac_drv_load_cursor_image;

    /* Get guac_drv_screen */
    guac_drv_screen* guac_screen =
        (guac_drv_screen*) dixGetPrivate(&(screen->devPrivates),
                                         GUAC_SCREEN_PRIVATE);

    guac_screen->display->cursor->cursor_info = cursor_info;

    /* Register other cursor related callbacks on the ScreenPtr.
     * The CursorInfoPtr callbacks above only seem to be called when
     * custom cursor rendering is applied but not when the standard
     * system cursors are applied (pointer, I-bar, finger pointer, etc.).
     * The callbacks on the ScreenPtr appear to be the hook for the
     * standard/system cursor rendering hooks.*/

    guac_screen->wrapped_realize_cursor = screen->RealizeCursor;
    screen->RealizeCursor = guac_drv_screen_realize_cursor;

    /* TODO: Investigate whether an implementation of UnrealizeCursor is
     * necessary. See guac_drv_screen_unrealize_cursor. */
    /*
    guac_screen->wrapped_unrealize_cursor = screen->UnrealizeCursor;
    screen->UnrealizeCursor = guac_drv_screen_unrealize_cursor;
    */

    guac_screen->wrapped_set_cursor_pos = screen->SetCursorPosition;
    screen->SetCursorPosition = guac_drv_screen_set_cursor_position;

    guac_screen->wrapped_display_cursor = screen->DisplayCursor;
    screen->DisplayCursor = guac_drv_screen_display_cursor;

    return xf86InitCursor(screen, cursor_info);

}

void guac_drv_cursor_free(guac_drv_cursor* cursor) {
    xf86DestroyCursorInfoRec(cursor->cursor_info);
    free(cursor);
}

