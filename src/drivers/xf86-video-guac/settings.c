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

#include "settings.h"

#include <guacamole/user.h>

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

const char* GUAC_DRV_CLIENT_ARGS[] = {
    "read-only",

#ifdef ENABLE_COMMON_SSH
    "enable-sftp",
    "sftp-hostname",
    "sftp-port",
    "sftp-username",
    "sftp-password",
    "sftp-private-key",
    "sftp-passphrase",
    "sftp-directory",
#endif

    "force-lossless",

    "recording-path",
    "recording-name",
    "recording-exclude-output",
    "recording-exclude-mouse",
    "recording-include-keys",
    "create-recording-path",

    NULL
};

enum GUAC_DRV_ARGS_IDX {

    /**
     * "true" if this connection should be read-only (user input should be
     * dropped), "false" or blank otherwise.
     */
    IDX_READ_ONLY,

#ifdef ENABLE_COMMON_SSH
    /**
     * "true" if SFTP should be enabled for the X.Org connection, "false" or
     * blank otherwise.
     */
    IDX_ENABLE_SFTP,

    /**
     * The hostname of the SSH server to connect to for SFTP. If blank,
     * "localhost" will be used.
     */
    IDX_SFTP_HOSTNAME,

    /**
     * The port of the SSH server to connect to for SFTP. If blank, the default
     * SSH port of "22" will be used.
     */
    IDX_SFTP_PORT,

    /**
     * The username to provide when authenticating with the SSH server for
     * SFTP.
     */
    IDX_SFTP_USERNAME,

    /**
     * The password to provide when authenticating with the SSH server for
     * SFTP (if not using a private key).
     */
    IDX_SFTP_PASSWORD,

    /**
     * The base64-encoded private key to use when authenticating with the SSH
     * server for SFTP (if not using a password).
     */
    IDX_SFTP_PRIVATE_KEY,

    /**
     * The passphrase to use to decrypt the provided base64-encoded private
     * key.
     */
    IDX_SFTP_PASSPHRASE,

    /**
     * The default location for file uploads within the SSH server. This will
     * apply only to uploads which do not use the filesystem guac_object (where
     * the destination directory is otherwise ambiguous).
     */
    IDX_SFTP_DIRECTORY,
#endif

    /**
     * "true" if all graphical updates for this connection should use lossless
     * compression only, "false" or blank otherwise.
     */
    IDX_FORCE_LOSSLESS,

    /**
     * The full absolute path to the directory in which screen recordings
     * should be written.
     */
    IDX_RECORDING_PATH,

    /**
     * The name that should be given to screen recordings which are written in
     * the given path.
     */
    IDX_RECORDING_NAME,

    /**
     * Whether output which is broadcast to each connected client (graphics,
     * streams, etc.) should NOT be included in the session recording. Output
     * is included by default, as it is necessary for any recording which must
     * later be viewable as video.
     */
    IDX_RECORDING_EXCLUDE_OUTPUT,

    /**
     * Whether changes to mouse state, such as position and buttons pressed or
     * released, should NOT be included in the session recording. Mouse state
     * is included by default, as it is necessary for the mouse cursor to be
     * rendered in any resulting video.
     */
    IDX_RECORDING_EXCLUDE_MOUSE,

    /**
     * Whether keys pressed and released should be included in the session
     * recording. Key events are NOT included by default within the recording,
     * as doing so has privacy and security implications.  Including key events
     * may be necessary in certain auditing contexts, but should only be done
     * with caution. Key events can easily contain sensitive information, such
     * as passwords, credit card numbers, etc.
     */
    IDX_RECORDING_INCLUDE_KEYS,

    /**
     * Whether the specified screen recording path should automatically be
     * created if it does not yet exist.
     */
    IDX_CREATE_RECORDING_PATH,

    DRV_ARGS_COUNT
};

