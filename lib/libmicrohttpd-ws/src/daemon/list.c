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
