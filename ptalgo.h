#ifndef PTALGO_H
#define PTALGO_H
#define A0 1.0
#define A1 -1.73356294
#define A2 0.77567951
#define B0 0.11216024
#define B1 0
#define B2 -0.11216024
#define LEN 150
#define SPACING 70

int get_peak(float *res)
{
    int i = SPACING;
    int j = 1;
    int max_ind = 0;

    while (i < LEN - SPACING)
    {
        if (!(res[i] > res[i - j] && res[i] > res[i + j]))
        {
            i = i + j;
            j = 1;
            continue;
        }

        else if (j == SPACING)
        {
            max_ind = res[i] > res[max_ind] ? i : max_ind;
            i = i + SPACING;
        }
        else
        {
            max_ind = i;
        }

        j++;
    }
    return max_ind;
}
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
    // int n = LEN + integration_window - 1;
    int n = LEN; // LEN-1 due to 1st order difference
    for (int i = 0; i < n; i++)
    {
        int k = i < integration_window ? 0 : (i - integration_window) + 1;
        for (; k <= i; k++)
            res[i] += y[k];
    }
    return get_peak(&res[0]);
}
#endif