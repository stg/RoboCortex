#include "srv.h" // This is a server plugin

#define ROT_DZN                3        // Dead-zone
#define ROT_ACC                6        // Acceleration
#define ROT_DMP                6        // Dampening
#define ROT_SEN              0.5        // Sensitivity
#define ROT_MAX             1000        // Max accumulated rotation

#define MOV_ACC                2        // Acceleration
#define MOV_BRK                5        // Breaking

#define CAM_SEN              0.3        // Sensitivity

#define TIMEOUT_EMOTICON     100        // Before emoticon is removed

// Emoticon list
enum emo_e {
  EMO_IDLE,
  EMO_CONNECTED,
  EMO_HAPPY,
  EMO_ANGRY
};

static pluginclient_t  kiwiray;         // Plugin descriptor
static  pluginhost_t  *host;            // RoboCortex descriptor

static          char   serdev[ 256 ];   // Serial device name

static          char   drive_x;         // Strafe
static          char   drive_y;         // Move
static          char   dj_x;         // Strafe
static          char   dj_y;         // Move
static          char   dk_x;         // Strafe
static          char   dk_y;         // Move
static          char   drive_r;         // Turn
static unsigned  int   drive_p;         // Pitch
static          long   integrate_r;     // Rotational(turn) integration

static          void  *h_thread;        // Communications thread handle
static           int   connected;       // Successfully connected

// Binary literals for emoticons
#define B00000000 0x00
#define B00011000 0x18
#define B00100100 0x24
#define B00111100 0x3c
#define B01000010 0x42
#define B10000001 0x81
#define B11100111 0xe7

// Emoticons
static unsigned char   emoticon;
static unsigned  int   emoticon_timeout;
static           int   timeout_emoticon = TIMEOUT_EMOTICON;
const unsigned  char   emotidata[ 4 ][ 24 ] = {
  {
    B00000000, B00000000, B00000000,
    B00000000, B00000000, B00000000,
    B00000000, B00000000, B00000000,
    B00000000, B00011000, B00000000,
    B00000000, B00011000, B00000000,
    B00000000, B00000000, B00000000,
    B00000000, B00000000, B00000000,
    B00000000, B00000000, B00000000
  }, {
    B00000000, B00000000, B00000000,
    B00000000, B00000000, B00000000,
    B00111100, B00111100, B00111100,
    B00100100, B00100100, B00100100,
    B00100100, B00100100, B00100100,
    B00111100, B00111100, B00111100,
    B00000000, B00000000, B00000000,
    B00000000, B00000000, B00000000
  }, {
    B00000000, B00000000, B00000000,
    B00000000, B11100111, B00000000,
    B00000000, B00000000, B00000000,
    B00000000, B00000000, B00000000,
    B00000000, B10000001, B00000000,
    B00000000, B01000010, B00000000,
    B00000000, B00111100, B00000000,
    B00000000, B00000000, B00000000
  }, {
    B00000000, B00000000, B00000000,
    B01000010, B00000000, B00000000,
    B00100100, B00000000, B00000000,
    B00000000, B00000000, B00000000,
    B00000000, B00000000, B00000000,
    B00111100, B00000000, B00000000,
    B01000010, B00000000, B00000000,
    B00000000, B00000000, B00000000
  }
};

// Thread: manage KiwiRay serial communications
static int commthread() {
  int b_working = 0;
  unsigned char n;
  unsigned char emotilast = 255;
  char p_pkt[ 64 ] = { ( char )0xFF, 0x00, 0x00, 0x00, 0x00, 0x00 };

  // Initial serial startup
  b_working = ( serial_open( serdev ) == 0 );
  if( !b_working ) {
    printf( "KiwiRay [warning]: Unable to open %s, disabling serial\n", serdev );
    return( 1 );
  }
  b_working = !serial_params( "115200,n,8,1" );
  if( !b_working ) {
    printf( "KiwiRay [warning]: Unable to configure %s, disabling serial\n", serdev );
    serial_close();
    return( 1 );
  }
  while( 1 ) {
    // Re-open on errors
    if( !b_working ) {
      printf( "KiwiRay [warning]: Serial port problem, re-opening...\n" );
      host->thread_delay( 5000 );
      serial_close();
      b_working = ( serial_open( serdev ) == 0 );
      if( b_working ) b_working = serial_params( "115200,n,8,1" );
    }
    p_pkt[ 1 ] = 0x00;               // Drive XYZ
    p_pkt[ 2 ] = -drive_x;           // Strafe X
    p_pkt[ 3 ] = -drive_y;           // Move   Y
    p_pkt[ 4 ] =  drive_r;           // Rotate R
    p_pkt[ 5 ] =  drive_p * CAM_SEN; // Look   Pitch
    p_pkt[ 6 ] =  3;                 // Stepsize = 1:2^6
    if( b_working) b_working = ( serial_write( p_pkt, 7 ) == 7 );
    if( emotilast != emoticon ) {
      p_pkt[ 1 ] = 0x33;             // Display
      p_pkt[ 2 ] = 23;               // 8x8x3 bits (-1)
      for( n = 0; n < 24; n++ ) p_pkt[ 3 + n ] = emotidata[ emoticon ][ n ];
      if( b_working) b_working = ( serial_write( p_pkt, 27 ) == 27 );
      emotilast = emoticon;
    }
    host->thread_delay( 20 ); // roughly 50 times second
  }
  return( 0 );
}

