/*
     This file is part of libmicrohttpd
     (C) 2013 2014 2015 2016 Giovanni Liboni

     This library is free software; you can redistribute it and/or
     modify it under the terms of the GNU Lesser General Public
     License as published by the Free Software Foundation; either
     version 2.1 of the License, or (at your option) any later version.

     This library is distributed in the hope that it will be useful,
     but WITHOUT ANY WARRANTY; without even the implied warranty of
     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
     Lesser General Public License for more details.

     You should have received a copy of the GNU Lesser General Public
     License along with this library; if not, write to the Free Software
     Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/
/*
 		Copyright 2016 Giovanni Liboni

 		Licensed under the Apache License, Version 2.0 (the "License");
 		you may not use this file except in compliance with the License.
 		You may obtain a copy of the License at

 				http://www.apache.org/licenses/LICENSE-2.0

 		Unless required by applicable law or agreed to in writing, software
 		distributed under the License is distributed on an "AS IS" BASIS,
 		WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 		See the License for the specific language governing permissions and
 		limitations under the License.
*/
/*
* @file websocket.c
* @author Giovanni Liboni
*/

#include "websocket.h"
#include <sys/socket.h>
#include <sys/fcntl.h>
#include <sys/socketvar.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <string.h>
/**
 * Set the socket for websocket protocol
 * @sockfd Socket
 * @return -1 if an error occurs
 */
int
WS_init_socket(int sockfd)
{
	int opt = 1;
	/* Disable Nagle */
	setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY,(const void *)&opt, sizeof(opt));
	setsockopt(sockfd, SOL_SOCKET, SO_KEEPALIVE, &opt, sizeof(opt));

	opt = 0;
	/* disable TCP_CORK */
	setsockopt( sockfd, IPPROTO_TCP, TCP_CORK, &opt, sizeof(opt) );

#ifdef WIN32
	opt = 0;
	ioctlsocket(fd, FIONBIO, (unsigned long *)&opt);
#else
	ioctl(sockfd, FIONBIO, &opt);
#endif
	return 1;
}
/**
 * Queue a response to be transmitted to the client (as soon as
 * possible but after MHD_AccessHandlerCallback returns).
 *
 * @param connection the connection identifying the client
 * @param status_code HTTP status code (i.e. 200 for OK)
 * @param response response to transmit
 * @return MHD_NO on error (i.e. reply already sent),
 *         MHD_YES on success or if message has been queued
 */
int
MHD_queue_response_upgrade (struct MHD_Connection *connection,
		unsigned int status_code, struct MHD_Response *response)
{
	if ( (NULL == connection) ||
			(NULL == response) ||
			(NULL != connection->response) ||
			( (MHD_CONNECTION_HEADERS_PROCESSED != connection->state) &&
					(MHD_CONNECTION_FOOTERS_RECEIVED != connection->state) ) )
		return MHD_NO;

	connection->read_closed = MHD_NO;
	connection->ws_upgraded = MHD_YES;
	MHD_increment_response_rc (response);
	connection->response = response;
	connection->responseCode = status_code;
	if ( (NULL != connection->method) &&
			(0 == strcasecmp (connection->method, MHD_HTTP_METHOD_HEAD)) )
	{
		/* if this is a "HEAD" request, pretend that we
	         have already sent the full message body */
		connection->response_write_position = response->total_size;
	}
	if ( (MHD_CONNECTION_HEADERS_PROCESSED == connection->state) &&
			(NULL != connection->method) &&
			( (0 == strcasecmp (connection->method,
					MHD_HTTP_METHOD_POST)) ||
					(0 == strcasecmp (connection->method,
							MHD_HTTP_METHOD_PUT))) )
	{
		/* response was queued "early", refuse to read body / footers or
	         further requests! */
		connection->state = MHD_CONNECTION_FOOTERS_RECEIVED;
	}

	return MHD_YES;
}
/*
 * Ritorna MHD_YES se una risposta è stata inserita, in caso contrario MHD_NO
 */
typedef int (*Callback_upgrade_request) ( struct MHD_connection *connection , void *cls);

/*
 * Per un eventuale riuso futuro bisogna definire dei campi personali e passare la struttura alla callback che gestisce la connessione
 */
typedef struct
{
	/*
	 * Websocket host
	 */
	char *host;
	/*
	 * Websocket key ( used in handshake )
	 */
	char sec_websocket_key[128];
	/*
	 * Websocket protocol ( if specified )
	 */
	char *sec_websocket_protocol;

	/*
	 * Websocket extension ( if specified )
	 */
	char *sec_websocket_extensions;
	/*
	 * Websocket version
	 */
	int sec_websocket_version;
	/*
	 * Websocket origin
	 */
	char *sec_websocket_origin;

}WS_upgrade;


