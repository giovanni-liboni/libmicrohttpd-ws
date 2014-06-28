/*
 * main.c
 *      Author: Giovanni Liboni
 */
#include "microhttpd.h"
#include <time.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <stdlib.h>

time_t current_time;
char* c_time_string;
int cont = 0;
char PATH[1024];

#define FILENAME_GLUE "."
#define EXTENSION_SVG ".svg"
#define EXTENSION_JPG ".jpg"
#define EXTENSION_JPEG ".jpeg"
#define EXTENSION_JS ".js"
#define EXTENSION_PNG ".png"
#define EXTENSION_BMP ".bmp"
#define EXTENSION_TIFF ".tiff"
#define EXTENSION_HTML ".html"
#define EXTENSION_HTM ".htm"
#define EXTENSION_CSS ".css"
#define EXTENSION_GIF ".gif"
#define EXTENSION_XML ".xml"

#define MIME_DEFAULT "text/plain"
#define MIME_SVG "image/svg+xml"
#define MIME_JPEG "image/jpeg"
#define MIME_JS "text/javascript"
#define MIME_PNG "image/png"
#define MIME_BMP "image/bmp"
#define MIME_TIFF "image/tiff"
#define MIME_HTML "text/html"
#define MIME_CSS "text/css"
#define MIME_GIF "image/gif"
#define MIME_XML "text/xml"

#define JSON_IDENTIFIER_GLUE '.'
#define JSON_IDENTIFIER_FIELD "id"
#define JSON_EVENT_FIELD "event"

#define HTTP_ERROR_PAGE "<html><head><title>Microhttpd Connector - ERROR 404</title></head><body>File not found</body></html>"
#define HTTP_COMET_PATH "comet"
#define HTTP_DEFAULT_DOCUMENT "ws_echo.html"
#define HTTP_SERVER_NAME "radGUI-Connector\2.0"
#define HTTP_ACCEPT_RANGES "bytes"
#define HTTP_DEFAULT_MIMETYPE "text/plain"

#define HTTP_POST_PARAMETER "json"
#define HTTP_POST_RESPONSE "OK"

int handleFileGet( char * c, struct MHD_Connection* connection, const char *url );
ssize_t fileReader( void *cls, uint64_t pos, char *buf, size_t max );
void freeFileCallback( void *cls );
const char * getFileMimeType( const char * filename );
void addExtraHeaders( struct MHD_Response *response, size_t size,
		const char * mime_type );

char *str;
/**
Option:
WS_TEXT
*/
/*
 * CALLBACK PER LA RICEZIONE DEI DATI DAL CANALE
 */
static int
ws_handle_recv( void *cls, char *data_in, int len_data_in,
		void *data_out, int MAX_LEN_DATA, int *option)
{

	WS_send((struct MHD_Connection*) cls, data_in,len_data_in, WS_TEXT );

	return MHD_NO;
}
/*
* CALLBACK PER INVIARE DATI SUL CANALE
*/
int
ws_handle_send( void *cls, char *data_out, int MAX_LEN_DATA, int *option )
{
	static int i_private=0;
	sleep(1);
	char *string=(char *)malloc(sizeof(char)* 256);

	sprintf(string,"Numero inviato dal server %d\n", i_private);
	strcpy((char *)data_out, string);
	i_private++;

	int len = strlen(string);

	SAFE_FREE(string);
	// OPZIONE PER SPECIFICARE LA TIPOLOGIA DEL DATO DA INVIARE SUL CANALE
	*option = WS_TEXT;

	// BISOGNA RITORNARE LA LUNGHEZZA DEI DATI DA INVIARE
	return len;
}
/*
* CALLBACK PER LIBERARE LA MEMORIA LEGATA AL PUNTATORE PASSATO CLS
*/
void
ws_free_callback( void *cls)
{
	free( cls);
}
static int
handle_response (void *cls,
		struct MHD_Connection *connection,
		const char *url,
		const char *method,
		const char *version,
		const char *upload_data, size_t *upload_data_size, void **ptr)
{
	static int aptr;
	int ret = MHD_NO;

	if (&aptr != *ptr)
	{
		/* do never respond on first call */
		*ptr = &aptr;
		return MHD_YES;
	}

    if ( MHD_YES == WS_upgrade_request( connection ) )
    {

	    ret = WS_start( connection,
	    		  ws_handle_recv,
				  NULL,
				  NULL, connection);
	    return ret;
    }

	if ( strcmp( method, MHD_HTTP_METHOD_GET ) == 0 )
	{
		*ptr = NULL;
        return handleFileGet( PATH, connection, url );
	}
	*ptr = NULL;                  /* reset when done */
	return ret;
}
ssize_t fileReader( void *cls, uint64_t pos, char *buf, size_t max )
{
	FILE *file = (FILE*) cls;
	(void) fseek( file, pos, SEEK_SET );
	return fread( buf, 1, max, file );
}
void freeFileCallback( void *cls )
{
	FILE *file = (FILE*) cls;
	fclose( file );
}
const char * getFileMimeType( const char * filename )
{
	char * start = strstr( filename, FILENAME_GLUE );

	if ( 0 == strcmp( start, EXTENSION_SVG ) )
		return MIME_SVG;
	if ( 0 == strcmp( start, EXTENSION_JS ) )
		return MIME_JS;
	if ( 0 == strcmp( start, EXTENSION_CSS ) )
		return MIME_CSS;
	if ( 0 == strcmp( start, EXTENSION_JPG ) )
		return MIME_JPEG;
	if ( 0 == strcmp( start, EXTENSION_JPEG ) )
		return MIME_JPEG;
	if ( 0 == strcmp( start, EXTENSION_HTML ) )
		return MIME_HTML;
	if ( 0 == strcmp( start, EXTENSION_HTM ) )
		return MIME_HTML;
	if ( 0 == strcmp( start, EXTENSION_TIFF ) )
		return MIME_TIFF;
	if ( 0 == strcmp( start, EXTENSION_BMP ) )
		return MIME_BMP;
	if ( 0 == strcmp( start, EXTENSION_XML ) )
		return MIME_XML;
	if ( 0 == strcmp( start, EXTENSION_PNG ) )
		return MIME_PNG;
	if ( 0 == strcmp( start, EXTENSION_GIF ) )
		return MIME_GIF;

	return MIME_DEFAULT;
}
void addExtraHeaders( struct MHD_Response *response, size_t size,
		const char * mime_type )
{
	MHD_add_response_header( response, MHD_HTTP_HEADER_SERVER, HTTP_SERVER_NAME );
	MHD_add_response_header( response, MHD_HTTP_HEADER_ACCEPT_RANGES,
			HTTP_ACCEPT_RANGES );
	MHD_add_response_header( response, MHD_HTTP_HEADER_CONTENT_TYPE, mime_type );
}
int handleFileGet( char * c,
		struct MHD_Connection* connection, const char *url )
{
	struct MHD_Response *response;
	int ret;
	struct stat buf;
	unsigned int response_status;
	char * filename;
	int filenameLen = 0;