guac_drv_settings* guac_drv_parse_args(guac_user* user,
        int argc, const char** argv) {

    /* Validate arg count */
    if (argc != DRV_ARGS_COUNT) {
        guac_user_log(user, GUAC_LOG_WARNING, "Incorrect number of connection "
                "parameters provided: expected %i, got %i.",
                DRV_ARGS_COUNT, argc);
        return NULL;
    }

    guac_drv_settings* settings = calloc(1, sizeof(guac_drv_settings));

    /* Read-only mode */
    settings->read_only =
        guac_user_parse_args_boolean(user, GUAC_DRV_CLIENT_ARGS, argv,
                IDX_READ_ONLY, false);

#ifdef ENABLE_COMMON_SSH
    /* SFTP enable/disable */
    settings->enable_sftp =
        guac_user_parse_args_boolean(user, GUAC_DRV_CLIENT_ARGS, argv,
                IDX_ENABLE_SFTP, false);

    /* Hostname for SFTP connection */
    settings->sftp_hostname =
        guac_user_parse_args_string(user, GUAC_DRV_CLIENT_ARGS, argv,
                IDX_SFTP_HOSTNAME, "localhost");

    /* Port for SFTP connection */
    settings->sftp_port =
        guac_user_parse_args_string(user, GUAC_DRV_CLIENT_ARGS, argv,
                IDX_SFTP_PORT, "22");

    /* Username for SSH/SFTP authentication */
    settings->sftp_username =
        guac_user_parse_args_string(user, GUAC_DRV_CLIENT_ARGS, argv,
                IDX_SFTP_USERNAME, "");

    /* Password for SFTP (if not using private key) */
    settings->sftp_password =
        guac_user_parse_args_string(user, GUAC_DRV_CLIENT_ARGS, argv,
                IDX_SFTP_PASSWORD, "");

    /* Private key for SFTP (if not using password) */
    settings->sftp_private_key =
        guac_user_parse_args_string(user, GUAC_DRV_CLIENT_ARGS, argv,
                IDX_SFTP_PRIVATE_KEY, NULL);

    /* Passphrase for decrypting the SFTP private key (if applicable */
    settings->sftp_passphrase =
        guac_user_parse_args_string(user, GUAC_DRV_CLIENT_ARGS, argv,
                IDX_SFTP_PASSPHRASE, "");

    /* Default upload directory */
    settings->sftp_directory =
        guac_user_parse_args_string(user, GUAC_DRV_CLIENT_ARGS, argv,
                IDX_SFTP_DIRECTORY, NULL);
#endif

    /* Lossless compression */
    settings->lossless =
        guac_user_parse_args_boolean(user, GUAC_DRV_CLIENT_ARGS, argv,
                IDX_FORCE_LOSSLESS, 0);

    /* Read recording path */
    settings->recording_path =
        guac_user_parse_args_string(user, GUAC_DRV_CLIENT_ARGS, argv,
                IDX_RECORDING_PATH, NULL);

    /* Read recording name */
    settings->recording_name =
        guac_user_parse_args_string(user, GUAC_DRV_CLIENT_ARGS, argv,
                IDX_RECORDING_NAME, GUAC_DRV_DEFAULT_RECORDING_NAME);

    /* Parse output exclusion flag */
    settings->recording_exclude_output =
        guac_user_parse_args_boolean(user, GUAC_DRV_CLIENT_ARGS, argv,
                IDX_RECORDING_EXCLUDE_OUTPUT, false);

    /* Parse mouse exclusion flag */
    settings->recording_exclude_mouse =
        guac_user_parse_args_boolean(user, GUAC_DRV_CLIENT_ARGS, argv,
                IDX_RECORDING_EXCLUDE_MOUSE, false);

    /* Parse key event inclusion flag */
    settings->recording_include_keys =
        guac_user_parse_args_boolean(user, GUAC_DRV_CLIENT_ARGS, argv,
                IDX_RECORDING_INCLUDE_KEYS, false);

    /* Parse path creation flag */
    settings->create_recording_path =
        guac_user_parse_args_boolean(user, GUAC_DRV_CLIENT_ARGS, argv,
                IDX_CREATE_RECORDING_PATH, false);

    return settings;

}

void guac_drv_settings_free(guac_drv_settings* settings) {

#ifdef ENABLE_COMMON_SSH
    /* Free SFTP settings */
    free(settings->sftp_directory);
    free(settings->sftp_hostname);
    free(settings->sftp_passphrase);
    free(settings->sftp_password);
    free(settings->sftp_port);
    free(settings->sftp_private_key);
    free(settings->sftp_username);
#endif

    if (settings->recording_name != NULL)
        free(settings->recording_name);
    if (settings->recording_path != NULL)
        free(settings->recording_path);

    /* Free settings structure */
    free(settings);

}

