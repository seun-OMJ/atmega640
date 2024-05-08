//---------------------------------------------------------------
//NAME: OLUWASEUN OMOJOLA
//STUDENT NUMBER: 7880480
//USERID: OMOJOLAS
//---------------------------------------------------------------



#include <avr/io.h>
#include <stdio.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
unsigned int sw0_bounce = 0;
unsigned int sw1_bounce = 0;
int sw0_single_press = 0;
uint16_t sw0_time_capture = 0;
uint16_t sw1_time_capture = 0;
uint16_t sw0_double_press = 0;
int sw0_pressed = -2;
int pressedsw1 = -2;


volatile uint16_t timer0_overflow_count; // overflow counter
ISR(TIMER0_OVF_vect){   // to keep track of overflows
    timer0_overflow_count++;
    TCNT0 = 131;
}


//handles hysterises for siwtch 0 inputs and handles debouncing
    int sw0_routine(){
        int result = 1;

        //if input is detected, increase bounce count and set sw0_pressed to 1
        if((!(PIND & 0x01)) &&(sw0_bounce < 101)){
            sw0_time_capture = timer0_overflow_count;
            sw0_bounce++;
            sw0_pressed = 1;
        }
        //if no input is detected, decrease bounce and set sw0_pressed to -1
        else if((PIND & 0x01) && (sw0_bounce > 0)){
            sw0_bounce--;
            sw0_pressed = -1;  
         }
        //if no input is detected, and time between presses >= 0.25 seconds, set double press to 0
        else if((((PIND & 0x01)))&&(timer0_overflow_count - sw0_time_capture>250)&& sw0_double_press==1){
            sw0_double_press = 0;
            sw0_single_press = 0;  
          
        }
        //if bounce above threshold, perform checks to know what type of press was triggered
        else if(((sw0_bounce==101))&&(sw0_pressed==1)){
            sw0_pressed = 0;
            result = 0;

            if((timer0_overflow_count - sw0_time_capture>250)){
            sw0_time_capture = timer0_overflow_count;
            sw0_double_press = 0;
      
            sw0_single_press = 0;
            }
            
            else if(timer0_overflow_count - sw0_time_capture<=250 ){
            sw0_double_press++;
            }
            
        }
    return result;
        }



    //handles hysterises for siwtch 1 inputs and handles debouncing
    int sw1_routine(){
        int result = 1;
        //if input is detected, increase bounce count and set sw0_pressed to 1
        if((!(PIND & 0x02)) &&(sw1_bounce < 101)){
            
                sw1_bounce++;
                pressedsw1 = 1;
            }

        //if input is detected, increase bounce count and set sw0_pressed to 1
        else if((PIND & 0x02) && (sw1_bounce > 0)){
            sw1_bounce--;
            pressedsw1 = -1;                
                }

        //if bounce above threshold, perform checks to know what type of press was triggered        
        if(((sw1_bounce>100)&&(sw1_bounce<=101))&&(pressedsw1==1)){
            pressedsw1 = 0;
            sw1_time_capture = timer0_overflow_count;
            result = 0;
                
            }
        return result;
    }
void Finite_State_Machine(){



    // if sw1 and sw0 are triggered together, toggle LED 3
    if(((sw0_pressed ==0) &&(pressedsw1==0))&&((sw0_time_capture - sw1_time_capture <=100)||(sw1_time_capture - sw0_time_capture <=100))){
        sw0_pressed = -3;
        pressedsw1 = -3;
            PORTB ^=~(0xf7);
            PORTB ^= ~(0xfe);
            PORTB ^= ~(0xfd);
    }


    // if sw0 is held for 2 seconds, turn off all LEDs,
    else if(((timer0_overflow_count - sw0_time_capture) >=2000) &&(sw0_pressed == 0)){
            PORTB = 0xff;
            timer0_overflow_count = 1;
            sw0_time_capture = timer0_overflow_count;}



    // if sw0 is triggered, check if it is a double or single press
    else if(sw0_routine()==0){ 

        // if a double press was triggered, toggle LED 2
        if((sw0_double_press >=2) &&(timer0_overflow_count - sw0_time_capture<250)){
            sw0_double_press = 0;
            PORTB ^=~(0xfb);
            PORTB ^=~(0xfe);

        }
        // if a single press wa triggered, toggle LED 0
        else if((sw0_single_press==0) ){
            PORTB ^= ~(0xfe);
            }
       
    } 


    // if sw1 is triggered, toggle LED 1 
    else if(sw1_routine() == 0){
        PORTB ^= ~(0xfd);
       
    }
    

    // if sw1 is held, toggle LED 1 every second
    else if(((timer0_overflow_count - sw1_time_capture) >=1000) &&(pressedsw1 == 0)){
        PORTB ^= ~(0xfd);
        timer0_overflow_count = 1;
        sw1_time_capture = timer0_overflow_count;
    }

}



int main(void){
    DDRB=0xff;      //make PORTB as output
    
    PORTB=0xff;     // all LEDS off
   
    TIMSK0|=(1<<TOIE0); 
    sei();  // enabling global interrupts
    TCCR0B |= (1 << CS01);  // prescaler value = 8; => 125KHz
  
    // initialize counter
    TCNT0 = 131;


    sw0_time_capture = timer0_overflow_count;

    while(1){
        Finite_State_Machine();
   
    }
        return 0;
    }