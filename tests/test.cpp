#include "model_pt.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <stdint.h>
#include <cstring>
#define TOLERANCE 1e-6
#define REAL_TIME_INFERENCE 0
#define MCU_TEST_ARRAY_PATH "../sampledata.h"
using namespace std;
int beats_written = 0;
char arrhythmia[] = {'N', 'S', 'V', 'F', '\0'};

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
int make_inference(float *ecg_input, int i)
{
    float layer2in[10]{0.0};
    float output[4]{0.0};
    if (REAL_TIME_INFERENCE)
    {
        int index = (int)i * 3 / 4;
        return neural_net(&ecg_input[0], index, &layer2in[0], &output[0]);
    }
    return neural_net(&ecg_input[0], 60, &layer2in[0], &output[0]);
}

int write_beat_to_file(float *dataset_beat, char label, int peak)
{
    ofstream outFile(MCU_TEST_ARRAY_PATH, beats_written > 0 ? ios::app : ios::trunc);

    if (!outFile)
    {
        cerr << "Error opening file " << MCU_TEST_ARRAY_PATH << " for writing!" << std::endl;
        return 1;
    }
    if (beats_written == 0)
        outFile << "#ifndef SAMPLEDATA_H\n#define SAMPLEDATA_H\n\n";

    outFile << "float SAMPLE_INPUT_" << label << "[]=" << "{";
    for (int i = 0; i < LEN; i++)
    {

        outFile << dataset_beat[i];
        if (i < LEN - 1)
            outFile << ", ";
    }
    outFile << "};\n";
    outFile << "uint8_t PEAK" << "_" << label << " = " << peak << ";\n\n";
    beats_written++;

    if (beats_written == 4)
        outFile << "\n\n#endif\n";
    outFile.close();
    cout << "Data " << "for label \'" << label << "\' written to file successfully.\n";

    return 0;
}

int main()
{
    ifstream file("../data/mit_bih_preprocessed_and_original.csv");
    if (!file.is_open())
        throw runtime_error("Error opening file");
    bool is_array_saved_for_mcu_testing[4] = {};
    string line;
    getline(file, line);
    vector<string> columns = split(line, ',');
    int row_index = 0;
    int inference_fail_count = 0;

    while (getline(file, line))
    {
        float preprocessed[LEN]{0.0};
        float dataset_beat[LEN]{0.0};
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

        float ecg_input[LEN]{0.0};
        skip = skip + 163;
        for (int i = skip; i <= skip + 149; i++)
        {
            dataset_beat[i - skip] = stof(row[i]);
            ecg_input[i - skip] = dataset_beat[i - skip];
        }

        pt_algo(ecg_input);

        for (int i = 0; i < LEN; i++)
        {
            if (i < 60 || i > 120)
            {
                preprocessed[i] = ecg_input[i]; // normalizing preprocessed with values from ecg_input, as values from [0, 60) and (120, 150] are zero
            }
        }

        for (int i = 0; i < LEN; i++)
        {
            if (abs(preprocessed[i] - ecg_input[i]) > TOLERANCE)
            {
                throw runtime_error("Value mismatch for tolerance " + to_string(TOLERANCE) + " and row_index " + to_string(row_index) + "!");
            }
        }

        int preprocessed_peak_index = get_peak(preprocessed);
        int calculated_peak_index = get_peak(ecg_input);

        if (preprocessed_peak_index != calculated_peak_index)
            throw runtime_error("Peak mismatch for row_index " + to_string(row_index) + "!");

        if (preprocessed_peak_index == -1 || calculated_peak_index == -1)
            throw runtime_error("Negative peak for row_index " + to_string(row_index) + "!");

        char preprocessed_label = arrhythmia[make_inference(preprocessed, preprocessed_peak_index)];
        char calculated_label = arrhythmia[make_inference(ecg_input, calculated_peak_index)];

        if (preprocessed_label != calculated_label)
            throw runtime_error("Inference mismatch! Dataset-preprocessed label -> " + string(1, preprocessed_label) + ", where calculated label -> " + string(1, calculated_label) + " for row_index " + to_string(row_index) + ".");

        if (preprocessed_label != label)
            inference_fail_count++;
        else
        {
            for (int i = 0; i < 4; i++)
            {
                if (is_array_saved_for_mcu_testing[i] == false && label == arrhythmia[i])
                {
                    write_beat_to_file(dataset_beat, label, calculated_peak_index);
                    is_array_saved_for_mcu_testing[i] = true;
                }
            }
        }
        row_index++;
    }

    printf("Inference success rate -> %lf\%\n", (row_index - inference_fail_count) * 100.0 / row_index); // row_index becomes length after the loop ends
    file.close();
    return 0;
}