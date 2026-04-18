#include "portfolio.h"
#include <cmath>

using namespace std;

Portfolio::Portfolio() {
    cash = 100000.0;
    total_accumulated_coupon = 0.0;
    total_transaction_costs = 0.0; // 초기화
}

bool Portfolio::buy(string ticker, int amount, double ask_price, double mid_price) {
    double cost = amount * ask_price;
    if (cash >= cost) {
        cash -= cost;
        holdings[ticker] += amount;
        // 실제 지불한 가격(ask)과 가치(mid)의 차이를 비용으로 기록
        total_transaction_costs += (ask_price - mid_price) * amount;
        return true;
    }
    return false;
}

bool Portfolio::sell(string ticker, int amount, double bid_price, double mid_price) {
    if (holdings.count(ticker) && holdings[ticker] >= amount) {
        cash += amount * bid_price;
        holdings[ticker] -= amount;
        // 실제 받은 가격(bid)과 가치(mid)의 차이를 비용으로 기록
        total_transaction_costs += (mid_price - bid_price) * amount;
        return true;
    }
    return false;
}

double Portfolio::receive_coupons(const vector<Bond>& market_bonds, double current_time) {
    double total_coupon_after_tax = 0.0;
    map<string, const Bond*> bond_lookup;
    for (const auto& b : market_bonds) bond_lookup[b.ticker] = &b;

    for (auto& pair : holdings) {
        string ticker = pair.first;
        int amt = pair.second;
        if (amt <= 0) continue;

        if (bond_lookup.count(ticker)) {
            const Bond* b = bond_lookup[ticker];
            if (!b->defaulted) {
                for (const auto& cf : b->cashflows) {
                    if (abs(cf.time - current_time) < 0.001) {
                        double interest = cf.amount * amt;
                        if (abs(b->time_to_maturity) < 0.001) {
                            interest -= 100.0 * amt;
                        }
                        double after_tax = interest * (1.0 - 0.154);
                        cash += after_tax;
                        total_coupon_after_tax += after_tax;
                    }
                }
            }
        }
    }
    total_accumulated_coupon += total_coupon_after_tax;
    return total_coupon_after_tax;
}

void Portfolio::process_events(const vector<string>& matured, const vector<string>& defaulted, string& out_msg) {
    for (string ticker : matured) {
        if (holdings.count(ticker) && holdings[ticker] > 0) {
            int amt = holdings[ticker];
            cash += amt * 100.0;
            holdings.erase(ticker);
            out_msg += "[" + ticker + "] 만기 상환 완료(액면가) / ";
        }
    }
    for (string ticker : defaulted) {
        if (holdings.count(ticker) && holdings[ticker] > 0) {
            int amt = holdings[ticker];
            double recovery_value = amt * 40.0;
            cash += recovery_value;
            holdings.erase(ticker);
            out_msg += "[" + ticker + "] 부도 발생!(회수율 40%) / ";
        }
    }
}