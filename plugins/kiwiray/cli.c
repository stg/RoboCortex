#include "cli.h" // This is a client plugin
#include <stdbool.h>

static pluginclient_t  kiwiray;         // Plugin descriptor
static pluginhost_t   *host;            // RoboCortex descriptor

static           char  p_text[ 256 ];   // Input buffer
static           char  l_text = -1;     // Current size of p_text

// Hooks were lost, reset input
static void lost_hooks() {
  host->text_clear( 1, host->text_rows - 2, 38 );
  host->text_crem();
  l_text = -1;
}

// Handles keyboard events
static void key_event( int event, int key, char ascii ) {
  unsigned char n;
  bool emo;
  if( event == E_KEYDOWN ) {
    if( l_text >= 0 ) {
      switch( key ) {
        case SDLK_BACKSPACE:
          // Remove last character
          if( l_text > 0 ) {
            p_text[ --l_text ] = 0;
            host->text_clear( l_text + 2, host->text_rows - 2, 1 );
            host->text_cins( l_text + 2, host->text_rows - 2 );
            break;
          }
          //...

        case SDLK_RETURN:
          if( l_text > 0 ) {
            // Send to server
            memset( p_text + strlen( p_text ), 0, 255 - strlen( p_text ) );
            host->server_send( p_text, strlen( p_text ) );
            // Queue for local playback
            if( p_text[ 0 ] != '/' ) {

              for( n = 0; n < strlen( p_text ); n++ ) {
                if( p_text[ n ] == ':' ) {
                  emo = true;
                  switch( p_text[ n + 1 ] ) {
                    case ')': 
                    case '(': 
                    case '|':
                      break;
                    default:
                      emo = false;
                  }
                  if( emo ) {
                    p_text[ n ] = ' ';
                    p_text[ n + 1 ] = ' ';
                  }
                }
              }
              
              host->speak_text( p_text );
            }
          }
          //...

        case SDLK_ESCAPE:
          // Abort text input
          p_text[ l_text ] = 0;
          host->keyboard_release();
          lost_hooks();
          break;

        default:
          // Attempt to insert character
          if( host->text_valid( ascii ) && l_text < 37 ) {
            p_text[ l_text++ ] = ascii;
            p_text[ l_text ] = 0;
            host->text_write( l_text + 1, host->text_rows - 2, &p_text[ l_text - 1 ], FONT_GREEN );
            host->text_cins( l_text + 2, host->text_rows - 2 );
          }

      }
    } else if( key == SDLK_t ) {
      if( host->keyboard_hook() ) {
        l_text = 0;
        p_text[ 0 ] = 0x00;
        host->text_write( 1, host->text_rows - 2, ">", FONT_GREEN );
        host->text_cins( 2, host->text_rows - 2 );
      }
    } else if( key == SDLK_m ) {
      host->server_send( "/MIRROR", 7 );
    } else if( key == SDLK_v ) {
      host->server_send( "/VOICE", 6 );
    } else if( key == SDLK_i ) {
      host->server_send( "/LIGHT", 6 );
    }
  }
}

// Initializes hooks and help
static void init() {
  host->key_bind( SDLK_t );
  host->key_bind( SDLK_m );
  host->key_bind( SDLK_v );
  host->key_bind( SDLK_i );
  host->help_add( "T: OPEN SPEECH/COMMAND PROMPT" );
  host->help_add( "V: SET LANGUAGE/VOICE FOR SPEECH" );
  host->help_add( "M: TOGGLE REAR-VIEW MIRROR" );
  host->help_add( "I: TOGGLE ILLUMINATION" );
}

// Handles messages from server plugin
static void message( void* data, unsigned char size ) {
  char msg[ 256 ];
  char *p;
  memcpy( msg, data, size );
  msg[ size ] = 0;

  if( memcmp( msg, "LANGUAGE/VOICE: ", 16 ) == 0 ) {
    p = strchr( msg + 16, ':' );
    *p = 0;
    host->speak_voice( p + 1 );
    host->draw_message( msg );
  } else {
    host->draw_message( msg );
  }
}

// Sets up the plugin descriptor
pluginclient_t *kiwiray_open( pluginhost_t *p_host ) {
  memcpy( &kiwiray.ident, "KIWI", 4 );
  host = p_host;
  kiwiray.init       = init;
  kiwiray.keyboard   = key_event;
  kiwiray.lost       = lost_hooks;
  kiwiray.recv       = message;
  return( &kiwiray );
}
