#ifndef PORTFOLIO_H
#define PORTFOLIO_H

#include "bond.h"
#include <map>
#include <vector>
#include <string>

class Portfolio {
public:
    double cash;
    double total_accumulated_coupon;
    double total_transaction_costs; // 누적 거래비용 변수 추가
    std::map<std::string, int> holdings;

    Portfolio();
    // 인자를 4개(mid_price 포함)로 업데이트
    bool buy(std::string ticker, int amount, double ask_price, double mid_price);
    bool sell(std::string ticker, int amount, double bid_price, double mid_price);
    double receive_coupons(const std::vector<Bond>& market_bonds, double current_time);
    void process_events(const std::vector<std::string>& matured, const std::vector<std::string>& defaulted, std::string& out_msg);
};

#endif