
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
#include "display.h"
#include "drawable.h"
#include "gc.h"
#include "pixmap.h"
#include "poly.h"
#include "screen.h"
#include "list.h"

#include <xorg-server.h>
#include <xf86.h>
#include <fb.h>

/**
 * Copies the region of the framebuffer which corresponds to the line having
 * the given coordinates, taking into account the stroke width, etc.
 */
static void guac_drv_copy_line(DrawablePtr drawable, GCPtr gc, int x1, int y1,
        int x2, int y2) {

    /* Get drawable */
    guac_drv_drawable* guac_drawable = guac_drv_get_drawable(drawable);

    /* Draw to windows only */
    if (guac_drawable == NULL)
        return;

    /* Get guac_drv_screen */
    guac_drv_screen* guac_screen =
        (guac_drv_screen*) dixGetPrivate(&(gc->devPrivates),
                                     GUAC_GC_PRIVATE);

    /* Copy region from framebuffer */
    GUAC_DRV_DRAWABLE_CLIP(guac_drawable, drawable,
        fbGetCompositeClip(gc), guac_drv_drawable_copy_fb,
        drawable, x1, y1, x2 - x1, y2 - y1, guac_drawable, x1, y1);

    /* Signal change */
    guac_drv_display_touch(guac_screen->display);

}

void guac_drv_polypoint(DrawablePtr drawable, GCPtr gc, int mode, int npt,
        DDXPointPtr init) {
    /* STUB */
    GUAC_DRV_DRAWABLE_STUB_OP(drawable, gc);
    fbPolyPoint(drawable, gc, mode, npt, init);
}

void guac_drv_polyline(DrawablePtr drawable, GCPtr gc, int mode, int npt,
        DDXPointPtr init) {

    /* Call framebuffer version */
    fbPolyLine(drawable, gc, mode, npt, init);

    /* If less than two points, nothing to do */
    if (npt < 2)
        return;

    /* Retrieve first point in list */
    int x1 = init->x;
    int y1 = init->y;

    /* Iterate over remaining points only */
    DDXPointPtr current = &init[1];
    int remaining = npt - 1;

    /* Draw one line between each pair of points */
    while (remaining > 0) {

        /* Get coordinates of current point in series */
        int x2 = current->x;
        int y2 = current->y;

        /* Use previous point as origin, if requested */
        if (mode == CoordModePrevious) {
            x2 += x1;
            y2 += y1;
        }

        /* Draw line between previous and current points */
        guac_drv_copy_line(drawable, gc, x1, y1, x2, y2);

        /* Start next line at current point */
        x1 = x2;
        y1 = y2;

        /* Advance to ext point in series */
        current++;
        remaining--;

    }

}

void guac_drv_polysegment(DrawablePtr drawable, GCPtr gc, int nseg,
        xSegment* segs) {
    /* STUB */
    GUAC_DRV_DRAWABLE_STUB_OP(drawable, gc);
    fbPolySegment(drawable, gc, nseg, segs);
}

void guac_drv_polyrectangle(DrawablePtr drawable, GCPtr gc, int nrects,
        xRectangle* rects) {

    int i;

    /* Call framebuffer version */
    fbPolyRectangle(drawable, gc, nrects, rects);

    /* Draw all rects */
    for (i = 0; i < nrects; i++) {

        xRectangle* rect = &(rects[i]);

        /* Determine rectangle extents */
        int left   = rect->x;
        int top    = rect->y;
        int right  = rect->x + rect->width;
        int bottom = rect->y + rect->width;

        /* Copy all four lines of the rectangle */
        guac_drv_copy_line(drawable, gc, left,  top,    right, top);
        guac_drv_copy_line(drawable, gc, right, top,    right, bottom);
        guac_drv_copy_line(drawable, gc, right, bottom, left,  bottom);
        guac_drv_copy_line(drawable, gc, left,  bottom, left,  top);

    }

}

void guac_drv_polyarc(DrawablePtr drawable, GCPtr gc, int narcs,
        xArc* arcs) {
    /* STUB */
    GUAC_DRV_DRAWABLE_STUB_OP(drawable, gc);
    fbPolyArc(drawable, gc, narcs, arcs);
}

