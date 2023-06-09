#define __AVR_ATmega328P__
#include <avr/io.h>
#include <util/delay.h>
#include <stdlib.h>
#include "model.h"
#define BAUD_RATE 9600
#define F_CPU 16000000UL
#define UBRR_VALUE ((F_CPU / (16UL * BAUD_RATE)) - 1)

float res[150]{0.0};
int index = -1;
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
int detect_qrs()
{
    int len = 150;
    float y[len];
    float x_2 = 0.0;
    float x_1 = 0.0;
    float y_1 = 0.0;
    float y_2 = 0.0;

    const float a0 = 1.0;
    const float a1 = -1.73356294;
    const float a2 = 0.77567951;
    const float b0 = 0.11216024;
    const float b1 = 0;
    const float b2 = -0.11216024;

    for (int i = 0; i < len;)
    {

        if (checkECGUnavailable())
        {
        }

        else
        {

            // float x_i = (analogRead() / 511.5) - 1;
            float x_i = SAMPLE_INPUT_F[i];
            y[i] = (b0 * x_i + b1 * x_1 + b2 * x_2 - (a1 * y_1 + a2 * y_2)) / a0;
            x_2 = x_1;
            x_1 = x_i;
            y_2 = y_1;
            y_1 = y[i];
            ++i;
        }
        _delay_us(120);
    }

    for (int i = 0; i < 5; i++)
        y[i] = y[5];

    for (int i = 0; i + 1 < len; i++)
    {
        y[i] = y[i + 1] - y[i];
        y[i] *= y[i];
    }

    len--;

    int integration_window{15};
    //  int n = len + integration_window - 1;
    int n = len;
    int max_len = len > integration_window ? len : integration_window;
    for (int i = 0; i < n; i++)
    {

        int kMax = i < max_len ? i : max_len;
        for (int k = 0; k <= kMax; k++)
        {
            if (k < len && (i - k) < integration_window)
            {
                res[i] += y[k];
            }
        }
    }

    int spacing{50};
    float limit{0.35};
    float qrs_peak_est{0.0};
    float noise_peak_est{0.0};
    float noise_peak_val{0.0};
    float qrs_filt{0.125};
    float noise_filt{0.125};
    float qrs_noise_diff_weight{0.25};
    float last_qrs_index{-1};
    int refractory_period{120};
    float threshold{0.0};

    for (int i = 0; i < n; i++)
    {
        int lookup = (i - 1) - spacing < 0 ? i : spacing;
        bool is_broken = false;
        for (int j = 1; j <= lookup; j++)
        {
            if (!(res[i] > res[i - j]))
            {
                is_broken = true;
                break;
            }
        }
        if (is_broken)
            continue;

        lookup = (i + 1) + spacing > n - 1 ? (n - 1) - i : spacing;
        for (int j = 1; j <= lookup; j++)
        {
            if (!(res[i] > res[i + j]))
            {
                is_broken = true;
                break;
            }
        }
        if (!is_broken && res[i] > limit)
        {
            if (last_qrs_index == -1 || i - last_qrs_index > refractory_period)
            {
                float current_peak_val = res[i];
                if (current_peak_val > threshold)
                {
                    last_qrs_index = i;
                    return i;
                    // qrs_peak_est = qrs_filt * current_peak_val + (1 - qrs_filt) * qrs_peak_est;
                }
                else
                {
                    noise_peak_est = noise_filt * current_peak_val + (1 - noise_filt) * noise_peak_est;
                }
                threshold = noise_peak_est + qrs_noise_diff_weight * (qrs_peak_est - noise_peak_est);
            }
        }
    }
    return -1;
}

void led_blink()
{
    DDRB |= (1 << PB5);
    if (index == 0)
    {
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
}

void send_float(float x)
{
    char myString[12];
    dtostrf(x, 11, 8, myString);

    int i = 0;
    while (myString[i] != '\0')
    {
        uart_transmit(myString[i++]);
        _delay_us(834);
    }
}
int main(void)
{
    initECGModule();
    float layer2in[SAMPLE_IN_LEN0 * LAYER0_KERNEL_DIM1]{0.0};
    float output[SAMPLE_IN_LEN0 * LAYER1_KERNEL_DIM1]{0.0};

    int k = detect_qrs();
    dense(layer2in, &res[k - 30], LAYER0_KERNEL, LAYER0_BIAS, SAMPLE_IN_LEN0, SAMPLE_IN_LEN1, LAYER0_KERNEL_DIM0, LAYER0_KERNEL_DIM1);
    dense(output, layer2in, LAYER1_KERNEL, LAYER1_BIAS, SAMPLE_IN_LEN0, LAYER0_KERNEL_DIM1, LAYER1_KERNEL_DIM0, LAYER1_KERNEL_DIM1);
    index = argmax(output, SAMPLE_IN_LEN0 * LAYER1_KERNEL_DIM1);

    uart_init();
    send_float((float)k);
    for (uint8_t i = 0; i < SAMPLE_IN_LEN0 * LAYER1_KERNEL_DIM1; i++)
    {
        send_float(output[i]);
    }
}