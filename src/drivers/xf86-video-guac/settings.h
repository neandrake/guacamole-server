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


#ifndef GUAC_DRV_SETTINGS_H
#define GUAC_DRV_SETTINGS_H

#include "config.h"

#include <guacamole/user.h>

#include <stdbool.h>

/**
 * The filename to use for the screen recording, if not specified.
 */
#define GUAC_DRV_DEFAULT_RECORDING_NAME "guac_recording"


/**
 * Settings specific to the Guacamole X.Org driver.
 */
typedef struct guac_drv_settings {

    /**
     * Whether this connection is read-only, and user input should be dropped.
     */
    bool read_only;

#ifdef ENABLE_COMMON_SSH
    /**
     * Whether SFTP should be enabled for the X.Org connection.
     */
    bool enable_sftp;

    /**
     * The hostname of the SSH server to connect to for SFTP.
     */
    char* sftp_hostname;

    /**
     * The port of the SSH server to connect to for SFTP.
     */
    char* sftp_port;

    /**
     * The username to provide when authenticating with the SSH server for
     * SFTP.
     */
    char* sftp_username;

    /**
     * The password to provide when authenticating with the SSH server for
     * SFTP (if not using a private key).
     */
    char* sftp_password;

    /**
     * The base64-encoded private key to use when authenticating with the SSH
     * server for SFTP (if not using a password).
     */
    char* sftp_private_key;

    /**
     * The passphrase to use to decrypt the provided base64-encoded private
     * key.
     */
    char* sftp_passphrase;

    /**
     * The default location for file uploads within the SSH server. This will
     * apply only to uploads which do not use the filesystem guac_object (where
     * the destination directory is otherwise ambiguous).
     */
    char* sftp_directory;
#endif

    /*
     * Whether all graphical updates for this connection should use lossless
     * compressoin only.
     */
    int lossless;

    /**
     * The path in which the screen recording should be saved, if enabled. If
     * no screen recording should be saved, this will be NULL.
     */
    char* recording_path;

    /**
     * The filename to use for the screen recording, if enabled.
     */
    char* recording_name;

    /**
     * Whether the screen recording path should be automatically created if it
     * does not already exist.
     */
    bool create_recording_path;

    /**
     * Whether output which is broadcast to each connected client (graphics,
     * streams, etc.) should NOT be included in the session recording. Output
     * is included by default, as it is necessary for any recording which must
     * later be viewable as video.
     */
    bool recording_exclude_output;

    /**
     * Whether changes to mouse state, such as position and buttons pressed or
     * released, should NOT be included in the session recording. Mouse state
     * is included by default, as it is necessary for the mouse cursor to be
     * rendered in any resulting video.
     */
    bool recording_exclude_mouse;

    /**
     * Whether keys pressed and released should be included in the session
     * recording. Key events are NOT included by default within the recording,
     * as doing so has privacy and security implications.  Including key events
     * may be necessary in certain auditing contexts, but should only be done
     * with caution. Key events can easily contain sensitive information, such
     * as passwords, credit card numbers, etc.
     */
    bool recording_include_keys;

    /**
     * Whether the tunnel should send keep-alive NOPs regularly to keep
     * the client from timing out when there's no activity.
     */
    bool keep_alive;

} guac_drv_settings;

/**
 * Parses all given args, storing them in a newly-allocated settings object. If
 * the args fail to parse, NULL is returned.
 *
 * @param user
 *     The user who submitted the given arguments while joining the
 *     connection.
 *
 * @param argc
 *     The number of arguments within the argv array.
 *
 * @param argv
 *     The values of all arguments provided by the user.
 *
 * @return
 *     A newly-allocated settings object which must be freed with
 *     guac_drv_settings_free() when no longer needed. If the arguments fail
 *     to parse, NULL is returned.
 */
guac_drv_settings* guac_drv_parse_args(guac_user* user,
        int argc, const char** argv);

/**
 * Frees the given guac_drv_settings object, having been previously allocated
 * via guac_drv_parse_args().
 *
 * @param settings
 *     The settings object to free.
 */
void guac_drv_settings_free(guac_drv_settings* settings);

/**
 * NULL-terminated array of accepted client args.
 */
extern const char* GUAC_DRV_CLIENT_ARGS[];

#endif