void guac_drv_fillpolygon(DrawablePtr drawable, GCPtr gc, int shape, int mode,
        int count, DDXPointPtr pts) {
    /* STUB */
    GUAC_DRV_DRAWABLE_STUB_OP(drawable, gc);
    fbFillPolygon(drawable, gc, shape, mode, count, pts);
}

void guac_drv_polyfillrect(DrawablePtr drawable, GCPtr gc, int nrects,
        xRectangle* rects) {

    /* Call framebuffer version */
    fbPolyFillRect(drawable, gc, nrects, rects);

    /* Get drawable */
    guac_drv_drawable* guac_drawable = guac_drv_get_drawable(drawable);

    /* Draw to windows only */
    if (guac_drawable != NULL) {

        int i;

        /* Get guac_drv_screen */
        guac_drv_screen* guac_screen =
            (guac_drv_screen*) dixGetPrivate(&(gc->devPrivates),
                                         GUAC_GC_PRIVATE);

        /* Draw all rects */
        for (i=0; i<nrects; i++) {
            xRectangle* rect = &(rects[i]);

            /* If tiled, fill with pixmap */
            if (gc->fillStyle == FillTiled && !gc->tileIsPixel) {

                guac_drv_drawable* guac_fill_drawable =
                    guac_drv_get_drawable((DrawablePtr) gc->tile.pixmap);

                if (guac_fill_drawable != NULL) {

                    /* Get dimensions of tile drawable */
                    int tile_w = guac_fill_drawable->layer->surface->width;
                    int tile_h = guac_fill_drawable->layer->surface->height;

                    /* Calculate coordinates of pattern within tile given GC origin */
                    int tile_x = GUAC_DRV_DRAWABLE_WRAP(rect->x - gc->patOrg.x, tile_w);
                    int tile_y = GUAC_DRV_DRAWABLE_WRAP(rect->y - gc->patOrg.y, tile_h);

                    /* Represent with a simple copy whenever possible */
                    if (tile_x + rect->width <= tile_w
                            && tile_y + rect->height <= tile_h)
                        GUAC_DRV_DRAWABLE_CLIP(guac_drawable, drawable,
                            fbGetCompositeClip(gc), guac_drv_drawable_copy,
                            guac_fill_drawable, tile_x, tile_y, rect->width,
                            rect->height, guac_drawable, rect->x, rect->y);

                    /* Otherwise, use an actual pattern fill */
                    else
                        GUAC_DRV_DRAWABLE_CLIP(guac_drawable, drawable,
                            fbGetCompositeClip(gc), guac_drv_drawable_drect,
                            guac_drawable, rect->x, rect->y, rect->width, rect->height,
                            guac_fill_drawable);

                }

                else
                    GUAC_DRV_DRAWABLE_CLIP(guac_drawable, drawable,
                        fbGetCompositeClip(gc), guac_drv_drawable_copy_fb,
                        drawable, rect->x, rect->y, rect->width, rect->height,
                        guac_drawable, rect->x, rect->y);

            }

            /* If solid, fill with color */
            else if (gc->fillStyle == FillSolid)
                GUAC_DRV_DRAWABLE_CLIP(guac_drawable, drawable,
                    fbGetCompositeClip(gc), guac_drv_drawable_crect,
                    guac_drawable, rect->x, rect->y, rect->width, rect->height,
                    gc->fgPixel);

            /* Otherwise, not yet implemented */
            else
                GUAC_DRV_DRAWABLE_CLIP(guac_drawable, drawable,
                    fbGetCompositeClip(gc), guac_drv_drawable_copy_fb,
                    drawable, rect->x, rect->y, rect->width, rect->height,
                    guac_drawable, rect->x, rect->y);

        }

        /* Signal change */
        guac_drv_display_touch(guac_screen->display);

    }

}

void guac_drv_polyfillarc(DrawablePtr drawable, GCPtr gc, int narcs,
        xArc* arcs) {
    /* STUB */
    GUAC_DRV_DRAWABLE_STUB_OP(drawable, gc);
    fbPolyFillArc(drawable, gc, narcs, arcs);
}

