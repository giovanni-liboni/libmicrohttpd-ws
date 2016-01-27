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
  * @file parser.c
  * @author Giovanni Liboni
  */
#include "websocket.h"

int
stringToUtf8(char *in, char *out)
{
	if( in != NULL || out != NULL )
	{
		while (*in)
			if (*in<128) *out++=*in++;
			else *out++=0xc2+(*in>0xbf), *out++=(*in++&0x3f)+0x80;

		return 1;	}
	else {
		out = NULL;
		return 0;
	}
}
int is_utf8(unsigned char *str, size_t len)
{
	size_t i = 0;
	size_t continuation_bytes = 0;

	while (i < len)
	{
		if (str[i] <= 0x7F)
			continuation_bytes = 0;
		else if (str[i] >= 0xC0 /*11000000*/ && str[i] <= 0xDF /*11011111*/)
			continuation_bytes = 1;
		else if (str[i] >= 0xE0 /*11100000*/ && str[i] <= 0xEF /*11101111*/)
			continuation_bytes = 2;
		else if (str[i] >= 0xF0 /*11110000*/ && str[i] <= 0xF4 /* Cause of RFC 3629 */)
			continuation_bytes = 3;
		else
			return i + 1;
		i += 1;
		while (i < len && continuation_bytes > 0
				&& str[i] >= 0x80
				&& str[i] <= 0xBF)
		{
			i += 1;
			continuation_bytes -= 1;
		}
		if (continuation_bytes != 0)
			return i + 1;
	}
	return 0;
}
void
liblog ( const char *format, ... )
{
	va_list arg;
	int done;
	char *string = malloc( sizeof(char) * SIZE );
	memset(string,0,SIZE*sizeof(char));

	strcat( string, "-- LIBMICROHTTPD WS -- ");
	strcat( string, format);
	strcat (string, "\n");
	va_start (arg, string);
	done = vfprintf (stdout, string, arg);
	va_end (arg);
	free(string);
	return done;
}
/*
 * Faccio il push
 */
int
WS_send( struct MHD_Connection *connection, char *data, int len, int type )
{


	if ( NULL == data || len <= 0 || NULL == connection || 	is_utf8(data,len) )
		return MHD_NO;

	if ( connection->daemon->wss == NULL || connection->ws_upgraded == MHD_NO )
		return MHD_NO;

	ws_connection *wsc = connection->wsc;

	switch(type)
	{
	case WS_TEXT:
		wsc->frame->opcode = WS_TEXT;
		break;
	case WS_BINARY:
		wsc->frame->opcode = WS_BINARY;
		break;
	default:
		liblog("Error on parsing string to send!");
		return MHD_NO;
	}
	if(wsc->frame->masked == NULL)
		return MHD_NO;

	if (  wsc->cs( wsc,WS_NOT_TO_ALL,data,len,type ) <= 0)
	{
		liblog("Errore durante la scrittura");
		return MHD_NO;
	}
	if(wsc->frame->masked != NULL){
		RESET_BUFFER(wsc->frame->masked, SIZE);
	}

	return MHD_YES;
}