// Handles packets received from client plugin
static void process_data( void* p_data, unsigned char size ) {
  int n;
  char data[ 256 ];
  memcpy( data, p_data, size );
  data[ size ] = 0;
  if( data[ 0 ] == '/' ) {
    printf( "KiwiRay [info]: Command: %s\n", data );
    if( strcmp( data, "/MIRROR" ) == 0 ) {
      host->cap_set( 1, CAP_TOGGLE, NULL, NULL );
    } else if( strcmp( data, "/VOICE" ) == 0 ) {
      memcpy( data, "LANGUAGE/VOICE: ", 16 );
      strcpy( data + 16, host->speak_voice( -1 ) );
      host->client_send( data, strlen( data ) );
    } else {
      memmove( data + 17, data, size );
      memcpy( data, "UNKNOWN COMMAND: ", 17 );
      host->client_send( data, size + 17 );
    }
  } else {
    printf( "KiwiRay [info]: Speaking: %s\n", data );
    for( n = 0; n < size - 1; n++ ) {
      if( data[ n ] == ':' ) {
        switch( data[ n + 1 ] ) {
          case ')': emoticon = EMO_HAPPY;
            break;
          case '(': emoticon = EMO_ANGRY;
            break;
        }
        if( emoticon > 1 ) emoticon_timeout = timeout_emoticon;
        data[ n ] = ' ';
        data[ n + 1 ] = ' ';
      }
    }
    host->speak_text( data );
  }
}

