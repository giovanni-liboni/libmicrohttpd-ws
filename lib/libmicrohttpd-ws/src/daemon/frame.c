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
* @file frame.c
* @author Giovanni Liboni
*/
#include "websocket.h"
#include <assert.h>

#define CHECK_BIT(var,pos) ((var) & (1<<(pos)))
#define SET_BIT(var, pos) ((var) |= 1<<(pos))
#define CLEAR_BIT(var,pos) ((var) &= ~(1 << pos))

/*
 *  04 logical framing from the spec (all this is masked when incoming
 *  and has to be unmasked)
 *
 * We ignore the possibility of extension data because we don't
 * negotiate any extensions at the moment.
 *
 *    0                   1                   2                   3
 *    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
 *   +-+-+-+-+-------+-+-------------+-------------------------------+
 *   |F|R|R|R| opcode|M| Payload len |    Extended payload length    |
 *   |I|S|S|S|  (4)  |A|     (7)     |             (16/64)           |
 *   |N|V|V|V|       |S|             |   (if payload len==126/127)   |
 *   | |1|2|3|       |K|             |                               |
 *   +-+-+-+-+-------+-+-------------+ - - - - - - - - - - - - - - - +
 *   |     Extended payload length continued, if payload len == 127  |
 *   +- - - - - - - - - - - - - - - -+-------------------------------+
 *   |                               |         Extension data        |
 *   +-------------------------------+-------------------------------+
 *   |    Masking-Key (continued)    |         Payload Data          |
 *   +---------------------------------------------------------------+
 *   :                       Payload Data continued...               :
 *   +---------------------------------------------------------------+
 *
 *  We pass payload through to userland as soon as we get it, ignoring
 *  FIN.  It's up to userland to buffer it up if it wants to see a
 *  whole unfragmented block of the original size (which may be up to
 *  2^63 long!)
 */
int set_length(long long data_length, unsigned char *message)
{
	/* --- Payload lenght --- */

	// mask == 0
	if( data_length <= 125 ) {
		// 0*** ****
		message[1] = ( unsigned char )data_length;

		// start from the byte number 2
		return 2;

	} else if( data_length > 125 && data_length <= 65535 ) {
		// 0111 1110
		message[1] = 126;
		//
		message[2] = ( unsigned char ) ( ( data_length >> 8 ) & 255 );

		message[3] = ( unsigned char ) ( ( data_length ) & 255 );

		// start from the byte number 4
		return 4;

	} else {
		// 0111 1111
		message[1] = 127;
		message[2] = ( unsigned char )( ( data_length >> 56 ) & 255 );
		message[3] = ( unsigned char )( ( data_length >> 48 ) & 255 );
		message[4] = ( unsigned char )( ( data_length >> 40 ) & 255 );
		message[5] = ( unsigned char )( ( data_length >> 32 ) & 255 );
		message[6] = ( unsigned char )( ( data_length >> 24 ) & 255 );
		message[7] = ( unsigned char )( ( data_length >> 16 ) & 255 );
		message[8] = ( unsigned char )( ( data_length >> 8 ) & 255 );
		message[9] = ( unsigned char )( ( data_length ) & 255 );

		// start from the byte number 10
		return 10;
	}
}
/*
void ws_StringToFrame( ws_data_frame *data_frame )
@data_frame Struct to parse
 */