/*
 * Can upgrade a socket. Ritorna MHD_YES se è stato effettuato un upgrade del socket, altrimenti ritorna MHD_NO;
 */
int
WS_upgrade_request( struct MHD_Connection *connection)
{
	const char *h_value;
	connection->ws_upgraded = MHD_NO;

	if( strcasecmp( connection->version, MHD_HTTP_VERSION_1_1 ) || strcasecmp( connection->method, MHD_HTTP_METHOD_GET ))
		return MHD_NO;

	h_value = MHD_lookup_connection_value (connection, MHD_HEADER_KIND, MHD_HTTP_HEADER_CONNECTION);
	if (h_value == NULL) return 0;
	if (0 != strcasecmp (h_value, MHD_HTTP_HEADER_UPGRADE)) return MHD_NO;


	h_value = MHD_lookup_connection_value (connection, MHD_HEADER_KIND, MHD_HTTP_HEADER_UPGRADE);
	if (h_value == NULL) return 0;
	if (0 != strcasecmp (h_value, "Websocket")) return MHD_NO;

	return MHD_YES;
}

/*
 * Apertura del canale websocket dopo aver eseguito l'upgrade. Dopo questa chiamata il controllo
 * della connessione verrà passato alle due callback definite.
 * Si occupa anche dell'handshake
 * NB. Entrambe NON devono essere NULL.
 */
int
WS_start( struct MHD_Connection *connection, Callback_handle_ws_recv rcv,
					Callback_handle_ws_send snd, Free_websocket_callback fwc, void *cls)
{
	int sec_websocket_version;
	char sec_websocket_key[128];
	struct MHD_Response *response;
	int ret;
	const char *h_value;

	if ( connection->daemon->wss == NULL )
		{
			connection->daemon->wss = WS_init();
//			connection->daemon->wss = connection->daemon;
		}

	ws_connection *wsc = WS_init_connection(connection->daemon->wss, connection);

	wsc->on = 1;
	wsc->sockfd = connection->socket_fd;
	connection->wsc = wsc;
	connection->ws_upgraded = MHD_NO;

	// SET CALLBACK TO READ
	if(rcv == NULL) return MHD_NO;
	else wsc->rcv = rcv;

	// SET USER'S POINTER
	wsc->cls = cls;

	// SET FREE CALLBACK
	wsc->fwc = fwc;

	wsc->snd = snd;

	if( NULL != wsc->crw )
		ret = wsc->crw( connection );
	else
	{

		/* ---------------------------------------------
		 * 	HANDSHAKE: THIS METHOD CREATES THE RESPONSE
		 * 			   FOR THE UPGRADE REQUEST
		 * ---------------------------------------------*/


		h_value = MHD_lookup_connection_value (connection, MHD_HEADER_KIND, "Sec-WebSocket-Key");
		if(h_value == NULL ) return MHD_NO;
		else strncpy(sec_websocket_key, h_value, 128);

		h_value = (char *) MHD_lookup_connection_value (connection, MHD_HEADER_KIND, "Sec-WebSocket-Version");
		if(h_value != NULL) sec_websocket_version = atoi(h_value);
		else return MHD_NO;

		if (sec_websocket_version == 13)
		{
			// creo la chiave da mandare
			unsigned char hash[20];
			unsigned char s[256];
			int i;
			i = sprintf((char *)s, "%s258EAFA5-E914-47DA-95CA-C5AB0DC85B11", sec_websocket_key);
			SHA1(s, i, hash);
			if ( (i = b64_encode_string((char *)hash, 20,(char *)s,256)) < 0)
				perror("b64_encode_string");

			if (i > 128)
				printf("Error: String b64_encode is larger than 128\n");

			if (memcpy(sec_websocket_key, (char *)s, i) < 0)
				perror("memcpy");

			// creo la risposta

			response = MHD_create_response_from_data(0,NULL,0,0);
			MHD_add_response_header (response, "Sec-WebSocket-Accept", sec_websocket_key);
			MHD_add_response_header (response, "Connection", "Upgrade");
			MHD_add_response_header (response, "Upgrade", "Websocket");

			ret = MHD_queue_response_upgrade (connection,MHD_HTTP_SWITCHING_PROTOCOLS, response);
			MHD_destroy_response (response);
		}
		else
		{
			/*
			 * HTTP/1.1 400 Bad Request
			 * ...
			 * Sec-WebSocket-Version: 13
			 */

			response = MHD_create_response_from_data(0,NULL,0,0);
			MHD_add_response_header (response, "Sec-WebSocket-Version", "13");
			ret = MHD_queue_response_upgrade (connection,MHD_HTTP_BAD_REQUEST, response);
			return ret;
		}
	}
	if( ret != MHD_NO )
	{
		connection->ws_upgraded = MHD_YES;
		WS_start_connection(wsc);
	}

	return ret;
}

