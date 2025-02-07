#include "utils.h"
#include "sampledata.h"

int main(void)
{
    initECGModule();
    uart_init();
    init_millis(F_CPU);
    sei();

    float *ecg_input = SAMPLE_INPUT_F;
    int time = (int)millis();
    unsigned long peak = pt_algo(ecg_input);
    send_unsigned_long(((int)millis() - time));
    send_unsigned_long(peak);
    send_unsigned_long(PEAK_F);
    time = (int)millis();
    int i = make_inference(ecg_input, 60);
    send_unsigned_long(((int)millis()) - time);
    char arrhythmia[] = {'N', 'S', 'V', 'F', '\0'};
    send_char(arrhythmia[i]);
    send_char('\n');

    return 0;
}