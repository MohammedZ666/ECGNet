#include "model_pt.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <stdint.h>
#define INDEX_TOLERANCE 10
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
    char arrhythmia[] = {'N', 'S', 'V', 'F', '\0'};
    ifstream file("../data/mit_bih_preprocessed_and_original.csv");
    if (!file.is_open())
    {
        cerr << "Error opening file" << endl;
        return 1;
    }

    string line;
    getline(file, line);
    vector<string> columns = split(line, ',');
    int row_index = 0;
    int inference_mismatch_count = 0;
    int inference_fail_count_preprocessed = 0;
    int inference_fail_count_res = 0;
    int max_index_fail_count = 0;

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

        // for (int i = 0; i < LEN; i++)
        // {
        //     if (i < 60 && i > 120)
        //     {
        //         preprocessed[i] = res[i]; // normalizing preprocessed with values from res, as values from [0, 60) and (120, 150] are zero
        //     }
        // }

        for (int i = 60; i <= 120; i++)
        {
            if (abs(preprocessed[i] - res[i]) > TOLERANCE)
            {
                cout << "PanTomkins test failed for row " << row_index << "\n";
                cout << "Details: \n";
                cout << "\tpreprocessed[" << i << "] = " << preprocessed[i] << " is not equal to res[" << i << "] = " << res[i] << " for tolerance = " << TOLERANCE << "\n";
                throw invalid_argument("Value mismatch!");
            }
        }

        int preprocessed_peak_index = get_peak(preprocessed);
        int res_peak_index = get_peak(res);
        if (abs(preprocessed_peak_index - res_peak_index) > INDEX_TOLERANCE)
        {
            cout << "PanTomkins test failed for row " << row_index << "\n";
            cout << "Details:\n";
            cout << "\tMaximum index of preprocessed " << preprocessed_peak_index << "\n ";
            cout << "\tMaximum index of res " << res_peak_index << "\n\n";
            max_index_fail_count++;
        }
        if (preprocessed_peak_index == -1 || res_peak_index == -1)
        {
            throw invalid_argument("received negative value");
        }

        char preprocessed_label = arrhythmia[make_inference(preprocessed, preprocessed_peak_index)];
        char res_label = arrhythmia[make_inference(res, res_peak_index)];
        if (preprocessed_label != res_label)
            inference_mismatch_count++;

        if (preprocessed_label != label)
            inference_fail_count_preprocessed++;
        if (res_label != label)
            inference_fail_count_res++;

        row_index++;
    }
    if (max_index_fail_count > 0)
    {
        cout << "Total Max Index Failures " << max_index_fail_count << " out of " << row_index << "\n";
        cout << "That is " << (1.0 * max_index_fail_count / row_index) << "\% of the MIT-BIH dataset" << "\n";
    }
    else
    {
        cout << "Pan-Tomkins Test Passed!\n";
    }
    printf("Inference mismatch between res and preprocessed %lf \%\n", inference_mismatch_count * 100.0 / row_index);
    printf("Inference fail of preprocessed %lf \%\n", inference_fail_count_preprocessed * 100.0 / row_index);
    printf("Inference fail of res %lf \%\n", inference_fail_count_res * 100.0 / row_index);

    file.close();
    return 0;
}