ws_connection *
WS_create_sender_struct( int sockfd, ws_server_struct *wss, Callback_handle_ws_send snd ,
		Callback_send cs, void* cls)
{
	ws_connection *wsc = ALLOC(ws_connection,1);

	wsc->wss = wss;
	wsc->sockfd = sockfd;

	wsc->cls = cls;
	wsc->snd = snd;
	wsc->cs = cs;

	/* ---------------------------------------------
	 * 	ALLOCATE SPACE TO STORE DATA FRAME
	 * ---------------------------------------------*/
	wsc->frame = malloc(sizeof(ws_data_frame));

	wsc->frame->unmasked = ALLOC(char, SIZE);
	wsc->frame->masked = ALLOC(char, SIZE);
	wsc->buffer = ALLOC(char, SIZE);

	/* ---------------------------------------------
	 * 	MUTEX INIT
	 * ---------------------------------------------*/
	pthread_mutex_init(&wsc->mutex_send, NULL);

	return wsc;
}
void
WS_destroy_sender_struct(ws_connection *wsc)
{
	SAFE_FREE(wsc->buffer);
	SAFE_FREE(wsc->frame->masked);
	SAFE_FREE(wsc->frame->unmasked);
	SAFE_FREE(wsc->frame);
	pthread_mutex_destroy(&wsc->mutex_send);
	SAFE_FREE(wsc);
}
int
WS_recv( ws_connection *wsc )
{
	// reset buffer
	RESET_BUFFER(wsc->frame->unmasked , SIZE);
	RESET_BUFFER(wsc->frame->masked , SIZE);
	wsc->frame->len_masked = 0;
	wsc->frame->len_unmasked = 0;

	wsc->frame->len_masked = recv( wsc->sockfd, wsc->frame->masked, SIZE, MSG_NOSIGNAL );

	if ( wsc->frame->len_masked < 0)
	{
		perror("RECV");
		liblog("Frame masked       :  %d", wsc->frame->len_masked);
		liblog("Connection closed");
		wsc->on = 0;
		wsc->status = MHD_NO;
		return MHD_NO;
	}
	else if ( wsc->frame->len_masked == 0 )
	{
		printf("-- LIBMICROHTTPD WS -- Connection closed\n");
		wsc->on = 0;
		wsc->status = MHD_NO;
		return MHD_NO;
	}
	else if( wsc->frame->len_masked >= SIZE )
	{
		liblog("Error: Max SIZE is %dB", SIZE);
		return MHD_NO;
	}

	return MHD_YES;
}
int
WS_parse_incoming_frame( ws_connection *wsc )
{

	switch(wsc->frame->opcode)
	{
	case WS_CLOSE:
		liblog("Close frame received");
		wsc->status_connection = WS_RIGHT_CLOSING;
		return MHD_NO;

	case WS_PING:

		liblog("Ping Frame received");
		int control_frame = 138;
		LOCK(&wsc->mutex_send);
		if(send(wsc->sockfd, (char *) &control_frame, sizeof(int), MSG_NOSIGNAL) < 0)
			perror("send");
		UNLOCK(&wsc->mutex_send);
		break;

	case WS_PONG:

		liblog("Pong Frame received");
		break;

		//	case WS_CONTINUATION:
		//
		//		liblog("Parse other fragments");
		//
		//		len_total_frag += wsc->frame->len_unmasked;
		//		wsc->buffer = (unsigned char *)realloc((unsigned char *)wsc->buffer, len_total_frag*(sizeof(char)));
		//
		//		strncpy((char *)wsc->buffer + ( len_total_frag - wsc->frame->len_unmasked),
		//				(char *) wsc->frame->unmasked, wsc->frame->len_unmasked);
		//
		//		if(wsc->frame->fin)
		//		{
		//			// chiamata alla callback per il parsing dei dati ricevuti
		//			if ( wsc->rcv(wsc->cls, wsc->buffer, len_total_frag, wsc->frame->masked, SIZE ,0) > 0)
		//			{
		//				switch(wsc->frame->opcode)
		//				{
		//				case WS_TEXT:
		//				{
		//					wsc->frame->opcode = WS_TEXT;
		//					to_all = 0;
		//					break;
		//				}
		//				}
		//				if(wsc->frame->masked == NULL)
		//					break;
		//
		//				if (wsc->cs(wsc,to_all,wsc->frame->masked,wsc->frame->len_masked,wsc->frame->opcode) < 0)
		//					printf("cs return < 0\n");
		//
		//				SAFE_FREE(wsc->buffer);
		//			}
		//		}
		//		break;

		// TEST IF THE FRAME IS A TEXT OR BINARY FRAME
	case WS_TEXT:
		// control if it is the first fragment
		//		if(!wsc->frame->fin)
		//		{
		//
		//			liblog("Parse the first fragments");
		//
		//			len_total_frag += wsc->frame->len_unmasked;
		//			wsc->buffer = ALLOC(unsigned char, len_total_frag);
		//			strncpy((char *)wsc->buffer, (char *)wsc->frame->unmasked, len_total_frag);
		//			break;
		//		}

		if( wsc->frame->unmasked[ wsc->frame->len_unmasked - 1] != '\0' )
		{
			wsc->frame->unmasked[wsc->frame->len_unmasked]='\0';
			wsc->frame->len_unmasked+=1;
		}


		// control if string is a valid utf-8 string (0return MHD_YES; if it's valid )
		if(is_utf8(wsc->frame->unmasked, wsc->frame->len_unmasked))
		{
			liblog("Stringa non valida.");
			return MHD_NO;
		}
		else
			return MHD_YES;
		// inviare codice errore sul socket
		break;
		// continue to parse incoming string
	case WS_BINARY:

		break;

	default:
		liblog("Opcode error : %d\n", wsc->frame->opcode);
		wsc->status = WS_ERROR_PARSING_OPCODE;
		return MHD_NO;
		break;

	}// end switch

	return MHD_YES;
}
int
WS_parse_data( ws_connection *wsc , int option )
{
	int to_all;
	// call to the callback for parsing the data received
	if ( (wsc->frame->len_masked =
			wsc->rcv( wsc->cls,
					wsc->buffer,
					wsc->len_buffer,
					wsc->frame->masked,
					SIZE ,&option)) >= 0)
	{
		if( wsc->frame->len_masked == MHD_NO ) return MHD_YES;
		if( wsc->frame->masked == NULL)	return MHD_YES;

		switch(option)
		{
		case WS_BINARY_TO_ALL:
			to_all = 1;
			wsc->frame->opcode = WS_BINARY;
			break;
		case WS_TEXT_TO_ALL:
			to_all = 1;
			wsc->frame->opcode = WS_TEXT;
			break;
		case WS_TEXT:
			wsc->frame->opcode = WS_TEXT;
			to_all=0;
			break;
		case WS_BINARY:
			to_all=0;
			wsc->frame->opcode = WS_BINARY;
			break;
		default:
			printf("Error on parsing string to send!\n");
			break;
		}

		if (  wsc->cs(wsc,to_all,wsc->frame->masked,wsc->frame->len_masked,wsc->frame->opcode) <= 0)
		{
			liblog("Errore durante la scrittura");
			return MHD_NO;
		}
		if(wsc->frame->masked != NULL){
			RESET_BUFFER(wsc->frame->masked, SIZE);
			wsc->frame->len_masked = 0;
		}
	}
	return MHD_YES;
}
static void
cleanup_handler(void *arg)
{
	ws_connection *wsc = (ws_connection *)arg;
	int s;
	void *unused;
	int rc;

	liblog("Called clean-up handler");
	liblog("Closing connection %d", wsc->sockfd );

	if( wsc->status > 0 || wsc->status_connection == WS_RIGHT_CLOSING )
	{
		// Send to client the Close Frame
		LOCK(&wsc->mutex_send);

		pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
		if ( wsc->cs( wsc , 0 , "1000" , 5 , WS_CLOSE ) <= 0 )
			liblog("cs return < 0");
		pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);

		liblog("Close frame sent");

		UNLOCK(&wsc->mutex_send);
	}

	ws_server_struct *wss = wsc->wss;
	SAFE_FREE( wsc->buffer );

	// free user data
	if ( wsc->fwc != NULL && wsc->cls != NULL )
		wsc->fwc(wsc->cls);

	if ( NULL != wsc->snd )
	{
		wsc->sender_struct->sender=0;

		pthread_cancel(wsc->sender_struct->pid_sender);
		s = pthread_join(wsc->sender_struct->pid_sender, NULL);
		if (s < 0)
			perror("pthread_join sender");

		WS_destroy_sender_struct(wsc->sender_struct);
	}

	if( NULL != wsc->connection && wsc->connection->state != MHD_CONNECTION_CLOSED )
	{
		wsc->connection->read_closed = MHD_NO;
		wsc->connection->ws_upgraded = MHD_NO;
		MHD_connection_close (wsc->connection, MHD_REQUEST_TERMINATED_COMPLETED_OK);
		liblog("Closing MHD_Connection");

//		WS_destroy_MHD_connection( wsc->connection );
	}

	// Remove the connection from the list
	WS_remove_connection( (ws_connection *)arg , wss);
}
void
*WS_reader(void *arg)
{
	/*
	 * Bisogna gestire i messaggi entranti con una FSM.
	 * i messaggi dovrebbero passare un primo controllo sul tipo
	 * 					-> se sono di controllo allora devono essere gestiti dalla libreria
	 * 					-> altrimenti si vede se il frame è frammentato
	 * 					-> quando si ha il messaggi nella sua interezza allora si passa alla callback
	 * 					-> la callback dovrebbe chiamare una funzione adetta per l'invio di messaggi per quel socket
	 *
	 *
	 */
	int control_frame;
	ws_connection *wsc = (ws_connection *) arg;
	int option;

	/*
	 * For Fragments
	 */
	int len_total_frag = SIZE;
	int to_all;
	int parse_frame=0;

	pthread_detach(wsc->pid_reader);
	pthread_cleanup_push( cleanup_handler , wsc );

	wsc->cs = callback_send;
	wsc->sender = MHD_NO;
	wsc->status = MHD_YES;

	/* ---------------------------------------------
	 * 	ALLOCATE SPACE TO STORE DATA FRAME
	 * ---------------------------------------------*/
	wsc->frame = ALLOC(ws_data_frame,1);
	wsc->frame->unmasked = ALLOC(char, SIZE);
	wsc->frame->masked = ALLOC(char, SIZE);

	if ( NULL != wsc->snd )
	{
		pthread_attr_t attr;
		pthread_attr_init(&attr);
		pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
		// Initialize the structure
		wsc->sender_struct = WS_create_sender_struct(wsc->sockfd,wsc->wss,wsc->snd,wsc->cs,wsc->cls);

		if ( pthread_create(&wsc->sender_struct->pid_sender,&attr,sender,wsc->sender_struct) )
			perror("pthread_create reader");

		pthread_attr_destroy(&attr);
	}
	while(wsc->on)
	{

		pthread_testcancel();

		if ( WS_recv( wsc ) == MHD_NO )
			goto end;
		else
		{
			// parse frame into string
			ws_FrameToString(wsc->frame);

			liblog("Payload: %s", wsc->frame->unmasked);

			// TEST IF THE FRAME IS A CONTROL FRAME


			if ( WS_parse_incoming_frame( wsc ) )
			{
				wsc->buffer = ( char * ) malloc( sizeof( char ) * wsc->frame->len_unmasked );
				memcpy(wsc->buffer, wsc->frame->unmasked, sizeof( char ) * wsc->frame->len_unmasked);
				wsc->len_buffer = wsc->frame->len_unmasked;

				if ( WS_parse_data( wsc, option ) == MHD_YES )
				{
					SAFE_FREE( wsc->buffer );
					parse_frame = 0;
				}
				else
				{
					SAFE_FREE( wsc->buffer );
					goto end;
				}
			}
			else
			{
				if( wsc->status_connection == WS_RIGHT_CLOSING )
					goto end;
			}
		}
	}// end while

	end:
	pthread_cleanup_pop(MHD_YES);

	return NULL;
}
void
*sender(void *arg)
{

	ws_connection *wsc = (ws_connection *) arg;
	pthread_detach(wsc->pid_sender);
	wsc->sender = 1;

	liblog("Sender ready.");
	while(wsc->sender)
	{
		wsc->len_buffer = wsc->snd( wsc->cls, wsc->buffer, SIZE, &wsc->frame->opcode );

		if( wsc->len_buffer > 0 )
		{
			if(!wsc->sender){
				goto exit;
			}
			else {
				switch(wsc->frame->opcode)
				{
				case WS_TEXT:
				{
					wsc->frame->opcode = WS_TEXT;
					break;
				}
				case WS_TEXT_TO_ALL:
				{
					wsc->frame->opcode = WS_TEXT;
					break;
				}
				}

				/*
				 * User callback
				 */
				if(wsc->buffer != NULL && wsc->len_buffer > 0)
				{
					pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
					if ( ( wsc->status = wsc->cs(wsc,0,wsc->buffer,wsc->len_buffer,wsc->frame->opcode)) <= 0)
					{
						liblog("Errore invio dati sender.");
						liblog("Errore : %d\n", wsc->status);
						goto exit;
					}
					pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
				}
			}
		}
	}
	exit:
	liblog("Sender of %d exit \n", wsc->sockfd);
	return NULL;
}
