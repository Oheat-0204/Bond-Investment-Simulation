#include "market.h"
#include "utils.h"
#include <cmath>
#include <algorithm>
#include <iomanip>
#include <sstream>

using namespace std;

BondMarket::BondMarket() {
    time = 0.0;
    dt = 0.25;
    ticker_counter = 1;

    r = 0.035; theta = 0.035; kappa = 0.15; sigma_r = 0.012;
    spread_shock = 0.0; sigma_s = 0.025; rho = -0.75;

    base_spreads["GOV"] = 0.0;
    base_spreads["AAA"] = 0.006;
    base_spreads["AA"] = 0.012;
    base_spreads["BBB"] = 0.035;
    base_spreads["BB"] = 0.055;
    base_spreads["B"] = 0.090;
    base_spreads["C"] = 0.170;


    transition_matrix["AAA"] = { {"AAA", 0.97}, {"AA", 0.03}, {"BBB", 0.00}, {"BB", 0.00}, {"B", 0.00}, {"C", 0.00}, {"Default", 0.00} };
    transition_matrix["AA"] = { {"AAA", 0.02}, {"AA", 0.93}, {"BBB", 0.05}, {"BB", 0.00}, {"B", 0.00}, {"C", 0.00}, {"Default", 0.00} };
    transition_matrix["BBB"] = { {"AAA", 0.00}, {"AA", 0.04}, {"BBB", 0.88}, {"BB", 0.06}, {"B", 0.01}, {"C", 0.00}, {"Default", 0.01} };
    transition_matrix["BB"] = { {"AAA", 0.00}, {"AA", 0.00}, {"BBB", 0.05}, {"BB", 0.82}, {"B", 0.08}, {"C", 0.03}, {"Default", 0.02} };
    transition_matrix["B"] = { {"AAA", 0.00}, {"AA", 0.00}, {"BBB", 0.00}, {"BB", 0.05}, {"B", 0.77}, {"C", 0.11}, {"Default", 0.07} };
    transition_matrix["C"] = { {"AAA", 0.00}, {"AA", 0.00}, {"BBB", 0.00}, {"BB", 0.00}, {"B", 0.05}, {"C", 0.65}, {"Default", 0.30} };


    random_device rd;
    gen = mt19937(rd());
    dist = normal_distribution<double>(0.0, 1.0);

    issue_new_bonds();
}

double BondMarket::get_cir_spot_rate(double t) {
    if (t <= 0.0) return r;
    double gamma = sqrt(kappa * kappa + 2.0 * sigma_r * sigma_r);
    double exp_gamma_t = exp(gamma * t);

    double denominator = (gamma + kappa) * (exp_gamma_t - 1.0) + 2.0 * gamma;
    double B = 2.0 * (exp_gamma_t - 1.0) / denominator;

    double A_numerator = 2.0 * gamma * exp((kappa + gamma) * t / 2.0);
    double A = pow(A_numerator / denominator, 2.0 * kappa * theta / (sigma_r * sigma_r));

    double P = A * exp(-B * r);
    return -log(P) / t;
}

void BondMarket::step() {
    time += dt;

    double z1 = dist(gen);
    double z2 = dist(gen);
    double shock_r = z1;
    double shock_s = rho * z1 + sqrt(1.0 - rho * rho) * z2;

    double dr = kappa * (theta - r) * dt + sigma_r * sqrt(r) * shock_r * sqrt(dt);
    r = max(0.0001, r + dr);

    spread_shock = max(0.0, spread_shock * 0.85 + sigma_s * shock_s * sqrt(dt));

    matured_tickers.clear();
    defaulted_tickers.clear();

    uniform_real_distribution<double> unif(0.0, 1.0);

    auto new_end = remove_if(bonds_in_market.begin(), bonds_in_market.end(), [&](Bond& bond) {
        bond.time_to_maturity -= dt;
        if (bond.issue_time < time - 0.5) bond.on_the_run = false;

        if (bond.rating != "GOV" && !bond.defaulted) {
            double p = unif(gen);
            double cumulative = 0.0;
            for (auto const& [next_state, prob] : transition_matrix[bond.rating]) {
                cumulative += prob;
                if (p <= cumulative) {
                    if (next_state == "Default") bond.defaulted = true;
                    else bond.rating = next_state;
                    break;
                }
            }
        }

        if (bond.defaulted) {
            defaulted_tickers.push_back(bond.ticker);
            return true;
        }
        if (bond.time_to_maturity <= 0.001) {
            matured_tickers.push_back(bond.ticker);
            return true;
        }
        return false;
        });

    bonds_in_market.erase(new_end, bonds_in_market.end());
    issue_new_bonds();
}

