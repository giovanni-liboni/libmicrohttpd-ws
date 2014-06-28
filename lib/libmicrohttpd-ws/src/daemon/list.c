/*
     This file is part of libuhttpd-ws
     (C) 2013 Giovanni Liboni

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
 * list.c
 *      Author: giovanni
 *
 * Funzioni per la gestione della lista
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