void
ws_StringToFrame( ws_data_frame *data_frame ) {

	unsigned char *message = ( unsigned char * )malloc( FRAME_SIZE * sizeof( char ) );
	assert ( message != NULL );

	int i;
	int data_start_index;

	message[0] = 0;
	switch(data_frame->opcode)
	{
	// continuation frame
	case 0: break;

	// text frame
	case 1: SET_BIT(message[0],0); break;

	// binary frame
	case 2: SET_BIT(message[0],1); break;

	// connection close
	case 8: SET_BIT(message[0],3); break;

	// ping frame
	case 9: SET_BIT(message[0],3); SET_BIT(message[0],0); break;

	// pong frame
	case 10: SET_BIT(message[0],3); SET_BIT(message[0],1); break;
	}

	/*
	 * Set FIN bit
	 */
	if( data_frame->fin)
		SET_BIT(message[0],7);

	if ( data_frame->opcode == WS_CONTINUATION || data_frame->opcode == WS_TEXT || data_frame->opcode == WS_BINARY)
	{
		/* --- set the payload length --*/
		data_start_index = set_length(data_frame->len_unmasked, message);

		/* Copy data from the data_start_index */
//		for( i = 0; i < data_frame->len_unmasked; i++ )
//			message[ data_start_index + i ] = ( unsigned char )data_frame->unmasked[i];

		memcpy(message + data_start_index, data_frame->unmasked, data_frame->len_unmasked);

		/*
		 * Copy the message on the dst
		 */
//		for( i = 0; i < data_frame->len_unmasked + data_start_index; i++ )
//			data_frame->masked[i] = ( unsigned char )message[ i ];

		memcpy(data_frame->masked, message, data_frame->len_unmasked + data_start_index);

		data_frame->len_masked = data_frame->len_unmasked + data_start_index;

		*( data_frame->masked + data_frame->len_masked) = '\0';
	}
	if( data_frame->opcode == WS_CLOSE )
	{
		data_frame->masked[0] = message[0];
		data_frame->len_masked = sizeof(char);
	}
	if( message ) {
		free( message );
		message = NULL;
	}
}

/*
void ws_FrameToString( ws_data_frame *data_frame )
@data_frame Struct to parse
 */
void
ws_FrameToString( ws_data_frame *data_frame )
{

	unsigned int i, j;
	unsigned char mask[4];
	unsigned int packet_length = 0;
	unsigned int length_code = 0;
	int index_first_mask = 0;
	int index_first_data_byte = 0;

	data_frame->fin = data_frame->masked[0] & WS_FIN;

	if((data_frame->masked[0] & WS_CLOSE) == WS_CLOSE)
	{
		/* WebSocket client disconnected */
		data_frame->opcode = 8;
		//return;
	}
	else if( ( data_frame->masked[0] & WS_PING) == WS_PING)
	{
		/* Ping frame 1000 1001*/
		data_frame->opcode = 9;
		//return;
	}
	else if( (data_frame->masked[0] & WS_PONG) == WS_PONG)
	{
		/* Pong frame 1000 1010*/
		data_frame->opcode = 10;
		//return;
	}

	else if (data_frame->masked[0] & WS_TEXT)
	{
		data_frame->opcode = 1;
	}
	/* Unknown error */
	else
	{
		data_frame->opcode = -1;
		return;
	}


	length_code = ((unsigned char) data_frame->masked[1]) & 127;

	if( length_code <= 125 )
	{
		index_first_mask = 2;

		mask[0] = data_frame->masked[2];
		mask[1] = data_frame->masked[3];
		mask[2] = data_frame->masked[4];
		mask[3] = data_frame->masked[5];
	}
	else if( length_code == 126 )
	{
		index_first_mask = 4;

		mask[0] = data_frame->masked[4];
		mask[1] = data_frame->masked[5];
		mask[2] = data_frame->masked[6];
		mask[3] = data_frame->masked[7];
	}
	else if( length_code == 127 )
	{
		index_first_mask = 10;

		mask[0] = data_frame->masked[10];
		mask[1] = data_frame->masked[11];
		mask[2] = data_frame->masked[12];
		mask[3] = data_frame->masked[13];
	}

	index_first_data_byte = index_first_mask + 4;

	packet_length = data_frame->len_masked - index_first_data_byte;

	for( i = index_first_data_byte, j = 0; i < data_frame->len_masked; i++, j++ )
		data_frame->unmasked[ j ] = ( unsigned char )data_frame->masked[ i ] ^ mask[ j % 4 ];

	data_frame->len_unmasked = packet_length;
	return;
}