// Tick: updates motion and timeouts
static void tick() {
	int temp;
	int tgt_x, tgt_y;

	if( !connected ) return; // Important - host->ctrl/diff not valid!
	
	// Emoticon timeout
	if( emoticon_timeout ) {
	  if( --emoticon_timeout == 0 ) emoticon = EMO_CONNECTED;
	}
/*
  // Handle movement X and Y
  if( host->ctrl->kb & KB_LEFT  ) {
    dk_x = ( dk_x > -( 127 - MOV_ACC ) ? dk_x - MOV_ACC : -127 );
  } else if( dk_x < 0 ) {
    dk_x = ( dk_x < -MOV_BRK ? dk_x + MOV_BRK : 0 );
  }
  if( host->ctrl->kb & KB_RIGHT ) {
    dk_x = ( dk_x <  ( 127 - MOV_ACC ) ? dk_x + MOV_ACC :  127 );
  } else if( dk_x > 0 ) {
    dk_x = ( dk_x >  MOV_BRK ? dk_x - MOV_BRK : 0 );
  }
  if( host->ctrl->kb & KB_UP    ) {
    dk_y = ( dk_y > -( 127 - MOV_ACC ) ? dk_y - MOV_ACC : -127 );
  } else if( dk_y < 0 ) {
    dk_y = ( dk_y < -MOV_BRK ? dk_y + MOV_BRK : 0 );
  }
  if( host->ctrl->kb & KB_DOWN  ) {
    dk_y = ( dk_y <  ( 127 - MOV_ACC ) ? dk_y + MOV_ACC :  127 );
  } else if( dk_y > 0 ) {
    dk_y = ( dk_y >  MOV_BRK ? dk_y - MOV_BRK : 0 );
  }

  if( host->ctrl->dx > dj_x ) dj_x = MIN( dj_x + MOV_ACC, host->ctrl->dx );
  if( host->ctrl->dx < dj_x ) dj_x = MAX( dj_x - MOV_ACC, host->ctrl->dx );
  if( host->ctrl->dy > dj_y ) dj_y = MIN( dj_y + MOV_ACC, host->ctrl->dy );
  if( host->ctrl->dy < dj_y ) dj_y = MAX( dj_y - MOV_ACC, host->ctrl->dy );
  
  drive_x = MAX( MIN( dk_x + dj_x, 127 ), -127 );
  drive_y = MAX( MIN( dk_y + dj_y, 127 ), -127 );

*/
  tgt_x = host->ctrl->dx;
  tgt_y = host->ctrl->dy;
  //printf("A: %i,%i\n",tgt_x,tgt_y);
  if( host->ctrl->kb & KB_LEFT  ) tgt_x -= 127;
  if( host->ctrl->kb & KB_RIGHT ) tgt_x += 127;
  //printf("B: %i,%i\n",tgt_x,tgt_y);
  if( host->ctrl->kb & KB_UP    ) tgt_y -= 127;
  if( host->ctrl->kb & KB_DOWN  ) tgt_y += 127;
  //printf("C: %i,%i\n",tgt_x,tgt_y);
  
  tgt_x = MAX( MIN( tgt_x, 127 ), -127 );
  tgt_y = MAX( MIN( tgt_y, 127 ), -127 );
  
  //printf("D: %i,%i\n",tgt_x,tgt_y);
  
  if( drive_x >= 0 ) {
    if( drive_x > tgt_x ) {
      drive_x = MAX( drive_x - MOV_BRK, tgt_x );
    } else {
      drive_x = MIN( drive_x + MOV_ACC, tgt_x );
    }
  } else {
    if( drive_x < tgt_x ) {
      drive_x = MIN( drive_x + MOV_BRK, tgt_x );
    } else {
      drive_x = MAX( drive_x - MOV_ACC, tgt_x );
    }
  }
  if( drive_y >= 0 ) {
    if( drive_y > tgt_y ) {
      drive_y = MAX( drive_y - MOV_BRK, tgt_y );
    } else {
      drive_y = MIN( drive_y + MOV_ACC, tgt_y );
    }
  } else {
    if( drive_y < tgt_y ) {
      drive_y = MIN( drive_y + MOV_BRK, tgt_y );
    } else {
      drive_y = MAX( drive_y - MOV_ACC, tgt_y );
    }
  }
  

  // Handle movement R
  integrate_r -= ( drive_r * ROT_SEN );
  integrate_r += host->diff->mx;
  integrate_r = MAX( MIN( integrate_r, ROT_MAX ), -ROT_MAX );
  if( integrate_r > ROT_DZN ) {
    drive_r = ( drive_r <  ( 127 - ROT_ACC ) ? drive_r + ROT_ACC :  127 );
    if( drive_r > integrate_r / ROT_DMP + ROT_DZN ) drive_r = integrate_r / ROT_DMP + ROT_DZN;
  } else if( integrate_r < -ROT_DZN ) {
    drive_r = ( drive_r > -( 127 - ROT_ACC ) ? drive_r - ROT_ACC : -127 );
    if( drive_r < integrate_r / ROT_DMP - ROT_DZN ) drive_r = integrate_r / ROT_DMP - ROT_DZN;
  } else {
    drive_r = 0;
  }

  // Handle camera pitch
  if( ( long )drive_p + host->diff->my > 255 / CAM_SEN ) {
    drive_p = 255 / CAM_SEN;
  } else if( ( long )drive_p + host->diff->my < 0 ) {
    drive_p = 0;
  } else {
    drive_p = drive_p + host->diff->my;
  }
}

// Connection has glitched: stops all motion
static void stop_moving() {
  drive_x = 0;
  drive_y = 0;
  drive_r = 0;
  dk_x = 0;
  dj_x = 0;
  dk_y = 0;
  dj_y = 0;
  drive_p = 165 / CAM_SEN;
  integrate_r = 0;
}

// Frees allocated resources
static void closer() {
  if( h_thread ) host->thread_stop( h_thread );
}

// Switches emoticon based on connection status
static void connect_status( int status ) {
  char data[ 256 ];
  connected = status;
  emoticon = ( connected ? EMO_CONNECTED : EMO_IDLE );
  if( connected ) {
      memcpy( data, "LANGUAGE/VOICE: ", 16 );
      strcpy( data + 16, host->speak_voice( 0 ) );
      host->client_send( data, strlen( data ) );
  }
}

// Initializes serial communications
static void init() {
  char temp[ CFG_VALUE_MAX_SIZE ];
  // Configuration
  if( host->cfg_read( temp, "timeout_emoticon" ) ) timeout_emoticon = atoi( temp );
  // Initialise serial
  if( host->cfg_read( serdev, "commport" ) ) {
    h_thread = host->thread_start( commthread );
  } else {
    printf( "KiwiRay [warning]: Configuration - commport missing, disabling serial\n" );
  }
}

// Sets up the plugin descriptor
pluginclient_t *kiwiray_open( pluginhost_t *p_host ) {
  memcpy( &kiwiray.ident, "KIWI", 4 );
  host = p_host;
  kiwiray.init       = init;
  kiwiray.close      = closer;
  kiwiray.still      = stop_moving;
  kiwiray.tick       = tick;
  kiwiray.recv       = process_data;
  kiwiray.connected  = connect_status;
  return( &kiwiray );
}
