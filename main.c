#define F_CPU 16000000

#define LED_NUM 4
#define PWM_PERIOD_LENGTH 65535

// maximum PWM value, -1 to never have LEDs on *all* the time.
// This ought to make sure they won't go up in smoke.
#define PWM_MAX PWM_PERIOD_LENGTH - 1


#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>


void setup(void){

  TCCR1B = 0;
  TCCR1A = 0;       
  TCCR1B |= (1<<CS10);                    //Kein Prescaler (16 MHz)

  TCCR3B = 0;
  TCCR3A = 0;
  TCCR3B |= (1<<CS30);                    //Kein Prescaler (16 MHz)

  TCCR4B = 0;
  TCCR4A = 0;
  TCCR4B |= (1<<CS40);                    //Kein Prescaler (16 MHz)


  // Set each 16 bit timer to use Fast PWM, with PWM period length defined in ICRn

  TCCR1B |= (1<<WGM13) | (1<<WGM12);
  TCCR1A |= (1<<WGM11) | (0<<WGM10);
 
  TCCR3B |= (1<<WGM33) | (1<<WGM32);
  TCCR3A |= (1<<WGM31) | (0<<WGM30);
 
  TCCR4B |= (1<<WGM43) | (1<<WGM42);
  TCCR4A |= (1<<WGM41) | (0<<WGM40);

  TCCR5B |= (1<<WGM53) | (1<<WGM52);
  TCCR5A |= (1<<WGM51) | (0<<WGM50);


  // Set PWM period length for each timer

  ICR1 = PWM_MAX;             
  ICR3 = PWM_MAX;             
  ICR4 = PWM_MAX;             
  ICR5 = PWM_MAX;             


  // Compare output mode. Connect all pwm channels to their output pins

  TCCR1A |= (1<<COM1A1) | (1<<COM1B1) | (1<<COM1C1);
  TCCR3A |= (1<<COM3A1) | (1<<COM3B1) | (1<<COM3C1);
  TCCR4A |= (1<<COM4A1) | (1<<COM4B1) | (1<<COM4C1);
  TCCR5A |= (1<<COM5A1) | (1<<COM5B1) | (1<<COM5C1);


  // Initialize all channels to 0

  OCR1A = 0;
  OCR1B = 0;
  OCR1C = 0;

  OCR3A = 0;
  OCR3B = 0;
  OCR3C = 0;

  OCR4A = 0;
  OCR4B = 0;
  OCR4C = 0;

  OCR5A = 0;
  OCR5B = 0;
  OCR5C = 0;


  // Set output direction for all ports connected to pwm channels 
  DDRB |= (1<<PB7) | (1<<PB6) | (1<<PB5);  // Bit 7 als Ausgang
  DDRE |= (1<<PE5);  // Bit 5 als Ausgang
  DDRH |= (1<<PH5);  // Bit 5 als Ausgang

  TIMSK1 |= (1>>TOIE1); // Let timer 1 trigger interrupt

}

volatile uint8_t FL_UPDATE = 0; // PWM update flag

typedef struct led {

    volatile uint16_t* red;
    volatile uint16_t* green;
    volatile uint16_t* blue;
}LED;

volatile LED LEDS[LED_NUM] = {
    { &OCR1A, &OCR1B, &OCR1C },
    { &OCR3A, &OCR3B, &OCR3C },
    { &OCR4A, &OCR4B, &OCR4C },
    { &OCR5A, &OCR5B, &OCR5C }
};


ISR( TIMER1_OVF_vect ) {

    FL_UPDATE = 1;
}


int main(void){

    setup(); // Configure / set up LEDs and interrupts
    sei(); // Globally enable interrupts

    uint16_t i = 0;

    while(1) {

        if(FL_UPDATE){

            if(i > PWM_MAX || i == 65535){
                i = 0;
            }

            *LEDS[0].red = i;
            *LEDS[0].blue = i;
            _delay_us(50);

            i++;
        }
    }
}
