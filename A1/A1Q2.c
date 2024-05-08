//---------------------------------------------------------------
//NAME: OLUWASEUN OMOJOLA
//STUDENT NUMBER: 7880480
//USERID: OMOJOLAS
//---------------------------------------------------------------



#include <avr/io.h>
#include <stdio.h>
#include <avr/interrupt.h>

volatile uint8_t overflow_count;
uint32_t elapsed_time;
uint8_t eeprom_address = 0;
ISR(TIMER1_OVF_vect){
    overflow_count++;
}


void write_to_eeprom(unsigned int address, unsigned char data){
    // Wait for previous write 
    while(EECR & (1<<EEPE));
    // Set up address registers.
    EEAR = address;
    EEDR = ucData;
    EECR |= (1<<EEMPE);
    EECR |= (1<<EEPE);
}





ISR(INT2_vect){ 
    if (!(PIND & 0x04)){
    //indicate action with led on
            PORTB = 0xfb;
        
    
        cli();
       // 0xedafcbe8; //
        elapsed_time =(((overflow_count)*(65535)) + TCNT1)/125;
        sei();

        //split the 32 bit integer into 4 bytes
        unsigned char b4 = (elapsed_time & 0xff000000UL) >> 24;
        unsigned char b3 = (elapsed_time & 0x00ff0000UL) >> 16;
        unsigned char b2 = (elapsed_time & 0x0000ff00UL) >>  8;
        unsigned char b1 = (elapsed_time & 0x000000ffUL);
        unsigned char separator = 0xff;
        
        cli();
        //write each byte to eeprom
        write_to_eeprom(eeprom_address++, b4);
        write_to_eeprom(eeprom_address++, b3);
        write_to_eeprom(eeprom_address++, b2);
        write_to_eeprom(eeprom_address++, b1);
        write_to_eeprom(eeprom_address++, separator);
        sei();
        }
     else{
        // indicate no action with no led on
        PORTB = 0xff;}
}


ISR(INT3_vect){ 
    if (!(PIND & 0x08)){
        //indicate action with timer
            PORTB = 0xf7; 

        //restart timer 
            overflow_count = 0;
            TCNT1 = 0;
    }
    else{
    //indicate no action with no led on
    PORTB = 0Xff;
   }}



int main(void){
    DDRB=0xff;      // make PORTB as output
    
    //set up external interrupts 2 and 3 to trigger on edges
    EIMSK|= (1<<INT2) | (1<<INT3);
    SREG|= (1<<INT2) | (1<<INT3);
    EICRA |= ((1<<ISC30)) | ((1<<ISC20));
    PORTB=0xff;     // all LEDS off
    sei();  // enabling global interrupts
    
    
    TIMSK1|=(1<<TOIE1);  
    TCCR1B |= (1 << CS11);  // prescaler value = 8; => 125KHz //125 TICKS PER MILLISECOND
    // initialize counter
    TCNT1 = 0;
    
    
    
    
    
    while(1){
    }
        return 0;
    }
 