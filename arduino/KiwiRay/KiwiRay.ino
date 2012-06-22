#include <Wire.h>

// Frequency gen. interrupt speed (reload value)
#define SPD 128

// Motor driver step size outputs
#define DS0 13
#define DS1 12
#define DS2 11

// Motor drivers enable output
#define DEN 10

// Wheels direction & step outputs
#define W0D  8
#define W0S  9
#define W1D  6
#define W1S  7
#define W2D  3
#define W2S  4

// Servo PWM output
#define SPW  5 // Must be 5

// Readability
#define STEPSIZE( n ) digitalWrite( DS0, n & 1 ); \
                      digitalWrite( DS1, n & 2 ); \
                      digitalWrite( DS2, n & 4 );
#define ENABLE( b )   digitalWrite( DEN, !b );
#define X ( (int)( (char)buf[ 0 ] ) )
#define Y ( (int)( (char)buf[ 1 ] ) )
#define R ( (int)( (char)buf[ 2 ] ) )

// Math
#define ABS( x ) ( x & 0x8000 ? ( ~x ) + 1 : x )

// Communications handling
unsigned char state, addr, len, got, buf[ 64 ];

// Frequency generation for the wheels 
int          wheel[ 3 ] = { 0, 0, 0 };
unsigned int whext[ 3 ];

// Turtle drive
int           turtle[ 3 ] = { 0, 0, 0 };
unsigned char turtleindex[ 3 ];

unsigned int  turtlespeed;
int           turtleinteg, turtlecount;
char          turtlemem[ 3 ];

unsigned int  step0, step1, step2;

// Connection loss protection
unsigned long timer_data = 0;

// Power save on idle
unsigned long timer_zero = 0;

// Turtle
unsigned char turtlemode = 0;

unsigned char last0, last1, last2, new0, new1, new2;

ISR( TIMER2_OVF_vect ) {
  unsigned char temp_uc;
  int temp_si;
  
  TCNT2 = SPD;

  if( turtlemode ) {

    if( turtlemode == 1 ) {
      if( turtlecount <= ( ABS( turtle[ turtleindex[ 0 ] ] ) >> 1 ) ) {
        if( turtlespeed < 32767 ) turtlespeed++; // Accellerate
      } else {
        if( turtlespeed > 0 ) turtlespeed--; // Decellerate
      }
          
      turtleinteg += turtlespeed;
  
      temp_uc = ( ( turtleinteg & 0x8000 ) != 0 );
      if( temp_uc != turtlemem[ turtleindex[ 0 ] ] ) {
        turtlemem[ turtleindex[ 0 ] ] = temp_uc;
        if( ++turtlecount == ABS( turtle[ turtleindex[ 0 ] ] ) ) {
          turtlemode = 2;
          turtlespeed = 0;
          wheel[ 0 ] = 0;
          wheel[ 1 ] = 0;
          wheel[ 2 ] = 0;
          //Serial.write( 'D' );
          Serial.write( step0 >> 8 );
          Serial.write( step0 & 0xFF );
          Serial.write( step1 >> 8 );
          Serial.write( step1 & 0xFF );
          Serial.write( step2 >> 8 );
          Serial.write( step2 & 0xFF );
        }
        turtlemem[ turtleindex[ 1 ] ] = ( ( long )turtlecount * ( long )ABS( turtle[ turtleindex[ 1 ] ] ) ) / ABS( turtle[ turtleindex[ 0 ] ] );
        turtlemem[ turtleindex[ 2 ] ] = ( ( long )turtlecount * ( long )ABS( turtle[ turtleindex[ 2 ] ] ) ) / ABS( turtle[ turtleindex[ 0 ] ] );
      }

      new0 = turtlemem[ 0 ] & 1;
      new1 = turtlemem[ 1 ] & 1;
      new2 = turtlemem[ 2 ] & 1;
      if( new0 != last0 ) step0++;
      if( new1 != last1 ) step1++;
      if( new2 != last2 ) step2++;
      last0 = new0;
      last1 = new1;
      last2 = new2;

      digitalWrite( W0D, ( turtle[ 0 ] & 0x8000 ) != 0 );
      digitalWrite( W1D, ( turtle[ 1 ] & 0x8000 ) != 0 );
      digitalWrite( W2D, ( turtle[ 2 ] & 0x8000 ) != 0 );
  
  //    Serial.print( turtleindex[ 2 ] );
      digitalWrite( W0S, turtlemem[ 0 ] & 1 );
      digitalWrite( W1S, turtlemem[ 1 ] & 1 );
      digitalWrite( W2S, turtlemem[ 2 ] & 1 );
  
      ENABLE( 1 );
    }
  } else {
    digitalWrite( W0D, ( wheel[ 0 ] & 0x8000 ) != 0 );
    digitalWrite( W1D, ( wheel[ 1 ] & 0x8000 ) != 0 );
    digitalWrite( W2D, ( wheel[ 2 ] & 0x8000 ) != 0 );
    whext[ 0 ] += ABS( wheel[ 0 ] );
    whext[ 1 ] += ABS( wheel[ 1 ] );
    whext[ 2 ] += ABS( wheel[ 2 ] );
    digitalWrite( W0S, ( whext[ 0 ] & 0x8000 ) != 0 );
    digitalWrite( W1S, ( whext[ 1 ] & 0x8000 ) != 0 );
    digitalWrite( W2S, ( whext[ 2 ] & 0x8000 ) != 0 );
    // Connection loss and idle detection
    if( wheel[ 0 ] || wheel[ 1 ] || wheel[ 2 ] ) timer_zero = 10000; 
    if( timer_data ) timer_data--;
    if( timer_zero ) timer_zero--;
    ENABLE( timer_data && timer_zero );
  }
}   

