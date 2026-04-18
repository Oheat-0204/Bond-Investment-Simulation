#pragma once
#ifndef UTILS_H
#define UTILS_H

#include <string>
#include <vector>

int get_display_width(const std::string& str);
std::string format_col(const std::string& text, int width, char align);
std::string double_to_string(double val, int precision);
std::vector<std::string> split(const std::string& s, char delimiter);
void clear_screen();
void draw_ascii_graph(const std::vector<double>& data, const std::string& title);
void draw_yield_curve(const std::vector<double>& maturities, const std::vector<double>& yields);

#endif