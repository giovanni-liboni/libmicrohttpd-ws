/*
 * Copyright 2016 Giovanni Liboni part of libmicrohttpd-ws
 *
 * This source file is free software, under either the GPLv2 or the Apachev2 licence, available at:
 *   https://opensource.org/licenses/GPL-2.0
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * This source file is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE. See the referenced license files for details.
 */

/*
* @file websocket.h
* @author Giovanni Liboni
*/

#ifndef WEBSOCKET_H_
#define WEBSOCKET_H_

#include "internal.h"
#include "connection.h"
#include "sha1.h"
#include "base-64.h"

#include <errno.h>
#include <pthread.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

/*
 * From RFC 6455
   1000

      1000 indicates a normal closure, meaning that the purpose for
      which the connection was established has been fulfilled.

   1001

      1001 indicates that an endpoint is "going away", such as a server
      going down or a browser having navigated away from a page.

   1002

      1002 indicates that an endpoint is terminating the connection due
      to a protocol error.

   1003

      1003 indicates that an endpoint is terminating the connection
      because it has received a type of data it cannot accept (e.g., an
      endpoint that understands only text data MAY send this if it
      receives a binary message).

   1004

      Reserved.  The specific meaning might be defined in the future.

   1005

      1005 is a reserved value and MUST NOT be set as a status code in a
      Close control frame by an endpoint.  It is designated for use in
      applications expecting a status code to indicate that no status
      code was actually present.

   1006

      1006 is a reserved value and MUST NOT be set as a status code in a
      Close control frame by an endpoint.  It is designated for use in
      applications expecting a status code to indicate that the
      connection was closed abnormally, e.g., without sending or
      receiving a Close control frame.

   1007

      1007 indicates that an endpoint is terminating the connection
      because it has received data within a message that was not
      consistent with the type of the message (e.g., non-UTF-8 [RFC3629]
      data within a text message).

   1008

      1008 indicates that an endpoint is terminating the connection
      because it has received a message that violates its policy.  This
      is a generic status code that can be returned when there is no
      other more suitable status code (e.g., 1003 or 1009) or if there
      is a need to hide specific details about the policy.

   1009

      1009 indicates that an endpoint is terminating the connection
      because it has received a message that is too big for it to
      process.

   1010

      1010 indicates that an endpoint (client) is terminating the
      connection because it has expected the server to negotiate one or
      more extension, but the server didn't return them in the response
      message of the WebSocket handshake.  The list of extensions that
      are needed SHOULD appear in the /reason/ part of the Close frame.
      Note that this status code is not used by the server, because it
      can fail the WebSocket handshake instead.

   1011

      1011 indicates that a server is terminating the connection because
      it encountered an unexpected condition that prevented it from
      fulfilling the request.

   1015

      1015 is a reserved value and MUST NOT be set as a status code in a
      Close control frame by an endpoint.  It is designated for use in
      applications expecting a status code to indicate that the
      connection was closed due to a failure to perform a TLS handshake
      (e.g., the server certificate can't be verified).
*/
#define CLOSE_STATUS_NOSTATUS  0
#define CLOSE_STATUS_NORMAL  1000
#define CLOSE_STATUS_GOINGAWAY 1001
#define CLOSE_STATUS_PROTOCOL_ERR  1002
#define CLOSE_STATUS_UNACCEPTABLE_OPCODE 1003
#define CLOSE_STATUS_RESERVED 1004
#define CLOSE_STATUS_NO_STATUS 1005
#define CLOSE_STATUS_ABNORMAL_CLOSE 1006
#define CLOSE_STATUS_INVALID_PAYLOAD 1007
#define CLOSE_STATUS_POLICY_VIOLATION 1008
#define CLOSE_STATUS_MESSAGE_TOO_LARGE 1009
#define CLOSE_STATUS_EXTENSION_REQUIRED 1010
#define CLOSE_STATUS_UNEXPECTED_CONDITION 1011
#define CLOSE_STATUS_TLS_FAILURE 1015
/* ---------------------------------------------
 * 	PRIVATE
 * ---------------------------------------------*/
int WS_handshake(ws_connection *ws_struct);

int WS_close_MHD_connection(ws_connection *wsc);

void *WS_reader(void *arg);

void *sender(void *arg);

void ws_StringToFrame(  ws_data_frame *data_frame );

void ws_FrameToString(  ws_data_frame *data_frame );

int ws_my_send(ws_connection *wsc,  char *data, int len_data, int type);

int WS_remove_connection(ws_connection *this, ws_server_struct *wss);

int WS_add_connection(ws_connection *this);

int WS_close_connection(ws_connection *this, int error_code);

void ws_stop_sender(ws_connection *wsc);

void clr(ws_connection *wsc);

int callback_send( ws_connection *wsc, int to_all, void *data, int len_data, int type );

/**
 * Init a new websocket connection
 * @param wss Websocket server struct
 * @param connection The connection on which we activate websocket
 * @return The struct to handle the websocket connection
 */
ws_connection* WS_init_connection(ws_server_struct *wss, struct MHD_Connection *connection);

/**
 * Function to destroy the websocket connection structure
 * @param A websocket connection structure to destroy
 */
void WS_destroy_connection(ws_connection *wsc);

/**
 * Init a websocket server
 * @param A websocket server structure to initialize
 * @return An integer to handle the errors
 */
ws_server_struct* WS_init();

/**
 * Destroy the websocket server
 * @param A websocket server structure
 */
void WS_destroy(ws_server_struct* wss);

#endif /* WEBSOCKET_H_ */
