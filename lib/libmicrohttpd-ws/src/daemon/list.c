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
 * @file list.c
 * @author Giovanni Liboni
 */
#include "websocket.h"
int
WS_add_connection(ws_connection *this)
{
	if(this){
		LOCK(&this->wss->mutex_list);

		if ( this->wss->tail )
		{
			this->wss->tail->next = this;
			this->prev = this->wss->tail;
			this->wss->tail = this;
		}
		else
		{
			this->wss->head = this;
			this->wss->tail = this;
		}
		UNLOCK(&this->wss->mutex_list);
		return 1;
	}
	return -1;
}
int
WS_remove_connection(ws_connection *this, ws_server_struct *wss)
{
	if(this){
		ws_connection *prev = this->prev;
		ws_connection *next = this->next;

		LOCK(&wss->mutex_list);

		if( prev )
		{
			if( next )
			{
				prev->next = next;
				next->prev = prev;
			}
			else
			{
				prev->next = NULL;
				wss->tail = prev;
			}
		}
		else
		{
			if( next )
			{
				next->prev = NULL;
				wss->tail = next;
			}
			else
			{
				wss->tail = NULL;
				wss->head = NULL;
			}
		}
		WS_destroy_connection(this);
		SAFE_FREE(this);

		UNLOCK(&wss->mutex_list);
		return 1;

	}
	return -1;
}
