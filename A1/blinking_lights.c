



#include <avr/io.h>
#include <stdio.h>
#include <avr/interrupt.h>
volatile uint8_t tot_overflow; // overflow counter
ISR(TIMER0_OVF_vect){   // to keep track of overflows
    tot_overflow++;
    TCNT0 = 131;
}

int main(void){
    DDRB=0xff;      //make PORTB as output
    
    PORTB=0xff;     // all LEDS off
    int i = 0;      // to indicate which LED should be on  
    int flag = 1;   // to indicate direction of light trail (0 => left; 1 => right)
   
    TIMSK0|=(1<<TOIE0); 
    sei();  // enabling global interrupts
    TCCR0B |= (1 << CS01);  // prescaler value = 8; => 125KHz
  
    // initialize counter
    TCNT0 = 131;
    
    while(1){
        
        if(tot_overflow >= 250){
            if((TCNT0>= 131)){
                TCNT0 = 131; 
                tot_overflow = 0;
                PORTB = 0xff;
                PORTB ^= 1<<i; //set specific led to be turned on
                if(i==7){flag = 0;} //if at led7, turn back
                else if(i == 0){flag = 1;}//if at led1, turn back
                if(flag == 1){//go right 
                    i++;} 
                else if(flag == 0){//go left
                    i--;}
            
                }
            }
    }
        return 0;
    }
 
