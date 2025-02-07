#ifndef PTALGO_H
#define PTALGO_H
#define A0 1.0
#define A1 -1.73356294
#define A2 0.77567951
#define B0 0.11216024
#define B1 0
#define B2 -0.11216024
#define LEN 150
#define SPACING 50

int get_peak(float *ecg_input)
{
    int i = SPACING;
    int j = 1;
    float max = -1.0;
    int max_ind = -1;

    while (i < LEN - SPACING)
    {
        if ((ecg_input[i] > ecg_input[i - j] && ecg_input[i] > ecg_input[i + j]))
        {
            if (ecg_input[i] > max)
            {
                max_ind = i;
                max = ecg_input[i];
            }

            if (j == SPACING)
            {
                i += SPACING;
                j = 1;
            }

            else
                j++;
        }
        else
            i++;
    }
    return max_ind;
}
int pt_algo(float *ecg_input)
{
    float y[LEN] = {0.0};
    for (int i = 0; i < LEN; i++)
    {
        y[i] = ecg_input[i];
        ecg_input[i] = 0;
    }

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
    // int n = LEN + integration_window - 1;
    int n = LEN; // LEN-1 due to 1st order difference
    for (int i = 0; i < n; i++)
    {
        int k = i < integration_window ? 0 : (i - integration_window) + 1;
        for (; k <= i; k++)
            ecg_input[i] += y[k];
    }
    return get_peak(ecg_input);
}
#endif