#ifndef PTALGO_H
#define PTALGO_H
#define A0 1.0
#define A1 -1.73356294
#define A2 0.77567951
#define B0 0.11216024
#define B1 0
#define B2 -0.11216024
#define LEN 150
#define SPACING 30
#define LIMIT 0.35
#define REFRACTORY_PERIOD 120
#define NOISE_FILT 0.125
#define QRS_NOISE_DIFF_WEIGHT 0.25
#define QRS_FILT 0.125

int pt_algo(float *res, float *y)
{
    float x_2 = 0.0; // delayed x, y samples
    float x_1 = 0.0;
    float y_1 = 0.0;
    float y_2 = 0.0;

    for (int i = 0; i < LEN; ++i)
    {
        float x_i = y[i];
        y[i] = (B0 * x_i + B1 * x_1 + B2 * x_2 - (A1 * y_1 + A2 * y_2)) / A0;

        x_2 = x_1;
        x_1 = x_i;
        y_2 = y_1;
        y_1 = y[i];
    }

    for (int i = 0; i < 5; i++)
        y[i] = y[5];

    for (int i = 0; i + 1 < LEN; i++)
    {
        y[i] = y[i + 1] - y[i];
        y[i] *= y[i];
    }

    int integration_window{15};
    // int n = (LEN - 1) + integration_window - 1;
    int n = LEN; // LEN-1 due to 1st order difference
    int max_len = LEN > integration_window ? LEN : integration_window;
    for (int i = 0; i < n; i++)
    {

        int kMax = i < max_len ? i : max_len;
        for (int k = 0; k <= kMax; k++)
        {
            if (k < LEN && (i - k) < integration_window)
            {
                res[i] += y[k];
            }
        }
    }

    float qrs_peak_est{0.0};
    float noise_peak_est{0.0};
    float noise_peak_val{0.0};
    int last_qrs_index{-1};
    float threshold{0.0};

    uint8_t i = 60;
    uint8_t j = 1;
    uint8_t max_ind = i;
    bool is_peak = false;

    while (j <= SPACING && i < LEN - 30)
    {
        if (!(res[i] > res[i - j] && res[i] > res[i + j]))
        {
            i = i + j;
            j = 1;
            continue;
        }

        else if (j == SPACING)
        {
            is_peak = true;
            max_ind = res[i] > res[max_ind] ? i : max_ind;
            i += SPACING + 1;
        }
        j++;
    }

    if (!is_peak)
        return -1;

    i = max_ind;
    if (last_qrs_index == -1 || i - last_qrs_index > REFRACTORY_PERIOD)
    {
        float current_peak_val = res[i];
        if (current_peak_val > threshold)
        {
            qrs_peak_est = QRS_FILT * current_peak_val + (1 - QRS_FILT) * qrs_peak_est;
            last_qrs_index = i;
            return i;
        }
        else
        {
            noise_peak_est = NOISE_FILT * current_peak_val + (1 - NOISE_FILT) * noise_peak_est;
        }
        threshold = noise_peak_est + QRS_NOISE_DIFF_WEIGHT * (qrs_peak_est - noise_peak_est);
    }
    return -1;
}
#endif