void BondMarket::issue_new_bonds() {
    double maturities[] = { 0.25, 1.0, 3.0, 5.0, 10.0 };
    string ratings[] = { "GOV", "AAA", "AA", "BBB", "BB", "B"};

    for (double m : maturities) {
        for (string rating : ratings) {
            bool exists = false;
            for (const auto& b : bonds_in_market) {
                if (b.rating == rating && abs(b.time_to_maturity - m) < 0.01 && b.on_the_run) {
                    exists = true; break;
                }
            }
            if (!exists) {
                double current_spot_y = get_cir_spot_rate(m) + base_spreads[rating] + (rating != "GOV" ? spread_shock : 0.0);

                ostringstream ticker_ss;
                ticker_ss << setw(4) << setfill('0') << ticker_counter;

                string m_str = double_to_string(m, 2);
                if (m == 0.25) m_str = "0p25";
                else if (m == 1.0) m_str = "1p0";
                else if (m == 3.0) m_str = "3p0";
                else if (m == 5.0) m_str = "5p0";
                else if (m == 10.0) m_str = "10p0";

                Bond new_bond;
                new_bond.ticker = ticker_ss.str();
                new_bond.name = rating + "_" + m_str + "Y";
                new_bond.rating = rating;
                new_bond.issue_time = time;
                new_bond.time_to_maturity = m;
                new_bond.coupon_rate = round(current_spot_y * 10000.0) / 10000.0;
                new_bond.on_the_run = true;
                new_bond.defaulted = false;

                int periods = round(m / dt);
                for (int i = 1; i <= periods; ++i) {
                    double cf_time = time + i * dt;
                    double amt = (new_bond.coupon_rate * 100.0) * dt;
                    if (i == periods) amt += 100.0;
                    new_bond.cashflows.push_back({ cf_time, amt });
                }

                bonds_in_market.push_back(new_bond);
                ticker_counter++;
            }
        }
    }
}

void BondMarket::calculate_metrics(const Bond& bond, double& mid_price, double& bid_price, double& ask_price, double& y, double& mod_dur, double& convexity) {
    double current_spread = base_spreads[bond.rating] + (bond.rating != "GOV" ? spread_shock : 0.0);
    double liquidity_penalty = bond.on_the_run ? 0.0 : 0.0025;
    double total_spread = current_spread + liquidity_penalty;

    mid_price = 0.0;
    double duration_num = 0.0;
    double convexity_num = 0.0;

    y = 0.0;
    double cf_count = 0.0;

    for (const auto& cf : bond.cashflows) {
        if (cf.time <= time + 0.001) continue;

        double t = cf.time - time;
        double R = get_cir_spot_rate(t) + total_spread;
        double pv_cf = cf.amount * exp(-R * t);

        mid_price += pv_cf;
        duration_num += t * pv_cf;
        convexity_num += t * t * pv_cf;

        y += R;
        cf_count += 1.0;
    }

    if (cf_count > 0) y /= cf_count;

    if (mid_price > 0) {
        mod_dur = duration_num / mid_price;
        convexity = convexity_num / mid_price;
    }
    else {
        mid_price = 100.0; mod_dur = 0.0; convexity = 0.0; y = get_cir_spot_rate(0.1) + total_spread;
    }

    double ba_spread = 0.02;
    if (bond.rating == "AAA") ba_spread = 0.05;
    else if (bond.rating == "AA") ba_spread = 0.10;
    else if (bond.rating == "BBB") ba_spread = 0.25;
    else if (bond.rating == "BB") ba_spread = 0.50;
    else if (bond.rating == "B")  ba_spread = 1.00;
    else if (bond.rating == "C")  ba_spread = 3.00;

    if (!bond.on_the_run) ba_spread *= 2.0;

    ask_price = mid_price + ba_spread / 2.0;
    bid_price = mid_price - ba_spread / 2.0;
}