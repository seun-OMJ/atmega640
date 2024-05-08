//---------------------------------------------------------------
//NAME: OLUWASEUN OMOJOLA
//STUDENT NUMBER: 7880480
//USERID: OMOJOLAS
//---------------------------------------------------------------

//NOTE: The button sampling and idea of the finite state machine 
// is gotten from the sample solution A1Q3


#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>

// timer 0 resolution -- conveniently set to overflow every 0.002048s with a 1MHz clock
#define TIMER0_SELECT  2

#define PROCESS_OVERFLOWS  50   // gives us .05s -- 20x/s

#define NUM_LEDS 4

// 300ms
#define BUTTON_DELAY (300/PROCESS_OVERFLOWS)
// 2 seconds
#define KEY_CLEAR    (2000/PROCESS_OVERFLOWS)
// a second
#define KEY_REPEAT   (1000/PROCESS_OVERFLOWS)

#define ON_LIMIT  2
#define ON_MAX    2
#define OFF_LIMIT 1
#define OFF_MIN   1
int led = 0; //used to indicate the fan speed level from 0 to 5
enum BOOL
{
  false,
  true
};
typedef enum BOOL boolean;

enum STATES
{
  NONE,
  SW1_ON,
  SW1_OFF,
  SW1_ON_HOLD,
  SW2_ON,
  SW2_OFF,
  SW2_ON_HOLD,
  NUM_STATES
};
typedef enum STATES State;

struct KEY_INFO
{
  unsigned int count;
  boolean      state;
};

typedef struct KEY_INFO KeyInfo;

static KeyInfo keyInfo[2] = { {OFF_MIN,false}, {OFF_MIN, false} };

volatile unsigned int overflows = 0;

void reset_timer0()
{
  // set timer count rate

  // start at 131 to give us .001s per overflow (or 1ms)
  TCNT0 = 131;
  TCCR0B = TIMER0_SELECT;
  TIMSK0 |= _BV(TOIE0);
}

// samples the input sw1 and sw2
void sample_inputs()
{
  unsigned char keys = PIND;  // store now so we don't get changes as we go...
  unsigned char key;
  
  for ( key=PD1 ; key <= PD2 ; key++ )
  {
    if ( (keys & _BV(key)) == 0 )
    {
      if ( keyInfo[key-1].count < ON_MAX )
        keyInfo[key-1].count++;
    }
    else if ( keyInfo[key-1].count > OFF_MIN )
      keyInfo[key-1].count--;
    
    if ( keyInfo[key-1].count >= ON_LIMIT )
      keyInfo[key-1].state = true;
    else if ( keyInfo[key-1].count <= OFF_LIMIT )
      keyInfo[key-1].state = false;
  }
}

//this processes the inputs and and performs the necessary tasks per state
void process_state()
{
  static State curr_state = NONE;
  
  // each run is 50ms so we can use that to our advantage here...
  static unsigned char timestamp = 0;


  
  
  switch ( curr_state )
  {
    case NONE:
      if ( keyInfo[0].state )
      {
        curr_state = SW1_ON;
        timestamp = 0;
      }
      
      else if ( keyInfo[1].state )
      {
        curr_state = SW2_ON;
        timestamp = 0;
      }
      break;
      
    case SW1_ON:
      timestamp++;
        
      if ( !keyInfo[0].state )
      {
        curr_state = SW1_OFF;
        timestamp = 0;
      }
        
      else if ( timestamp > BUTTON_DELAY )
      {
        curr_state = SW1_ON_HOLD;
      }
      break;
      
    case SW1_OFF:
      timestamp++;
      if ( timestamp > BUTTON_DELAY )
      {
         if(OCR2B - 51>=0){
            OCR2B -= 51;
            led--;
            PORTB = ~(1<<led);
        }
     
        curr_state = NONE;
      }
        

      break;
      

      
    case SW1_ON_HOLD:
      timestamp++;
      if ( timestamp >= KEY_REPEAT )
      {
       
  
        if(OCR2B - 51>=0){
            OCR2B -= 51;
            led--;
            PORTB = ~(1<<led);

        }
        
        timestamp = 0;
        
      }
        
      else if ( !keyInfo[0].state )
      {
        curr_state = NONE;
        timestamp = 0;
      }
      break;
      
    case SW2_ON:
      timestamp++;

        
      if ( !keyInfo[1].state )
      {
        curr_state = SW2_OFF;
      }
        
      else if ( timestamp > KEY_REPEAT )
      {
        curr_state = SW2_ON_HOLD;
      }
        
      break;
      
    case SW2_ON_HOLD:
      timestamp++;
      if ( timestamp >= KEY_REPEAT )
      {
        if(OCR2B + 51<=255){
            OCR2B += 51;
            led++;
            PORTB = ~(1<<led);
        }

        timestamp = 0;
      }
        
      else if ( !keyInfo[1].state )
      {
        curr_state = NONE;
        timestamp = 0;
      }
      break;
      
    case SW2_OFF:
      timestamp++;
      if ( timestamp > BUTTON_DELAY )
      {
         if(OCR2B + 51<=255){
            OCR2B += 51;
            led++;
            PORTB = ~(1<<led);

        }

        curr_state = NONE;
      }
      break;
      

      
    default:
      break;
      
  }  


}

//this initializes the PWMing
void InitPWM(){
   DDRH|=(1<<PH6);
   OCR2B = 255;
  
   TCCR2A |=(1<<WGM21)|(1<<WGM20)|(1<<COM2B1);
   TCCR2B |= (1<<CS21);
   led = 5;
   PORTB = ~(1<<led);//indicates the level of the fan speed
   
   
}


int main (void)
{
  // Initialize port directions
  PORTB = 0xFF;
  DDRB = 0xFF;

  InitPWM();
  set_sleep_mode( SLEEP_MODE_IDLE );
  
  sei();

  reset_timer0();
  
  // do an infinite loop
  for (;;) 
  {
    sleep_mode();
    if ( (overflows >= PROCESS_OVERFLOWS) )
    {      
        
      sample_inputs();
      process_state();
      
      overflows = 0;
    }
  }
}

ISR(TIMER0_OVF_vect)
{
  // start at 131 to give us .001s per overflow (or 1ms)
  TCNT0 = 131;
  
  overflows++;
}