int
WS_start_connection( ws_connection *wsc )
{


	if (!wsc)
	{
		printf("Errore wsc ws_start_connection\n");
		exit(1);
	}

	/* ---------------------------------------------
	 * 	ADD NEW CONNECTION INTO LIST
	 * ---------------------------------------------*/
	WS_add_connection(wsc);

	/* ---------------------------------------------
	 * 	SET OPTION FOR SOCKET
	 * ---------------------------------------------*/
	WS_init_socket(wsc->sockfd);

	/* ---------------------------------------------
	 * 	WS_READER: FUNCTION TO HANDLE SOCKET
	 * ---------------------------------------------*/
	if ( pthread_create(&wsc->pid_reader,0,WS_reader,wsc) )
		perror("pthread_create reader");

	return MHD_YES;
}
/**
 * Create a new struct for the new connection
 */
ws_connection*
WS_init_connection(ws_server_struct *wss, struct MHD_Connection *connection)
{
	ws_connection* wsc = (ws_connection*) malloc(sizeof(ws_connection));
	memset(wsc, 0, sizeof(ws_connection));

	wsc->wss = wss;
	wsc->connection = connection;
	wsc->buffer = NULL;
	wsc->next = NULL;
	wsc->prev = NULL;
	wsc->sender_struct = NULL;
	wsc->frame=NULL;
	wsc->next = NULL;
	wsc->prev = NULL;

	/*
	 * TODO: Controllare i muutex e riveder il meccanismo di chiusura connessione
	 */
	pthread_mutexattr_t mutex_attr;
	pthread_mutexattr_init(&mutex_attr);
	pthread_mutexattr_settype(&mutex_attr, PTHREAD_MUTEX_RECURSIVE);
	pthread_mutex_init(&wsc->mutex_send, &mutex_attr);
	pthread_mutexattr_destroy(&mutex_attr);

	return wsc;
}
/**
 * param wsc The connection to release
 */
void
WS_destroy_connection(ws_connection *wsc)
{
	if (!wsc)
		return;
	if(NULL != wsc->frame)
	{
		SAFE_FREE(wsc->frame->masked);
		SAFE_FREE(wsc->frame->unmasked);
		SAFE_FREE(wsc->frame);
	}
	SAFE_FREE(wsc->buffer);
	wsc->next = NULL;
	wsc->prev = NULL;
	wsc->cls = NULL;
	wsc->sender_struct = NULL;
	wsc->wss = NULL;
	wsc->connection = NULL;
	wsc->fwc = NULL;
	pthread_mutex_destroy(&wsc->mutex_send);

}
/**
 * Create a new struct for the server Websocket
 * return wss A pointer to the new server struct
 */
ws_server_struct*
WS_init()
{
	ws_server_struct *wss = ALLOC(ws_server_struct, 1);

	memset(wss, 0, sizeof(ws_server_struct));

	if (wss != NULL)
	{
		wss->head = NULL;
		wss->tail = NULL;
		pthread_mutex_init(&wss->mutex_list, NULL);
		pthread_mutex_init(&wss->write_to_all, NULL);


		return wss;
	}
	else
		return wss;
}
void
WS_destroy(ws_server_struct* wss)
{
	ws_connection *this = wss->head;
	ws_connection *next ;
	pthread_t t;
	wss->tail = NULL;

	while( this )
	{
		t = this->pid_reader;
		next = this->next;
		pthread_cancel(t);
		pthread_join(t, NULL);
		perror("pthread_join");
		this = next;
	}

	pthread_mutex_destroy(&wss->write_to_all);
	pthread_mutex_destroy(&wss->mutex_list);

	wss->head = NULL;

	free(wss);
}
int
WS_close_MHD_connection(ws_connection *wsc)
{
	struct MHD_Daemon *daemon;
	struct MHD_Connection *connection = wsc->connection;

	daemon = connection->daemon;

	SHUTDOWN (connection->socket_fd,
			(connection->read_closed == MHD_YES) ? SHUT_WR : SHUT_RDWR);
	connection->state = MHD_CONNECTION_CLOSED;
	if ( (NULL != daemon->notify_completed) &&
			(MHD_YES == connection->client_aware) )
		daemon->notify_completed (daemon->notify_completed_cls,
				connection,
				&connection->client_context,
				MHD_HTTP_OK);
	connection->client_aware = MHD_NO;

	return MHD_YES;
}
