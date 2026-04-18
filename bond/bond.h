#pragma once
#ifndef BOND_H
#define BOND_H

#include <string>
#include <vector>

struct CashFlow {
    double time;
    double amount;
};

struct Bond {
    std::string ticker;
    std::string name;
    std::string rating;
    double issue_time;
    double time_to_maturity;
    double coupon_rate;
    bool on_the_run;
    bool defaulted;
    std::vector<CashFlow> cashflows;
};

#endif