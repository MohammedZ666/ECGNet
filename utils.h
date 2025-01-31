#ifndef UTILS_H
#define UTILS_H
#define __AVR_ATmega328P__
#include <avr/io.h>
#include <util/delay.h>
// #include <stdlib.h>
#include <time.h>
#include <util/atomic.h>
// #include <avr/interrupt.h>
#include "ptalgo.h"
#include "model.h"
#define BAUD_RATE 9600
#define F_CPU 16000000UL
#define UBRR_VALUE ((F_CPU / (16UL * BAUD_RATE)) - 1)
volatile unsigned long timer1_millis;
ISR(TIMER1_COMPA_vect)
{
    timer1_millis++;
}

void init_millis(unsigned long f_cpu)
{
    unsigned long ctc_match_overflow;

    ctc_match_overflow = ((f_cpu / 1000) / 8); // when timer1 is this value, 1ms has passed

    // (Set timer to clear when matching ctc_match_overflow) | (Set clock divisor to 8)
    TCCR1B |= (1 << WGM12) | (1 << CS11);

    // high byte first, then low byte
    OCR1AH = (ctc_match_overflow >> 8);
    OCR1AL = ctc_match_overflow;

    // Enable the compare match interrupt
    TIMSK1 |= (1 << OCIE1A);

    // REMEMBER TO ENABLE GLOBAL INTERRUPTS AFTER THIS WITH sei(); !!!
}

unsigned long millis(void)
{
    unsigned long millis_return;

    // Ensure this cannot be disrupted
    ATOMIC_BLOCK(ATOMIC_FORCEON)
    {
        millis_return = timer1_millis;
    }
    return millis_return;
}

void uart_init()
{
    // Set baud rate
    UBRR0H = (UBRR_VALUE >> 8);
    UBRR0L = UBRR_VALUE;

    // Enable transmitter
    UCSR0B |= (1 << TXEN0);

    // Set data frame format: 8 data bits, no parity, 1 stop bit
    UCSR0C |= (1 << UCSZ01) | (1 << UCSZ00);
}

void uart_transmit(char data)
{
    // Wait for the transmit buffer to be empty
    while (!(UCSR0A & (1 << UDRE0)))
        ;

    // Transmit the data
    UDR0 = data;
}
bool checkECGUnavailable()
{ // checking pin 11 or 10 being high
    return (PORTB & (1 << PORTB2)) != 0 || (PORTB & (1 << PORTB3)) != 0;
}
uint16_t analogRead()
{
    // Start the conversion
    ADCSRA |= (1 << ADSC);

    // Wait for the conversion to complete
    while (ADCSRA & (1 << ADSC))
        ;

    // Read the digital value
    uint16_t analogValue = ADC;
    return analogValue;
}

void initECGModule()
{
    DDRB &= ~(1 << DDB2); // pin 10 set to read
    DDRB &= ~(1 << DDB3); // pin 11 set to read

    // Set the reference voltage to the default (5V)
    ADMUX &= ~(1 << REFS1);
    ADMUX |= (1 << REFS0);

    // Configure analog input pin A0
    ADMUX &= 0xF0;
    ADMUX |= 0x00;

    // Set ADC prescaler to 128 and enable ADC
    ADCSRA |= (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0) | (1 << ADEN);
}

void led_blink()
{
    DDRB |= (1 << PB5);
    while (1)
    {
        // Turn on the LED
        PORTB |= (1 << PB5);

        // Delay for 1 second
        _delay_ms(1000);

        // Turn off the LED
        PORTB &= ~(1 << PB5);

        // Delay for 1 second
        _delay_ms(1000);
    }
}

void send_float(float x)
{
    int len = sizeof(float) * 8 + 3;
    char myString[len];
    dtostrf(x, -(len - 3), 3, myString);

    int i = 0;
    while (myString[i] != '\0')
    {
        uart_transmit(myString[i++]);
        _delay_us(834);
    }
    uart_transmit('\r');
    _delay_us(834);
    uart_transmit('\n');
    _delay_us(834);
}

int send_unsigned_long(unsigned long x)
{
    char myString[sizeof(unsigned long) * 8 + 1];
    ultoa(x, myString, 10);

    int i = 0;
    while (myString[i] != '\0')
    {
        uart_transmit(myString[i++]);
        _delay_us(834);
    }
    uart_transmit('\r');
    _delay_us(834);
    uart_transmit('\n');
    _delay_us(834);

    return (int)ceil(((i + 2) * 834.0 / 1000));
}
int detect_qrs(float *res)
{
    float y[LEN]{0.0};
    for (int i = 0; i < LEN; i++)
    {
        while (checkECGUnavailable())
            ;
        y[i] = (analogRead() / 511.5) - 1;
    }
    pt_algo(&res[0], &y[0]);
}

int make_inference(float *res, int i)
{

    float layer2in[10]{0.0};
    float output[4]{0.0};
    int index = neural_net(&res[0], i, &layer2in[0], &output[0]);

    for (uint8_t i = 0; i < 4; i++)
    {
        send_float(output[i]);
    }
}

#endif