void setup() {
  // Pin setup
  pinMode( W0D, OUTPUT );
  pinMode( W0S, OUTPUT );
  pinMode( W1D, OUTPUT );
  pinMode( W1S, OUTPUT );
  pinMode( W2D, OUTPUT );
  pinMode( W2S, OUTPUT );
  pinMode( DS0, OUTPUT );
  pinMode( DS1, OUTPUT );
  pinMode( DS2, OUTPUT );
  pinMode( DEN, OUTPUT );
  pinMode( SPW, OUTPUT );
  
  // Configure Servo PWM on T0
  analogWrite( SPW, 96 );
  TCCR0A |= 0b00000011;
  TCCR0B  = 0b11001100;
  OCR0A   = 255;
  OCR0B   = 110;

  // Configure frequency gen. interrupt
  TIMSK2 &= ~( ( 1 << TOIE2  ) );
  TCCR2A &= ~( ( 1 << WGM21  ) | ( 1 << WGM20 ) );
  TCCR2B &= ~( ( 1 << WGM22  ) );
  ASSR   &= ~( ( 1 << AS2    ) );
  TIMSK2 &= ~( ( 1 << OCIE2A ) );   
  TCCR2B &= ~( ( 1 << CS22   ) | ( 1 << CS21  ) | ( 1 << CS20 ) );
  //TCCR2B |=  ( ( 1 << CS20 ) ); // Prescaler 1:1
  TCCR2B |=  ( ( 1 << CS21 ) ); // Prescaler 1:8
  //TCCR2B |=  ( ( 1 << CS21 ) | ( 1 << CS20 ) ); // Prescaler 1:32
  TCNT2   = SPD;
  TIMSK2 |=  ( ( 1 << TOIE2  ) );
    
  // Initialize I2C
  Wire.begin();
  // Set silly slow I2C for PIC soft I2C
  //_SFR_BYTE( TWSR ) |= _BV( TWPS0 ) | _BV( TWPS1 );

  // Initialize PC-communications
  Serial.begin( 115200 );
  
  // Initialize motor control
  STEPSIZE( 6 );
  ENABLE( 0 );
}

void process() {
  if( addr & 0xFC ) {
    // start i2c
    Wire.beginTransmission( addr );
    Wire.write( buf, len );
    Wire.endTransmission();
  } else {
    cli();
    turtlemode = 0;
    if( addr == 1 ) {
      // Addr 0x01: direct 16-bit motor control
      wheel[ 0 ] = *( int* )&buf[ 0 ];
      wheel[ 1 ] = *( int* )&buf[ 2 ];
      wheel[ 2 ] = *( int* )&buf[ 4 ];
      OCR0B = ( ( ( (unsigned int)buf[ 6 ] ) * 86 ) >> 8 ) + 48;
      STEPSIZE( buf[ 7 ] );
    } else if( addr == 0 ) {
      // Addr 0x00: 8-bit XYR control
      wheel[ 0 ] = ( X + R ) * 128;
      wheel[ 1 ] = ( -0.5 * X - 0.866 * Y + R ) * 128;
      wheel[ 2 ] = ( -0.5 * X + 0.866 * Y + R ) * 128;
      OCR0B = ( ( ( (unsigned int)buf[ 3 ] ) * 86 ) >> 8 ) + 48;
      STEPSIZE( buf[ 4 ] );
    } else {
      turtlemode = 1;
      turtle[ 0 ] = *( int* )&buf[ 0 ];
      turtle[ 1 ] = *( int* )&buf[ 2 ];
      turtle[ 2 ] = *( int* )&buf[ 4 ];
      if( ABS( turtle[ 0 ] ) > ABS( turtle[ 1 ] ) ) {
        turtleindex[ 0 ] = ( ABS( turtle[ 0 ] ) > ABS( turtle[ 2 ] ) ) ? 0 : 2;
      } else {
        turtleindex[ 0 ] = ( ABS( turtle[ 1 ] ) > ABS( turtle[ 2 ] ) ) ? 1 : 2;
      }
      turtleindex[ 1 ] = ( turtleindex[ 0 ] + 1 ) % 3;
      turtleindex[ 2 ] = ( turtleindex[ 0 ] + 2 ) % 3;
      turtleinteg = 0;
      turtlecount = 0;
      turtlemem[ 0 ] = 0;
      turtlemem[ 1 ] = 0;
      turtlemem[ 2 ] = 0;
      STEPSIZE( 6 );
      step0 = 0;
      step1 = 0;
      step2 = 0;
      Serial.write( 0xFF );
    }
  }
  sei();
}

void loop() {
  unsigned char data;

  if( Serial.available() ) {
    data = Serial.read();
    switch( state ) {
      case 0: // wait for start
        if( data == 0xFF ) state = 1;
        break;
      case 1: // address
        addr = data;
        if( addr & 0xFC ) {
          state = 2;
        } else {
          state = 3;
          if( addr & 2 ) {
            len = 6;
          } else {
            if( addr & 1 ) {
              len = 8;
            } else {  
              len = 5;
            }
          }
          timer_data = 10000;
        }
        got = 0;
        break;
      case 2: // Length & R/W bit
        len = ( data & 0x3F ) + 1;
        // TODO: implement read
        state = 3;
        break;
      case 3: // data
        buf[ got++ ] = data;
        if( got == len ) {
          process();
          state = 0;
        }
        break;
    }
  }
}
