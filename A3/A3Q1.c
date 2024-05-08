//---------------------------------------------------------------
//NAME: OLUWASEUN OMOJOLA
//STUDENT NUMBER: 7880480
//USERID: OMOJOLAS
//---------------------------------------------------------------

#include <avr/io.h>
#include <stdio.h>
#include <avr/interrupt.h>

// timer 0 resolution -- conveniently set to overflow every 0.002048s with a 1MHz clock
#define TIMER0_SELECT  2

#define PROCESS_OVERFLOWS  5   // gives us .005s -- 20x/s

#define BUFFER_SIZE 20

// 250ms
#define BUTTON_DELAY (250/PROCESS_OVERFLOWS)
// 250ms
#define KEY_REPEAT   (250/PROCESS_OVERFLOWS)

#define ON_LIMIT  2
#define ON_MAX    2
#define OFF_LIMIT 1
#define OFF_MIN   1
volatile unsigned int overflows;
volatile unsigned int timer3_overflows;
volatile unsigned int timer1_overflows;
static unsigned int buffer_count;//used to know how many bits to transmit
int transmit_time = 0;//to check if a second has passed after user input

int key = 1;

enum STATES
{
  NONE,
  SW1_ON,
  SW1_OFF,
  SW1_ON_HOLD,
  SW1_OFF_HOLD,
};
typedef enum STATES State;


enum BOOL
{
  false,
  true
};
typedef enum BOOL boolean;

int curr_duty = 0x1ff;
unsigned int count;
boolean      state;
volatile unsigned int current_index = 0;
int buffer[BUFFER_SIZE];
boolean is_transmit = false;

void reset_timer0(){
  // set timer count rate
  overflows = 0;
  // start at 131 to give us .001s per overflow (or 1ms)
  TCNT0 = 131;
  TCCR0B |= 2;
  TIMSK0 |= _BV(TOIE0);
}

ISR(TIMER0_OVF_vect){
  // start at 131 to give us .001s per overflow (or 1ms)
  TCNT0 = 131;
  overflows++;
}


//sample button to handle hysteresis
void sample_sw1(){
    unsigned char keys = PIND;  
  
 
    if ( (keys & _BV(key)) == 0 )
    {
      if ( count < ON_MAX )
        count++;
    }
    else if ( count > OFF_MIN )
      count--;
    
    if ( count >= ON_LIMIT )
      state = true;
    else if ( count <= OFF_LIMIT )
      state = false;
  
}
//this processes the inputs and performs the necessary tasks per state
void process_state()
{
  static State curr_state = NONE;
  
  // each run is 5ms so we can use that to our advantage here...
  static unsigned char timestamp = 0;


  
  
  switch ( curr_state )
  {
    case NONE:
    PORTB = 0XFF;
      if ( state )
      {
        curr_state = SW1_ON;
        timestamp = 0;
        is_transmit = true;
        transmit_time = overflows;
        
      }
      break;
      
    case SW1_ON:
      timestamp++;
        
      if ( !state )
      {
        PORTB = 0x7f;
        curr_state = SW1_OFF;
        transmit_time = overflows;
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
        PORTB = (0X7F);
        buffer[current_index] = 0;
        current_index++;
        buffer_count++;
        transmit_time = overflows;
        curr_state = NONE;
      }
        

      break;
      

      
    case SW1_ON_HOLD:
      timestamp++;
      if ( timestamp >= KEY_REPEAT )
      {        
        PORTB = (0XBF);
        transmit_time = overflows;
        timestamp = 0;
        curr_state = SW1_OFF_HOLD;
        
      }
        
      else if ( !state )
      {
        curr_state = NONE;
        timestamp = 0;
      }
      break;   

      case SW1_OFF_HOLD:
      if ( !state )
      {
        PORTB = (0XBF);
        transmit_time = overflows;
        buffer[current_index] = 1;
        current_index++;
        buffer_count+=1;
        curr_state = NONE;
      }
      break;   

      
    default:
      break;
      
  }  


}


//set up timer for pwm 
void reset_timer3(){
    DDRE   |= (1 << PE3);
    OCR3A  = 0x1FF;  
    TCCR3A |= (1<<WGM30) | (1 << WGM31);
    TCCR3B |=  (1 << WGM32);
    TCCR3A |=  (1<< COM3A1);
    TCCR3B |= (1<<CS32);
    TIMSK3 |= (1<<TOIE3);
    TIFR3  |= (1<<TOV3);
    timer3_overflows = 0;
    TCNT3  = 0;
}

SIGNAL(TIMER3_OVF_vect){
    OCR3A = curr_duty;
    timer3_overflows++;
}


//trnamsit the duty cycle
int start_duty = 0x2ff;
int zero_duty = 0x000;
int one_duty = 0x3ff;
int end_duty = 0x1ff;

//this function transmits the duty cycles representing the bits
void transmit_bits(int buf){
  int i = -1;
  
  while(i<=buf+1){
  if((timer3_overflows >=5)){
    if(i==-1){
        PORTB = ~(buffer_count);
        curr_duty = start_duty;}
    else if(buffer[i] == 0){
      curr_duty = zero_duty; 
      buffer[i] = -1;
    }
    else if(buffer[i] == 1){

      curr_duty = one_duty;
      buffer[i] = -1;
    }

    if(i==buf){
    curr_duty = end_duty;}
    timer3_overflows = 0;
    i++;
    
    }
  }
  buffer_count = 0;
}


// int recv_buf[];
// void receive_bits(){}

//Timer1 to be used for timing the bits received
void reset_timer1(){
    TIMSK1 &= ~_BV(TOIE1);
  
    TCCR1A |=  (1 << WGM11);
    TCCR1B |=  (1 << WGM12)|(1<<WGM13);
    TCCR1B = (1<<CS12);
    TIFR1 |= _BV(TOV1);
    TIMSK1 |= _BV(TOIE1);
    timer1_overflows = 0;
    TCNT1 = 0;
}

 SIGNAL(TIMER1_OVF_vect){
    timer1_overflows++;

 }


int main(void){

    PORTB = 0xff;
    DDRB = 0xff;      // make PORTB as output
   
    reset_timer0();
    reset_timer3();
    reset_timer1();
    sei();
    
    for(int i = 0; i < BUFFER_SIZE; i++){
      buffer[i] = -1;
    }


    int overflow_count = overflows;
    while(1){
         if ( ((overflows - overflow_count) >= PROCESS_OVERFLOWS) )
    {      
      sample_sw1();
      process_state();
      overflow_count = overflows;
    }

        if((is_transmit)  && (overflows - transmit_time)>=1000){
            is_transmit = false;
            transmit_time = 0;
            overflows = 0;
            overflow_count = 0;
            while(overflows <= 300){
            
            overflow_count = overflows;
            current_index = 0;
            }
            transmit_bits(buffer_count);
        }
  
    }

    return 0;
    }
 