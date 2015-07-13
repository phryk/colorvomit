#define F_CPU 16000000

#include <avr/io.h>
#include <util/delay.h>

int main(void){

    DDRB |= (1 << 7); // Set output direction for the pwm port with the LED
    TCCR0A = 0b10100011;
    TCCR0B = 0b00000011;
    //TCNT0 = 0;           // Reset TCNT0 <- what does this actually do?
    OCR0A = 0;           // Initial the Output Compare register A & B

    while(1) {

        for(uint8_t i = 0; i <= 200; i+=2){
            OCR0A = i;
            _delay_ms(1);
        }

        for(uint8_t i = 200; i > 0; i--){
            OCR0A = i;
            _delay_ms(3);
        }
    }
}
