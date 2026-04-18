#include "utils.h"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <vector>
#include <cmath>
#include <cstdlib>

using namespace std;

void draw_yield_curve(const vector<double>& maturities, const vector<double>& yields) {
    cout << "\n=== 현재 시장 수익률 곡선 (Term Structure) ===\n";
    double min_y = *min_element(yields.begin(), yields.end()) * 0.9;
    double max_y = *max_element(yields.begin(), yields.end()) * 1.1;

    int height = 20;
    int width = 40;

    for (int row = height; row >= 0; --row) {
        double threshold = min_y + (max_y - min_y) * ((double)row / height);
        cout << setw(8) << fixed << setprecision(2) << threshold * 100.0 << "% |";

        for (int col = 0; col <= width; ++col) {
            double current_m = (10.0 / width) * col; // 만기 0년 ~ 10년

            bool plot = false;
            for (size_t i = 0; i < maturities.size(); ++i) {
                if (abs(maturities[i] - current_m) < 0.15) {
                    double y_val = yields[i];
                    if (y_val >= threshold && y_val < threshold + (max_y - min_y) / height) {
                        plot = true; break;
                    }
                }
            }
            cout << (plot ? "*" : " ");
        }
        cout << "\n";
    }
    cout << "         +" << string(width + 1, '-') << "\n";
    cout << "         0        2.5       5.0       7.5       10.0 (년)\n\n";
} 

int get_display_width(const string& str) {
    int width = 0;
    for (size_t i = 0; i < str.length(); ) {
        unsigned char c = str[i];
        if (c >= 0xE0) { width += 2; i += 3; }
        else if (c >= 0xC0) { width += 1; i += 2; }
        else { width += 1; i += 1; }
    }
    return width;
}

string format_col(const string& text, int width, char align) {
    int display_width = get_display_width(text);
    int pad_len = width - display_width;
    if (pad_len < 0) pad_len = 0;

    if (align == '<') return text + string(pad_len, ' ');
    else if (align == '>') return string(pad_len, ' ') + text;
    else {
        int left_pad = pad_len / 2;
        return string(left_pad, ' ') + text + string(pad_len - left_pad, ' ');
    }
}

string double_to_string(double val, int precision) {
    ostringstream out;
    out << fixed << setprecision(precision) << val;
    return out.str();
}

vector<string> split(const string& s, char delimiter) {
    vector<string> tokens;
    string token;
    istringstream tokenStream(s);
    while (getline(tokenStream, token, delimiter)) {
        tokens.push_back(token);
    }
    return tokens;
}

void clear_screen() {
#ifdef _WIN32
    system("cls");
#else
    system("clear");
#endif
}

void draw_ascii_graph(const vector<double>& data, const string& title) {
    if (data.size() < 2) {
        cout << "\n[" << title << "] 데이터가 부족하여 그래프를 그릴 수 없습니다.\n";
        return;
    }

    cout << "\n=== " << title << " 추세 그래프 ===\n";
    double min_val = *min_element(data.begin(), data.end());
    double max_val = *max_element(data.begin(), data.end());

    if (max_val - min_val < 0.0001) {
        max_val += 0.1;
        min_val -= 0.1;
    }

    int height = 10;

    for (int row = height; row >= 0; --row) {
        double threshold_upper = min_val + (max_val - min_val) * ((double)row / height);
        double threshold_lower = min_val + (max_val - min_val) * ((double)(row - 1) / height);

        cout << setw(10) << fixed << setprecision(2) << threshold_upper << " | ";

        int print_count = min((int)data.size(), 60);
        int start_idx = max(0, (int)data.size() - print_count);

        for (int i = start_idx; i < data.size(); ++i) {
            if (data[i] >= threshold_lower && data[i] <= threshold_upper) {
                cout << "O ";
            }
            else if (data[i] > threshold_upper) {
                cout << "| ";
            }
            else {
                cout << "  ";
            }
        }
        cout << "\n";
    }
    cout << "            +" << string(min((int)data.size(), 60) * 2, '-') << "\n";
    cout << "             (최근 " << min((int)data.size(), 60) << "분기의 흐름)\n\n";
} 