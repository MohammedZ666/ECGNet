#include "utils.h"
#include "ptalgo.h"

int main(void)
{
    initECGModule();
    uart_init();
    init_millis(F_CPU);
    sei();

    float res[150]{0.0};
    int i = detect_qrs(res);
    send_unsigned_long((int)millis());
    int time = (int)millis();
    make_inference(&res[i], i);
    send_unsigned_long(((int)millis()) - time);
    return 0;
}