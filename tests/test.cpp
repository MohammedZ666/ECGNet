#include "model_pt.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <stdint.h>
#define TOLERANCE 1e-6
#define INDEX_TOLERANCE 3
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

int main()
{
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
    int max_index_fail_count = 0;

    while (getline(file, line))
    {
        float preprocessed[150]{0.0};
        float original[150]{0.0};

        vector<string> row = split(line, ',');
        int skip = 6;
        for (int i = skip + 60; i <= skip + 120; i++)
            preprocessed[i - skip] = stof(row[i]);

        skip = skip + 163;
        for (int i = skip; i <= skip + 149; i++)
            original[i - skip] = stof(row[i]);

        float res[150]{0.0};
        pt_algo(&res[0], &original[0]);

        for (int i = 60; i <= 120; i++)
        {
            if (abs(preprocessed[i] - res[i]) > TOLERANCE)
            {
                cout << "PanTomkins test failed for row " << row_index << "\n";
                cout << "Details: array index " << i << " of preprocessed-> " << preprocessed[i] << " and index " << i << " of MCU preproccesed-> " << res[i] << "\n";
                cout << "for tolerance " << TOLERANCE << "\n\n";
            }
        }
        // cout << "PanTomkins test passed for tolerance " << TOLERANCE << "\n";
        int preprocessed_peak_index = get_peak(preprocessed);
        int res_peak_index = get_peak(res);
        if (abs(preprocessed_peak_index - res_peak_index) > INDEX_TOLERANCE)
        {
            cout << "PanTomkins test failed for row " << row_index << "\n";
            cout << "Details: Maximum index of preprocessed " << preprocessed_peak_index << "\n";
            cout << "Details: Maximum index of res " << res_peak_index << "\n\n";
            max_index_fail_count++;
        }

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
    file.close();
    return 0;
}