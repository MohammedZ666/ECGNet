#define __AVR_ATmega328P__
#include <avr/io.h>
#include <util/delay.h>
#include <stdlib.h>
#include "model.h"
#define BAUD_RATE 9600
#define F_CPU 16000000UL
#define UBRR_VALUE ((F_CPU / (16UL * BAUD_RATE)) - 1)

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
int detect_qrs(float *x, float *res)
{
    int len = 150;
    float y[len]{0.0};
    float x_2 = 0.0; // delayed x, y samples
    float x_1 = 0.0;
    float y_1 = 0.0;
    float y_2 = 0.0;

    const float a0 = 1.0;
    const float a1 = -1.73356294;
    const float a2 = 0.77567951;
    const float b0 = 0.11216024;
    const float b1 = 0;
    const float b2 = -0.11216024;

    for (int i = 0; i < len; ++i)
    {
        y[i] = (b0 * x[i] + b1 * x_1 + b2 * x_2 - (a1 * y_1 + a2 * y_2)) / a0;

        x_2 = x_1;
        x_1 = x[i];
        y_2 = y_1;
        y_1 = y[i];
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
    int n = len + integration_window - 1;
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

    int spacing{30};
    float limit{0.35};
    float qrs_peak_est{0.0};
    float noise_peak_est{0.0};
    float noise_peak_val{0.0};
    float qrs_filt{0.125};
    float noise_filt{0.125};
    float qrs_noise_diff_weight{0.25};
    float last_qrs_index{-1};
    float threshold{0.0};
    uint8_t refractory_period{120};
    uint8_t i = 60;
    uint8_t j = 1;
    uint8_t max_ind = i;
    bool is_peak = false;

    while (j <= spacing && i < len - 30)
    {
        if (!(res[i] > res[i - j] && res[i] > res[i + j]))
        {
            i = i + j;
            j = 1;
            continue;
        }

        else if (j == spacing)
        {
            is_peak = true;
            max_ind = res[i] > res[max_ind] ? i : max_ind;
            i += spacing + 1;
        }
        j++;
    }
    if (!is_peak)
        return -1;

    i = max_ind;
    if (last_qrs_index == -1 || i - last_qrs_index > refractory_period)
    {
        float current_peak_val = res[i];
        if (current_peak_val > threshold)
        {
            qrs_peak_est = qrs_filt * current_peak_val + (1 - qrs_filt) * qrs_peak_est;
            last_qrs_index = i;
            return i;
        }
        else
        {
            noise_peak_est = noise_filt * current_peak_val + (1 - noise_filt) * noise_peak_est;
        }
        threshold = noise_peak_est + qrs_noise_diff_weight * (qrs_peak_est - noise_peak_est);
    }
    return -1;
}

void softmax(float *x, int rows, int cols)
{
    float sum = 0;
    for (int i = 0; i < rows; i++)
    {
        for (int j = 0; j < cols; j++)
        {
            x[i * cols + j] = expf(x[i * cols + j]);
            sum += x[i * cols + j];
        }
    }
    for (int i = 0; i < rows; i++)
    {
        for (int j = 0; j < cols; j++)
        {
            x[i * cols + j] = x[i * cols + j] / sum;
        }
    }
}
void relu(float *x, int rows, int cols)
{
    for (int i = 0; i < rows; i++)
    {
        for (int j = 0; j < cols; j++)
        {
            x[i * cols + j] = x[i * cols + j] < 0.0f ? 0.0f : x[i * cols + j];
        }
    }
}

void sigmoid(float *x, int rows, int cols)
{
    for (int i = 0; i < rows; i++)
    {
        for (int j = 0; j < cols; j++)
        {
            x[i * cols + j] = 1 / (1 + expf(-1 * x[i * cols + j]));
        }
    }
}

void matmul(float *result, float *input, int8_t *kernel, uint8_t r1, uint8_t c1, uint8_t r2, uint8_t c2)
{

    for (int i = 0; i < r1; i++)
    {
        for (int j = 0; j < c2; j++)
        {
            for (int k = 0; k < c1; k++)
            {
                result[i * c1 + j] += input[i * c1 + k] * dequantize(kernel[k * c2 + j]);
            }
        }
    }
}

void dense(float *output, float *input, int8_t *kernel, int8_t *bias, uint8_t ir, uint8_t ic, uint8_t kr, uint8_t kc, char gate)
{
    matmul(output, input, kernel, ir, ic, kr, kc);
    for (int i = 0; i < ir; i++)
    {
        for (int j = 0; j < kc; j++)
        {
            output[i * kc + j] += dequantize(bias[i * kc + j]);
        }
    }

    if (gate == 's')
        sigmoid(output, ir, kc);
    else if (gate == 'r')
        relu(output, ir, kc);
    else
        softmax(output, ir, kc);
}

int argmax(float *output, int len)
{
    int max_ind = 0;
    float max = output[max_ind];

    for (int i = 1; i < len; i++)
    {
        if (output[i] > max)
        {
            max = output[i];
            max_ind = i;
        }
    }
    return max_ind;
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
    float res[149 + 15 - 1]{0.0};
    float layer2in[10]{0.0};
    float output[4]{0.0};

    int i = detect_qrs(SAMPLE_INPUT_N, res);
    dense(layer2in, &res[60], LAYER0_KERNEL, LAYER0_BIAS, 1, 61, 61, 10, 's');
    dense(output, layer2in, LAYER1_KERNEL, LAYER1_BIAS, 1, 10, 10, 4, 't');
    int index = argmax(output, 4);

    uart_init();
    send_float((float)i);
    for (uint8_t i = 0; i < 4; i++)
    {
        send_float(output[i]);
    }
}