#ifndef _ROBOCORTEX_H_
#define _ROBOCORTEX_H_
#include "SDL/SDL_video.h"
#include <stdint.h>

#define CORTEX_VERSION       3 // Current protocol revision
#define CFG_TOKEN_MAX_SIZE  32 // Maxmimum length of a token value
#define CFG_VALUE_MAX_SIZE 256 // Maxmimum length of a configuration value

enum font_e {
  FONT_GREEN,
  FONT_RED
};

enum kb_bitmask_e {
  KB_LEFT  = 1,
  KB_RIGHT = 2,
  KB_UP    = 4,
  KB_DOWN  = 8
};

// Visemes
typedef struct {
	uint8_t viseme;
	uint8_t pad[3];
	uint32_t time;
} __attribute__ ((packed)) viseme_t ;

// DISP packet
typedef struct {
  uint8_t trust_srv;
  uint8_t trust_cli;
  uint16_t pad;
  int32_t timer;
} __attribute__ ((packed)) disp_data_t;

// Control data
typedef struct {
  int32_t mx;
  int32_t my;
  int32_t dx;
  int32_t dy;
  uint8_t kb; // kb_bitmask_e
  uint8_t pad[3];
} __attribute__ ((packed)) ctrl_t;

// DATA packet
typedef struct {
  uint8_t trust_srv;
  uint8_t trust_cli;
  uint8_t pad[2];
  ctrl_t ctrl;
} __attribute__ ((packed)) ctrl_data_t;

// Linked buffer
struct linked_buf_t {
  uint8_t data[ 8192 ];
  int32_t size;
  struct linked_buf_t *next;
} __attribute__ ((packed));
typedef struct linked_buf_t linked_buf_t;

// Chunk
typedef struct {
  void *addr;
  int size;
  void *handler;
} __attribute__ ((packed)) remote_t;

extern char *config_rc;
extern unsigned char term_w, term_h;
extern int config_read_line( char **value, char **token, FILE *f );
extern int config_find_line( char **find_value, char *find_token, FILE *f );
extern int config_plugin( uint32_t i_ident, char* dst, char* req_token );
extern void config_parse( int( *callback )( char*, char* ) );
extern SDL_Rect *rect( SDL_Rect *r, int x, int y, int w, int h );
extern char unicode_ascii( int uni );
#endif
