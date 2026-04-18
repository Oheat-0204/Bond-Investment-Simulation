#include <iostream>
#include <iomanip>
#include <algorithm>
#include <vector>
#include <string>
#include <sstream> 
#include "utils.h"
#include "bond.h"
#include "market.h"
#include "portfolio.h"

using namespace std;

int main() {
    BondMarket market;
    Portfolio portfolio;
    string msg = "환영합니다! 명령어를 입력하세요. (gv/gd/gc: 시계열, gy: 현재 수익률곡선, ls: 필터링)";

    vector<double> history_value;
    vector<double> history_duration;
    vector<double> history_convexity;

    history_value.push_back(100000.0);
    history_duration.push_back(0.0);
    history_convexity.push_back(0.0);

    auto record_history = [&]() {
        double current_bond_val = 0.0;
        double current_dur_sum = 0.0;
        double current_conv_sum = 0.0;

        map<string, const Bond*> bond_lookup;
        for (const auto& b : market.bonds_in_market) bond_lookup[b.ticker] = &b;

        for (auto& pair : portfolio.holdings) {
            if (pair.second > 0 && bond_lookup.count(pair.first)) {
                const Bond* b = bond_lookup[pair.first];
                double m_mid, m_bid, m_ask, m_y, m_dur, m_conv;
                market.calculate_metrics(*b, m_mid, m_bid, m_ask, m_y, m_dur, m_conv);
                double val = pair.second * m_mid;
                current_bond_val += val;
                current_dur_sum += val * m_dur;
                current_conv_sum += val * m_conv;
            }
        }
        double current_asset = portfolio.cash + current_bond_val;
        double current_pd = (current_bond_val > 0) ? (current_dur_sum / current_bond_val) : 0.0;
        double current_pc = (current_bond_val > 0) ? (current_conv_sum / current_bond_val) : 0.0;

        history_value.push_back(current_asset);
        history_duration.push_back(current_pd);
        history_convexity.push_back(current_pc);
        };

    string filter_mode = "all";

    while (true) {
        clear_screen();
        string line(105, '=');
        cout << line << "\n";
        cout << "현재 시점: " << fixed << setprecision(2) << market.time << "년 | "
            << "기준금리(단기): " << market.r * 100.0 << "% | "
            << "신용스프레드 충격: " << setprecision(0) << market.spread_shock * 10000.0 << " bp\n";
        cout << "필터링 모드: [" << filter_mode << "]\n";
        cout << line << "\n\n[시장 상황]\n";

        cout << format_col("Ticker", 6, '^') << " "
            << format_col("Name(RTG)", 14, '^') << " "
            << format_col("Tenor", 10, '>') << " "
            << format_col("CPN", 8, '>') << " "
            << format_col("Status", 6, '^') << " "
            << format_col("Bid", 9, '>') << " "
            << format_col("Ask", 9, '>') << " "
            << format_col("YTM(Avg)", 9, '>') << " "
            << format_col("Duration", 10, '>') << " "
            << format_col("Convexity", 8, '>') << "\n";
        cout << string(105, '-') << "\n";

        map<string, double> current_mids;
        map<string, double> current_asks;
        map<string, double> current_bids;
        map<string, double> current_durations;
        map<string, double> current_convexities;

        auto compare_bonds = [](const Bond& a, const Bond& b) {
            if (abs(a.time_to_maturity - b.time_to_maturity) > 0.001)
                return a.time_to_maturity < b.time_to_maturity;
            return a.name < b.name;
            };

        vector<Bond> sorted_bonds = market.bonds_in_market;
        sort(sorted_bonds.begin(), sorted_bonds.end(), compare_bonds);

        for (const auto& b : sorted_bonds) {
            // 필터링 검사 로직
            bool match = false;
            if (filter_mode == "all") match = true;
            else if ((filter_mode == "gov" || filter_mode == "국채") && b.rating == "GOV") match = true;
            else if ((filter_mode == "ig" || filter_mode == "투자적격") && (b.rating == "AAA" || b.rating == "AA" || b.rating == "BBB")) match = true;
            else if ((filter_mode == "junk" || filter_mode == "정크") && (b.rating == "BB" || b.rating == "B" || b.rating == "C")) match = true;
            else if (b.rating == filter_mode) match = true;

            if (!match) continue;

            double mid, bid, ask, y, mod_dur, conv;
            market.calculate_metrics(b, mid, bid, ask, y, mod_dur, conv);

            current_mids[b.ticker] = mid;
            current_asks[b.ticker] = ask;
            current_bids[b.ticker] = bid;
            current_durations[b.ticker] = mod_dur;
            current_convexities[b.ticker] = conv;

            string status = b.on_the_run ? "ON" : "OFF";
            string display_name = b.name;
            if (b.name.substr(0, b.rating.length()) != b.rating ||
                (b.name.length() > b.rating.length() && b.name[b.rating.length()] != '_')) {
                display_name += "(" + b.rating + ")";
            }

            cout << format_col(b.ticker, 6, '^') << " "
                << format_col(display_name, 14, '<') << " "
                << format_col(double_to_string(b.time_to_maturity, 2), 10, '>') << " "
                << format_col(double_to_string(b.coupon_rate * 100.0, 2) + "%", 8, '>') << " "
                << format_col(status, 6, '^') << " "
                << format_col(double_to_string(ask, 2), 9, '>') << " "
                << format_col(double_to_string(bid, 2), 9, '>') << " "
                << format_col(double_to_string(y * 100.0, 2) + "%", 9, '>') << " "
                << format_col(double_to_string(mod_dur, 2), 10, '>') << " "
                << format_col(double_to_string(conv, 2), 8, '>') << "\n";
        }

        cout << "\n[내 포트폴리오]\n";
        double total_bond_value = 0.0;
        double weighted_duration_sum = 0.0;
        double weighted_convexity_sum = 0.0;

        cout << "보유 현금: " << fixed << setprecision(0) << portfolio.cash;
        if (portfolio.total_accumulated_coupon > 0) {
            cout << " (누적 세후 이자: " << fixed << setprecision(2) << portfolio.total_accumulated_coupon << ")";
        }
        if (portfolio.total_transaction_costs > 0) {
            cout << " (누적 거래비용: " << fixed << setprecision(2) << portfolio.total_transaction_costs << ")";
        }
        cout << "\n";

        for (auto& pair : portfolio.holdings) {
            string ticker = pair.first;
            int amt = pair.second;
            if (amt > 0) {
                // 시장 전체를 다시 순회하여 최신 메트릭을 가져옴 (필터링과 무관하게 계산 필요)
                for (const auto& b : market.bonds_in_market) {
                    if (b.ticker == ticker) {
                        double m_mid, m_bid, m_ask, m_y, m_dur, m_conv;
                        market.calculate_metrics(b, m_mid, m_bid, m_ask, m_y, m_dur, m_conv);

                        double val = amt * m_mid;
                        total_bond_value += val;
                        weighted_duration_sum += val * m_dur;
                        weighted_convexity_sum += val * m_conv;

                        cout << "- [" << ticker << "] " << b.name << "(" << b.rating << "): " << amt
                            << "개 (중간가 평가액: " << val << ", 듀레이션: " << setprecision(2) << m_dur << ")\n";
                        break;
                    }
                }
            }
        }

        double total_asset = portfolio.cash + total_bond_value;
        double portfolio_duration = (total_bond_value > 0) ? (weighted_duration_sum / total_bond_value) : 0.0;
        double portfolio_convexity = (total_bond_value > 0) ? (weighted_convexity_sum / total_bond_value) : 0.0;

        cout << string(105, '-') << "\n";
        cout << "채권 평가액: " << setprecision(0) << total_bond_value << " | 총 자산 가치: " << total_asset << "\n";
        cout << "채권 포트폴리오 듀레이션: " << setprecision(2) << portfolio_duration << "년 | 볼록성: " << portfolio_convexity << "\n";
        cout << string(105, '-') << "\n";
        cout << "최근 메시지: " << msg << "\n\n";

        cout << "명령어 (b/s [티커] [수량]), (n/y), (gv/gd/gc/gy), (ls [rating/category]) >> ";
        string raw_inp;
        getline(cin, raw_inp);

        if (raw_inp.empty()) continue;
        if (raw_inp == "exit") break;

        vector<string> cmds = split(raw_inp, ',');
        vector<string> msg_list;
        bool show_graph_val = false, show_graph_dur = false, show_graph_conv = false;

        for (string cmd_str : cmds) {
            istringstream iss(cmd_str);
            vector<string> inp;
            string token;
            while (iss >> token) inp.push_back(token);

            if (inp.empty()) continue;
            string cmd = inp[0];

            if (cmd == "ls") {
                if (inp.size() > 1) filter_mode = inp[1];
                else filter_mode = "all";
                msg_list.push_back("필터 변경: " + filter_mode);
            }
            else if (cmd == "next" || cmd == "n") {
                market.step();
                double earned = portfolio.receive_coupons(market.bonds_in_market, market.time);
                string event_msg = "";
                portfolio.process_events(market.matured_tickers, market.defaulted_tickers, event_msg);
                record_history();
                msg_list.push_back("다음 분기 이동(세후 이자 " + double_to_string(earned, 2) + ")");
                if (!event_msg.empty()) msg_list.push_back(event_msg);
            }
            else if (cmd == "year" || cmd == "y") {
                double total_earned = 0.0;
                string event_msg = "";
                for (int i = 0; i < 4; i++) {
                    market.step();
                    total_earned += portfolio.receive_coupons(market.bonds_in_market, market.time);
                    portfolio.process_events(market.matured_tickers, market.defaulted_tickers, event_msg);
                    record_history();
                }
                msg_list.push_back("1년 경과(총 세후 이자 " + double_to_string(total_earned, 2) + ")");
                if (!event_msg.empty()) msg_list.push_back(event_msg);
            }
            else if (cmd == "gv") { show_graph_val = true; }
            else if (cmd == "gd") { show_graph_dur = true; }
            else if (cmd == "gc") { show_graph_conv = true; }
            else if (cmd == "gy") {
                clear_screen();
                vector<double> sample_m;
                vector<double> sample_y;
                for (double m = 0.25; m <= 10.0; m += 0.25) {
                    sample_m.push_back(m);
                    sample_y.push_back(market.get_cir_spot_rate(m));
                }
                draw_yield_curve(sample_m, sample_y);
                cout << "엔터를 누르면 메인 화면으로 돌아갑니다...";
                string dummy;
                getline(cin, dummy);
            }
            else if ((cmd == "buy" || cmd == "b" || cmd == "bn" || cmd == "by") && inp.size() == 3) {
                string ticker = inp[1];
                int amt = stoi(inp[2]);

                // 현재 모든 채권을 뒤져서 티커가 존재하는지 확인하고 가격 정보를 가져옴
                double target_ask = -1.0, target_mid = -1.0;
                for (const auto& b : market.bonds_in_market) {
                    if (b.ticker == ticker) {
                        double dummy_bid, dummy_y, dummy_dur, dummy_conv;
                        market.calculate_metrics(b, target_mid, dummy_bid, target_ask, dummy_y, dummy_dur, dummy_conv);
                        break;
                    }
                }

                if (target_ask > 0 && portfolio.buy(ticker, amt, target_ask, target_mid)) {
                    msg_list.push_back("[" + ticker + "] " + to_string(amt) + "개 매수");
                    if (cmd == "bn") {
                        market.step();
                        double earned = portfolio.receive_coupons(market.bonds_in_market, market.time);
                        string event_msg = "";
                        portfolio.process_events(market.matured_tickers, market.defaulted_tickers, event_msg);
                        record_history();
                        msg_list.push_back("분기 이동(세후 이자 " + double_to_string(earned, 2) + ")");
                        if (!event_msg.empty()) msg_list.push_back(event_msg);
                    }
                }
                else {
                    msg_list.push_back("매수 실패(잔고 부족 혹은 티커 오류)");
                }
            }
            else if ((cmd == "sell" || cmd == "s" || cmd == "sn" || cmd == "sy") && inp.size() == 3) {
                string ticker = inp[1];
                int amt = stoi(inp[2]);

                double target_bid = -1.0, target_mid = -1.0;
                for (const auto& b : market.bonds_in_market) {
                    if (b.ticker == ticker) {
                        double dummy_ask, dummy_y, dummy_dur, dummy_conv;
                        market.calculate_metrics(b, target_mid, target_bid, dummy_ask, dummy_y, dummy_dur, dummy_conv);
                        break;
                    }
                }

                if (target_bid > 0 && portfolio.sell(ticker, amt, target_bid, target_mid)) {
                    msg_list.push_back("[" + ticker + "] " + to_string(amt) + "개 매도");
                    if (cmd == "sn") {
                        market.step();
                        double earned = portfolio.receive_coupons(market.bonds_in_market, market.time);
                        string event_msg = "";
                        portfolio.process_events(market.matured_tickers, market.defaulted_tickers, event_msg);
                        record_history();
                        msg_list.push_back("분기 이동(세후 이자 " + double_to_string(earned, 2) + ")");
                        if (!event_msg.empty()) msg_list.push_back(event_msg);
                    }
                }
                else {
                    msg_list.push_back("매도 실패(보유량 부족 혹은 티커 오류)");
                }
            }
            else {
                msg_list.push_back("잘못된 입력");
            }
        }

        if (show_graph_val || show_graph_dur || show_graph_conv) {
            clear_screen();
            if (show_graph_val) draw_ascii_graph(history_value, "총 자산 가치");
            if (show_graph_dur) draw_ascii_graph(history_duration, "듀레이션");
            if (show_graph_conv) draw_ascii_graph(history_convexity, "볼록성");

            cout << "엔터를 누르면 메인 화면으로 돌아갑니다...";
            string dummy;
            getline(cin, dummy);
        }

        msg = "";
        for (size_t i = 0; i < msg_list.size(); ++i) {
            msg += msg_list[i];
            if (i != msg_list.size() - 1) msg += " / ";
        }
    }

    return 0;
}