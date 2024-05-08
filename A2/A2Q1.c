//---------------------------------------------------------------
//NAME: OLUWASEUN OMOJOLA
//STUDENT NUMBER: 7880480
//USERID: OMOJOLAS
//---------------------------------------------------------------


#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#define TIMER0_SELECT  2
#define size 1000
volatile int overflow_count;
ISR(TIMER0_OVF_vect){
    TCNT0 = 131;
    overflow_count++;
}

void reset_timer0()
{
  TCNT0 = 131;
  TCCR0B = TIMER0_SELECT;
  TIMSK0 |= _BV(TOIE0);
}


//initialize the ADC 
void ADC_Init(){
	ADCSRA |= ((1<<ADPS1)&&(1<<ADPS0));	
	ADMUX |= (1<<REFS0);			
	ADCSRA |=(1<<ADEN);
    ADCSRA |=(1<<ADSC);
}

int ADC_Read(uint8_t pin){
    int high, low;
	ADMUX &=(0xF0);
	ADMUX |= (pin);	// set the pin to be read

	ADCSRA |= (1<<ADSC);	//start conversion	
	while((ADCSRA & (1<<ADSC))==0);	//loop till end of conversion 
    
     while(overflow_count <=100){
     }
    reset_timer0();

    // return the reading
	low = (int) ADCL;
    high = (int) ADCH *256;
    high = high + low;
    return high;
}

int main(void){
    int index = 0; // used to keep track of what value in the buffer to overwrite
    int full = -1; //a flag to check if the buffer for averaging is fully populated
    int ADC_readings[size];//buffer to hold the readings
    int sum = 0; //total of all values in the buffer
    DDRF = 0x00;
    PORTB = 0xFF;
    DDRB = 0xFF;
    sei();
    reset_timer0();
    ADC_Init();
    while(1){

        //check if the buffer is full
        if(index == size){
            full = 1;
            index = 0;
        }
        //if buffer is not full 
        if(full!=1){
            ADC_readings[index] = (ADC_Read(PF0))/4;
            sum += ADC_readings[index];
            PORTB = ~(sum/(index+1));
            index++;
            
        }
        //if buffer is full
        else if(full==1){
            sum = sum - ADC_readings[index];
            ADC_readings[index] = (ADC_Read(PF0))/4;
            sum += ADC_readings[index];
            PORTB = ~(sum/size);
            index++;
        }
        


    
    while(overflow_count <=250){
     }
    reset_timer0();
    }
       
    
    return 0;
}