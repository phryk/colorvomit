#define F_CPU 16000000UL

#define LED_NUM 4
#define PWM_PERIOD_LENGTH 65535

// maximum PWM value, -1 to never have LEDs on *all* the time.
// This ought to make sure they won't go up in smoke.
#define PWM_MAX PWM_PERIOD_LENGTH - 1

//#define BAUD 57600
//#define BAUD 250000 
//#define BAUD 9600
#define BAUD 115200
#define LINE_LENGTH 54 // Maximum length in byte of a single command from USB

#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
//#include <util/setbaud.h> // Uses BAUD define
#include <util/delay.h>


void uart_putchar(char c, FILE *stream) {
    if (c == '\n') {
        uart_putchar('\r', stream);
    }

    loop_until_bit_is_set(UCSR0A, UDRE0);
    UDR0 = c;
}

FILE uart_output = FDEV_SETUP_STREAM(uart_putchar, NULL, _FDEV_SETUP_WRITE);


void uart_setup(void){

    UCSR0A = 0;
    UCSR0B = 0;
    UCSR0C = 0;

    UCSR0B = 0;
    UCSR0B |= (1 << RXEN0) | (1 << TXEN0); //| (1 << RXCIE0) | (1 << TXCIE0); // Enable transmissitting and receiving
    UCSR0C |= (1 << UCSZ01) | (1 << UCSZ00); // 8 bit per transmitted byte

    //UBRR0H = UBRRH_VALUE;
    //UBRR0L = UBRRL_VALUE;
    UBRR0H = 0;
    UBRR0L = 34; 
    UCSR0A |= (1 << U2X0);
    //UCSR0A &= ~(1 << U2X0);
}

void led_setup(void){

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
  DDRB |= (1<<PB7) | (1<<PB6) | (1<<PB5);  // Bit 5-7 als Ausgang
  DDRE |= (1<<PE3) | (1<<PE4) | (1<<PE5);  // Bit 3-5 als Ausgang
  DDRH |= (1<<PH3) | (1<<PH4) | (1<<PH5);  // Bit 3-5 als Ausgang
  DDRL |= (1<<PL3) | (1<<PL4) | (1<<PL5);  // Bit 3-5 als Ausgang
    
  //TIMSK1 |= (1>>TOIE1); // Let timer 1 trigger interrupt

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


/*ISR( TIMER1_OVF_vect ) {

    FL_UPDATE = 1;
}*/


int flag_set(int reg, int bit_pos){
    return (((1 << bit_pos) & (int)reg) == (1 << bit_pos));
}


int main(void){

    uart_setup();
    led_setup(); // Configure / set up LEDs and interrupts
    //sei(); // Globally enable interrupts

    stdout = &uart_output;

    // LED stuff
    //volatile uint16_t* channel;

    // uart / usb api stuff 
    uint8_t line_idx = 0;
    char line[LINE_LENGTH] = "";
    uint8_t odd = 1;

    while(1) {

        if(flag_set(UCSR0A, RXC0)){

            char c = UDR0;
            // Carriage return (enter)
            if((c == '\n') || ((int)c == 13) || (line_idx == LINE_LENGTH)){

                char *command_name = strtok(line, " ");

                if(strcmp(command_name,"FRAME") == 0){ // Is valid command

                    uint8_t i = 0;
                    uint8_t values[12] = {0,0,0, 0,0,0, 0,0,0, 0,0,0};
                    char* token;
                    while((token = strtok(NULL, " "))){
                        values[i] = atoi(token);
                        i++;
                    }

                    uint8_t sub = 0;
                    uint8_t led_num = 0;
                    volatile uint16_t* channel;

                    for(i = 0; i < 12; i++){

                        if((i > 0) && (i % 3 == 0)){
                            led_num++;
                            sub = 0;
                        }

                        switch(sub){

                            case 0:
                                channel = LEDS[led_num].red;
                                break;

                            case 1:
                                channel = LEDS[led_num].green;
                                break;

                            case 2:
                                channel = LEDS[led_num].blue;
                                break;
                        }

                        *channel = values[i] * 255;
                        sub++;
                    }

                    printf("OK\n");
                }

                // Reset line_idx and line
                line_idx = 0;
                memset(line, 0, sizeof(line));

            } else  {
                line[line_idx] = c;
                line_idx++;
            }
        }


        // LED Test code that works without shit from UART
/*        for(uint8_t l = 0; l < LED_NUM; l++){

            for(uint16_t x = 0; x < PWM_MAX; x++){

                *LEDS[l].red = x;
                *LEDS[l].green = x;
                *LEDS[l].blue = x;
                _delay_us(50);
            }
        }*/

    }
}
