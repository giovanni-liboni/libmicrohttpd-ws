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
* @file callback.c
* @author Giovanni Liboni
*/
#include "websocket.h"
int
write_on_socket(ws_connection *wsc)
{
	int len_sent = -1;

	if (NULL == wsc)
		return -1;

	ws_StringToFrame(wsc->frame);

	if(wsc->sockfd > 0 && wsc->sockfd < WS_MAX_FD)
	if( ( len_sent = send( wsc->sockfd, wsc->frame->masked, wsc->frame->len_masked, MSG_NOSIGNAL)) <= 0)
				perror("Write_on_socket");

	liblog("Sent frame on %d", wsc->sockfd);
	return len_sent;
}
/**
 * Send a message over websockets sockets. Max lenght of frame is 1024B
 * @param wsc ws_connection
 * @param to_all If set to 1 send message to all client connected
 */
int
ws_my_send(ws_connection *wsc, char *data, int len_data, int type)
{
	int total_sent = 0;
	int ret = 0;

	RESET_BUFFER(wsc->frame->unmasked, SIZE);

	if( FRAME_SIZE >= len_data )
	{

		memcpy(wsc->frame->unmasked, data, len_data * sizeof(char));

		wsc->frame->opcode = type;
		wsc->frame->fin = 1;
		wsc->frame->len_unmasked = len_data;

		if( ( ret = write_on_socket( wsc) ) <= 0 ) return ret;
	}
	else
	{

		// copy into buffer FRAME_SIZE byte
		memcpy(wsc->frame->unmasked, data, FRAME_SIZE * sizeof(char));

		wsc->frame->len_unmasked = FRAME_SIZE;
		wsc->frame->opcode = type;
		wsc->frame->fin = 0;

		total_sent+=FRAME_SIZE;

		if( ( ret = write_on_socket(wsc) ) <= 0 ) return ret;

		do
		{
			// copy into buffer FRAME_SIZE bytes
			if (total_sent + FRAME_SIZE <= len_data)
			{
				strncpy((char *)wsc->frame->unmasked, (char *) (data + total_sent), FRAME_SIZE);
				wsc->frame->len_unmasked = FRAME_SIZE;
				wsc->frame->fin = 0;
			}
			else
			{
				strncpy((char *)wsc->frame->unmasked, (char *) (data + total_sent), (len_data - total_sent));
				wsc->frame->len_unmasked = len_data - total_sent;
				wsc->frame->fin = 1;
			}

			wsc->frame->opcode = 0;

			if( ( ret = write_on_socket(wsc) ) <= 0 ) return ret;

			total_sent+=FRAME_SIZE;

			RESET_BUFFER(wsc->frame->unmasked, SIZE);

		}
		while(!wsc->frame->fin);
	}
	return ret;
}
int
callback_send( ws_connection *wsc, int to_all, void *data, int len_data, int type )
{
	int ret = -1;

	if(wsc  && data )
	{

		/*
		 * Only this method can write over the socket
		 */
		if(wsc->sockfd <= 0  && !wsc->sender)
			return -1;
		if(!to_all)
		{
			LOCK(&wsc->mutex_send);

			if(wsc->sockfd > 0)	ret = ws_my_send(wsc,data,len_data,type);
			else printf("Error = FD: %d TO_ALL: %d\n", wsc->sockfd, to_all);

			UNLOCK(&wsc->mutex_send);
		}
		else
		{
			LOCK(&wsc->wss->write_to_all);
			LOCK(&wsc->wss->mutex_list);

			ws_connection *this;

			this = wsc->wss->head;
			while(this)
			{

				this->cs(this,0,data,len_data,type);
				// save the pointer to the next item
				this = this->next;
			}
#if DBG
			printf("\nData sent to all clients\n\n");
#endif
			UNLOCK(&wsc->wss->mutex_list);
			UNLOCK(&wsc->wss->write_to_all);
		}
	}

	return ret;
}
void
clr(ws_connection *wsc)
{
#if CLR
	// clean up and close connection

	struct MHD_Connection *connection = wsc->connection;
	ws_server_struct *wss = wsc->wss;
	LOCK(&wss->mutex_list);
	if(wsc->wss->list_length == 0)
	{
		wsc->wss->snd_active = 1;
		pthread_cancel(wsc->wss->thread_send);
	}

	WS_list_remove(wsc);
	free_wsc(wsc);
	connection->read_closed = MHD_YES;
	MHD_connection_close(connection,MHD_HTTP_OK);
	UNLOCK(&wsc->mutex_send);
	pthread_mutex_destroy(&wsc->mutex_send);
	SAFE_FREE(wsc);
	UNLOCK(&wss->mutex_list);
	printf("Resource free\n");

	struct MHD_Connection *connection = wsc->connection;
	/*
	 * Clear wsc connection here
	 */
	LOCK(&wsc->mutex_send);
	/* Cancello dalla lista questa connessione */
	WS_list_remove(wsc);
	connection->read_closed = MHD_YES;

	UNLOCK(&wsc->mutex_send);
#endif
}