	if ( strcmp( "/", url ) == 0 )
	{
		filenameLen = strlen( c ) + strlen( url )
                		+ strlen( HTTP_DEFAULT_DOCUMENT ) + 1;
		filename = (char*) malloc( sizeof(char) * filenameLen );
		strcpy( filename, c );
		strcat( filename, url );
		strcat( filename, HTTP_DEFAULT_DOCUMENT );
	}
	else
	{
		filenameLen = strlen( c ) + strlen( url ) + 1;
		filename = (char*) malloc( sizeof(char) * filenameLen );
		strcpy( filename, c );
		strcat( filename, url );
	}

	printf( "Microhttpd-connector - Requested file:\n%s\n", filename );

	if ( stat( filename, &buf ) )
	{
		printf( "Microhttpd-connector - File not found:\n%s\n", filename );

		response = MHD_create_response_from_buffer( strlen( HTTP_ERROR_PAGE ),
				(void *) HTTP_ERROR_PAGE, MHD_RESPMEM_PERSISTENT );
        addExtraHeaders( response, strlen( HTTP_ERROR_PAGE ), MIME_HTML );
		response_status = MHD_HTTP_NOT_FOUND;
	}
	else
	{
		FILE *file = fopen( filename, "rb" );
		response = MHD_create_response_from_callback( buf.st_size, 32 * 1024,
                &fileReader, file, &freeFileCallback );
        addExtraHeaders( response, buf.st_size,
                getFileMimeType( filename ) );
		// Check if error occured during response process
		if ( response == NULL )
		{
			fclose( file );
			free( filename );
			return MHD_NO;
		}
        // fclose called by freeFileCallback function
		response_status = MHD_HTTP_OK;
	}

	/*
	 * Commit response
	 */
	ret = MHD_queue_response( connection, response_status, response );
	MHD_destroy_response( response );

	free( filename );
	return ret;
}int
main (int argc, char *const *argv)
{
	if (getcwd(PATH, sizeof(PATH)) == NULL)
		perror("getcwd() error");
	strcat(PATH, "/example/www");

	struct MHD_Daemon *daemon;

	if (argc != 2)
	{
		printf ("%s PORT\n", argv[0]);
		return 1;
	}
	// ESEMPIO DI FUNZIONAMENTO PER PASSARE UN PUNTATORE GENERICO ALLE CALLBACK
	str = malloc(sizeof(char) * 10);
	strcpy(str, "Ciao\0");

	daemon  = MHD_start_daemon (MHD_USE_THREAD_PER_CONNECTION | MHD_USE_DEBUG,
			atoi (argv[1]),
			NULL, NULL, &handle_response, NULL, MHD_OPTION_END);

	printf("Daemon start...\n");
	(void) getc (stdin);


	free(str);
	MHD_stop_daemon (daemon);


	printf("Daemon END \n");
	return 0;
}
