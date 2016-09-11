
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

#ifndef __GUAC_IMAGE_TEXT_H
#define __GUAC_IMAGE_TEXT_H

#include "config.h"

#include <xorg-server.h>
#include <xf86.h>

/**
 * Guacamole implementation of ImageText8.
 */
void guac_drv_imagetext8(DrawablePtr drawable, GCPtr gc, int x, int y,
        int count, char* chars);

/**
 * Guacamole implementation of ImageText16.
 */
void guac_drv_imagetext16(DrawablePtr drawable, GCPtr gc, int x, int y,
        int count, unsigned short* chars);

/**
 * Guacamole implementation of ImageGlyphBlt.
 */
void guac_drv_imageglyphblt(DrawablePtr drawable, GCPtr gc, int x, int y,
        unsigned int nglyph, CharInfoPtr* char_info, pointer glyph_base);

#endif
