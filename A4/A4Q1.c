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
#define NUM_LIGHTS  100

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
  SW0_ON,
  SW0_OFF,
  SW1_ON,
  SW1_OFF,
  SW2_ON,
  SW2_OFF,  
  NUM_STATES
};
typedef enum STATES State;

struct KEY_INFO
{
  unsigned int count;
  boolean      state;
};

typedef struct KEY_INFO KeyInfo;

static KeyInfo keyInfo[3] = { {OFF_MIN,false}, {OFF_MIN, false}, {OFF_MIN, false}}; 

volatile unsigned int overflows = 0;
boolean incr = true;
int curr_duty = 0;



//PID values
int p_gain = 0;
int i_gain = 0;
int d_gain = 0;
int d_state = 0;
int error = 0;
int i_state = 0;
int cmd_value = 0 ;
int feedback_value = 0;



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
  
  for ( key=PD0 ; key <= PD2 ; key++ )
  {
    if ( (keys & _BV(key)) == 0 )
    {
      if ( keyInfo[key].count < ON_MAX )
        keyInfo[key].count++;
    }
    else if ( keyInfo[key].count > OFF_MIN )
      keyInfo[key].count--;
    
    if ( keyInfo[key].count >= ON_LIMIT )
      keyInfo[key].state = true;
    else if ( keyInfo[key].count <= OFF_LIMIT )
      keyInfo[key].state = false;
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
        curr_state = SW0_ON;
        timestamp = 0;
      }
      
      else if ( keyInfo[1].state )
      {
        curr_state = SW1_ON;
        timestamp = 0;
      }

       else if ( keyInfo[2].state )
      {
        curr_state = SW2_ON;
        timestamp = 0;
      }
      break;



   
      
    case SW0_ON:
      timestamp++;
        
      if ( !keyInfo[0].state )
      {
        curr_state = SW0_OFF;
        timestamp = 0;
      }
        

      break;
      
    case SW0_OFF:
      timestamp++;
      if ( timestamp > BUTTON_DELAY )
      {
        cmd_value = 5;
        incr = true;     
        curr_state = NONE;
      }
      break;
        
    case SW1_ON:
      timestamp++;

        
      if ( !keyInfo[1].state )
      {
        curr_state = SW1_OFF;
      }
        
      break;

      
    case SW1_OFF:
      timestamp++;
      if ( timestamp > BUTTON_DELAY )
      {
        cmd_value = 100;
        incr = false;
        curr_state = NONE;
      }
      break;


    case SW2_ON:
    timestamp++;
        
    if ( !keyInfo[2].state )
    {
        curr_state = SW2_OFF;
        timestamp = 0;
    }
        
    break;
    
    case SW2_OFF:
    timestamp++;
    if ( timestamp > BUTTON_DELAY )
    {
        if((incr) &&(cmd_value + 5 <=100)){
            cmd_value += 5;
        }

        else if((!(incr)) &&(cmd_value - 5>=5)){
            cmd_value -= 5;
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
    TIMSK2 &= ~_BV(TOIE2);
    // set to fast PWM count to OCR2B match and clear output on compare match (for PH6) with an 8-bit timer running at CPU clock speed
    TCCR2A = 0x23;
    TCCR2B = 0x01;
    TCNT2 = 0;
    OCR2B = curr_duty;
    TIFR2 |= _BV(TOV2);
    TIMSK2 |= _BV(TOIE2);
    led = 0; 
   
}

void sample_light()
{  
  static unsigned short light[NUM_LIGHTS];
  static unsigned short light_index = 0;
  static unsigned short light_samples = 0;
  static unsigned long avg_light = 0;
  
  unsigned short value;
  unsigned short volts;
  
  // initialize the sample array...
  if (light_samples == 0)
  {
    int i;
    
    for (i=0; i<NUM_LIGHTS; i++)
      light[i] = 0;
  }
  
  // initiate a sample
  ADCSRA |= _BV(ADSC);
  
  // wait for the sample to finish by checking the interrupt flag
  while ( !(ADCSRA & _BV(ADIF)) )
    ;
  // clear the interrupt flag for the next run
  ADCSRA |= _BV(ADIF);
  
  value = ADC;
  
  // based on 10 bits of resolution and a 2.56V reference (multiplied by 1000 for greater precision)
  volts = (value * 1024UL) / 1024UL;
  
  // replace oldest value with newest for the running averaging
  avg_light -= light[light_index];
  light[light_index] = volts;
  avg_light += light[light_index];
  
  // stop counting after we have full history
  if ( light_samples < NUM_LIGHTS )
    light_samples++;

  // scale the value for our 8 bits of display resolution  -- hey, 2.56V is useful!!!
  PORTB = ~((avg_light/light_samples) / 10);
  feedback_value = ((avg_light/light_samples) / 10);
  light_index = (light_index+1)%NUM_LIGHTS;
}


// canaculates and returns the pid values
int pid_controller(int cmd_state, int state){
  cmd_state = cmd_state * 10;
  state = state * 10;
  error = cmd_state - state;
  int p_term = p_gain * error;
  if(i_state + error <=(i_gain * 255) && i_state + error >=-(i_gain * 255)){// (i_gain * 255)
    i_state+= error;}
  int i_term = i_gain * i_state;
  int d_term = d_gain* (d_state - state);
  d_state = state;
  return (p_term + i_term + d_term)/100;
}

//used to set the values of out gains
void tune_pid(int p_g, int i_g, int d_g){
  p_gain = p_g;
  i_gain = i_g;
  d_gain = d_g;
}

//generates the duty_cycle
void generate_output( int duty, int pid_value){
  if((curr_duty + pid_value <=255) && (curr_duty + pid_value >=0)){
    curr_duty += pid_value;}
  else if(curr_duty + pid_value >255){
    curr_duty = 255;}
  else if(curr_duty + pid_value <0){
    curr_duty = 0;}
}



int main (void)
{
  // Initialize port directions
  PORTB = 0xFF;
  DDRB = 0xFF;
  ADMUX = 0xC0;
  ADCSRA |= _BV(ADEN);

  InitPWM();
  set_sleep_mode( SLEEP_MODE_IDLE );
  
  sei();
  tune_pid(4,3,7);// gains scaled by a factor of 100
  reset_timer0();
  sample_light();
  int process_ovf_count = 0;
  // do an infinite loop
  for (;;) {
    sleep_mode();
    if ( (overflows >= PROCESS_OVERFLOWS) ){       
      process_ovf_count++;
      sample_inputs();
      process_state();
      if(process_ovf_count >= 2){
        sample_light();
        int curr_pid = pid_controller(cmd_value, feedback_value);
        generate_output(curr_duty, curr_pid);
        process_ovf_count = 0;}
      overflows = 0;
      
    }
  }
}

ISR(TIMER0_OVF_vect){
  // start at 131 to give us .001s per overflow (or 1ms)
  TCNT0 = 131;
  overflows++;
}

ISR(TIMER2_OVF_vect){
    OCR2B = curr_duty;
}



