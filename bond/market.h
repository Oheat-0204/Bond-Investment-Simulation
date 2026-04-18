#pragma once
#ifndef MARKET_H
#define MARKET_H

#include "bond.h"
#include <map>
#include <vector>
#include <string>
#include <random>

class BondMarket {
public:
    double time;
    double dt;
    int ticker_counter;

    double r, theta, kappa, sigma_r;
    double spread_shock, sigma_s, rho;
    std::map<std::string, double> base_spreads;
    std::map<std::string, std::map<std::string, double>> transition_matrix;

    std::vector<Bond> bonds_in_market;

    std::vector<std::string> matured_tickers;
    std::vector<std::string> defaulted_tickers;

    std::mt19937 gen;
    std::normal_distribution<double> dist;

    BondMarket();
    double get_cir_spot_rate(double t);
    void step();
    void issue_new_bonds();
    void calculate_metrics(const Bond& bond, double& mid_price, double& bid_price, double& ask_price, double& y, double& mod_dur, double& convexity);
};

#endif