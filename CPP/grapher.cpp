#include <set>

extern "C" {
#include "../CPP/grapher.h"
}

#include <string>
#include <vector>
#include <span>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcast-qual"
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#pragma GCC diagnostic ignored "-Wfloat-equal"
#pragma GCC diagnostic ignored "-Wconversion"
#include "matplotlibcpp.h"
#pragma GCC diagnostic pop

#include "../C/ranks.h"

namespace plt = matplotlibcpp;

void plot_ranks(const std::span<StockRanks*> all_stock_ranks) {
    plt::title("Stock Ranks");
    plt::xlabel("Date");
    plt::ylabel("Rank");

    std::set<int> unique_dates;
    std::map<int, std::string> date_labels;
    for (const auto& stock_rank : all_stock_ranks) {
        if (stock_rank != nullptr) {
            for (size_t i = 0; i < stock_rank->size; ++i) {
                const Date* d = stock_rank->dates[i];
                int date_int = d->year * 10000 + d->month * 100 + d->day;
                unique_dates.insert(date_int);
                date_labels[date_int] = std::to_string(date_int);
            }
        }
    }

    // Convert unique_dates to vector for indexing
    const std::vector all_dates(unique_dates.begin(), unique_dates.end());
    std::vector<std::string> all_labels;
    all_labels.reserve(all_dates.size());
    for (int d : all_dates) {
        all_labels.push_back(date_labels[d]);
    }

    // Plot each stock, align ranks to the all_dates axis, put NAN if missing
    for (const auto& stock_rank : all_stock_ranks) {
        if (stock_rank == nullptr || stock_rank->size == 0) continue;

        // Map date -> rank for this stock
        std::map<int, double> rank_map;
        for (size_t i = 0; i < stock_rank->size; ++i) {
            const Date* d = stock_rank->dates[i];
            int date_int = d->year * 10000 + d->month * 100 + d->day;
            rank_map[date_int] = static_cast<double>(stock_rank->rank_per_day[
                i]);
        }
        std::vector<double> y;
        y.reserve(all_dates.size());
        for (int d : all_dates) {
            if (auto it = rank_map.find(d); it != rank_map.end()) {
                y.push_back(it->second);
            } else {
                y.push_back(std::nan("")); // missing data
            }
        }
        std::vector<double> x_numeric(all_dates.size());
        for (size_t i = 0; i < all_dates.size(); ++i) {
            x_numeric[i] = static_cast<double>(i);
        }
        plt::plot(
            x_numeric,
            y,
            {
                {"label", stock_rank->stock_symbol},
                {"linewidth", "0.5"}
            }
        );
    }
    std::vector<double> x_positions(all_dates.size());
    for (size_t i = 0; i < all_dates.size(); i += 1) {
        x_positions[i] = static_cast<double>(i);
    }
    plt::xticks(x_positions, all_labels);
    plt::show();
}

extern "C" {
void convert_and_plot(SymbolToRanksHashMap* all) {
    const std::span stocks(
        all->symbol_to_ranks,
        RANK_MAP_SIZE
    );
    plot_ranks(stocks);
}
}
