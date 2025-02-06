#include "model_pt.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <stdint.h>
#define TOLERANCE 1e-6

using namespace std;

vector<string> split(const string &s, char delimiter)
{
    vector<string> tokens;
    string token;
    istringstream tokenStream(s);
    while (getline(tokenStream, token, delimiter))
    {
        tokens.push_back(token);
    }
    return tokens;
}
int make_inference(float *res, int i)
{
    float layer2in[10]{0.0};
    float output[4]{0.0};
    // return neural_net(&res[0], 60, &layer2in[0], &output[0]);

    int index = (int)i * 3 / 4;
    // printf("real %d mod %d\n", i, index);
    return neural_net(&res[0], 60, &layer2in[0], &output[0]);
}

int main()
{
    ifstream file("../data/mit_bih_preprocessed_and_original.csv");
    if (!file.is_open())
        throw runtime_error("Error opening file");

    char arrhythmia[] = {'N', 'S', 'V', 'F', '\0'};
    string line;
    getline(file, line);
    vector<string> columns = split(line, ',');
    int row_index = 0;
    int inference_fail_count = 0;

    while (getline(file, line))
    {
        float preprocessed[LEN]{0.0};
        float original[LEN]{0.0};
        char label = '\0';

        vector<string> row = split(line, ',');
        int skip = 2;
        for (int i = skip; i <= 5; i++)
        {
            if (stoi(row[i]) == 1)
            {
                label = arrhythmia[i - skip];
                break;
            }
        }

        skip = 6;
        for (int i = skip + 60; i <= skip + 120; i++)
            preprocessed[i - skip] = stof(row[i]);

        skip = skip + 163;
        for (int i = skip; i <= skip + 149; i++)
            original[i - skip] = stof(row[i]);

        float res[LEN]{0.0};
        pt_algo(&res[0], &original[0]);

        for (int i = 0; i < LEN; i++)
        {
            if (i < 60 || i > 120)
            {
                preprocessed[i] = res[i]; // normalizing preprocessed with values from res, as values from [0, 60) and (120, 150] are zero
            }
        }

        for (int i = 0; i < LEN; i++)
        {
            if (abs(preprocessed[i] - res[i]) > TOLERANCE)
            {
                throw runtime_error("Value mismatch for tolerance " + to_string(TOLERANCE) + " and row_index " + to_string(row_index) + "!");
            }
        }

        int preprocessed_peak_index = get_peak(preprocessed);
        int res_peak_index = get_peak(res);

        if (preprocessed_peak_index != res_peak_index)
            throw runtime_error("Peak mismatch for " + to_string(row_index) + "!");

        if (preprocessed_peak_index == -1 || res_peak_index == -1)
            throw runtime_error("Negative peak for " + to_string(row_index) + "!");

        char preprocessed_label = arrhythmia[make_inference(preprocessed, preprocessed_peak_index)];
        char res_label = arrhythmia[make_inference(res, res_peak_index)];

        if (preprocessed_label != res_label)
            throw runtime_error("Inference mismatch! Preprocessed label -> " + to_string(preprocessed_label) + " and res_label -> " + to_string(res_label) + "!");

        if (preprocessed_label != label)
            inference_fail_count++;
        row_index++;
    }

    printf("Inference success rate -> %lf\%\n", (row_index - inference_fail_count) * 100.0 / row_index); // row_index becomes length after the loop ends

    file.close();
    return 0;
}