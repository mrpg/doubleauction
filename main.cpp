/*
 * main.cpp, part of doubleauction
 *
 * (c) Max R. P. Grossmann, 2018.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published
 * by the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 *
*/

#include <iostream>
#include <cmath>
#include <cstdbool>
#include <utility>
#include <cstdint>
#include <vector>
#include <algorithm>
#include <chrono>
#include <cassert>
#include <cstring>
#include <functional>
#include <fstream>
#include <type_traits>

typedef uintmax_t PRICE; // to be imposed by regulator
typedef intmax_t QUANTITY;

inline QUANTITY next(QUANTITY x) {
	return x+1;
}

typedef struct order {
	char type;
	uint64_t id, time;
	PRICE price;
	QUANTITY quant;
	bool del = false;
} ORDER;

typedef std::vector<ORDER> ORDERBOOK;
typedef std::pair<ORDERBOOK, ORDERBOOK> MARKET;

PRICE split_the_difference(PRICE a, PRICE b) {
	return (a+b)/2;
}

bool order_predicate(const ORDER& a, const ORDER& b) {
	return ((a.type == 'B' && (a.price > b.price || (a.price == b.price && a.time < b.time))) || (a.type == 'S' && (a.price < b.price || (a.price == b.price && a.time < b.time))));
}

void sort_orderbooks(MARKET& market) {
	std::sort(market.first.begin(), market.first.end(), order_predicate);
	std::sort(market.second.begin(), market.second.end(), order_predicate);
}

std::pair<PRICE, bool> quote(const ORDERBOOK& sorted_ob, QUANTITY desired_q) {
	PRICE last_price(0);

	for (const ORDER& cur: sorted_ob) {
		last_price = cur.price;
		desired_q -= cur.quant;

		if (desired_q <= 0) break;
	}

	return std::make_pair(last_price, desired_q <= 0);
}

std::tuple<bool, PRICE, QUANTITY> is_feasible(const MARKET& market, QUANTITY trial_q, std::function<PRICE(PRICE, PRICE)> pricing) {
	std::pair<PRICE, bool> ret1, ret2;

	ret1 = quote(market.first, trial_q);
	ret2 = quote(market.second, trial_q);

	if (ret1.second && ret2.second) {
		if (ret1.first >= ret2.first) {
			return std::make_tuple(true, pricing(ret2.first, ret1.first), trial_q);
		}
	}

	return std::make_tuple(false, 0, 0);
}

std::tuple<bool, PRICE, QUANTITY> double_auction(MARKET& market, std::function<PRICE(PRICE, PRICE)> pricing) {
	if (market.first.size() == 0 || market.second.size() == 0) {
		return std::make_tuple(false, 0, 0);
	}

	// sort both orderbooks

	sort_orderbooks(market);

	// determine clearing price:

	// find all possible quantities in equilibrium...

	std::vector<QUANTITY> qcum;
	qcum.reserve(market.first.size()+market.second.size());

	for (const ORDERBOOK* ob: {&market.first, &market.second}) {
		bool newob = true;

		for (const ORDER& cur: *ob) {
			if (newob) {
				qcum.push_back(cur.quant);
				newob = false;
			}
			else {
				qcum.push_back(qcum[qcum.size()-1]+cur.quant);
			}
		}
	}

	std::sort(qcum.begin(), qcum.end(), std::greater<QUANTITY>());
	std::tuple<bool, PRICE, QUANTITY> eq;

	// bisect...
	// we try to find the largest feasible quantity

	size_t region_a = 0, region_b = qcum.size()-1, trial;

	while (region_b-region_a > 3) {
		trial = (region_a+region_b)/2;
		eq = is_feasible(market, qcum[trial], pricing);

		if (std::get<0>(eq)) {
			region_b = trial;
		}
		else {
			region_a = trial;
		}
	}

	for (size_t j = region_a; j <= region_b; j++) {
		eq = is_feasible(market, qcum[j], pricing);

		if (j == region_a && region_b != region_a) assert(!std::get<0>(eq)); // todo?

		if (std::get<0>(eq)) break;
	}

	// transact...

	if (std::get<0>(eq)) { // equilibrium exists
		assert(!std::get<0>(is_feasible(market, next(std::get<2>(eq)), pricing))); // assert optimality

		for (ORDERBOOK* ob: {&market.first, &market.second}) {
			QUANTITY q_req = std::get<2>(eq);

			for (ORDER& cur: *ob) {
				assert((cur.type == 'S' && cur.price <= std::get<1>(eq)) || (cur.type == 'B' && cur.price >= std::get<1>(eq))); // assert individual rationality

				if (cur.quant <= q_req) { // total fulfillment
					cur.del = true;
				}
				else { // partial fulfillment
					cur.quant -= q_req;
					q_req = 0;
					break;
				}

				if ((q_req -= cur.quant) <= 0) break;
			}

			assert(q_req <= 0); // assert feasibility

			ob->erase(std::remove_if(ob->begin(), ob->end(), [](const ORDER& co){return co.del;}), ob->end());
		}
	}

	// and return new order book...

	return eq;
}

void print_market(const MARKET& market, std::ostream& out) {
	for (const ORDERBOOK* ob: {&market.first, &market.second}) {
		for (const ORDER& co: *ob) {
			out << co.type << ' ' << co.price << ' ' << co.quant << ' ' << co.id << ' ' << co.time << std::endl;
		}
	}
}

inline std::chrono::time_point<std::chrono::high_resolution_clock> now() {
	return std::chrono::high_resolution_clock::now();
}

inline double deltaT(std::chrono::time_point<std::chrono::high_resolution_clock> a, std::chrono::time_point<std::chrono::high_resolution_clock> b) {
	return ((std::chrono::duration_cast<std::chrono::nanoseconds>(b-a).count())/1000000000.0);
}

int main(int argc, char** argv) {
	MARKET market;

	ORDER tmp;
	tmp.del = false;
	std::ifstream file;

	if (argc == 2) {
		if (strcmp(argv[1], "--help") != 0) {
			file.open(argv[1]);

			if (!file.is_open()) {
				std::cerr << "Error opening '" << argv[1] << "'. Aborting." << std::endl;
				goto USAGE;
			}
		}
		else {
			USAGE:

			std::cerr << "USAGE:" << std::endl;
			std::cerr << argv[0] << " [file]" << std::endl;
			exit(1);
		}
	}

	while ((file.is_open() ? file : std::cin) >> tmp.type >> tmp.price >> tmp.quant >> tmp.id >> tmp.time) {
		if (tmp.time == 0) {
			tmp.time = std::chrono::system_clock::now().time_since_epoch()/std::chrono::nanoseconds(1);
		}

		if (tmp.type == 'B') { // new buy order
			market.first.push_back(tmp);
		}
		else if (tmp.type == 'S') { // new sell order
			market.second.push_back(tmp);
		}
		else {
			// error
			std::cerr << "Invalid character '" << tmp.type << "'. Ignoring." << std::endl;
		}
	}

	// find equilibrium

	auto t1 = now();
	auto eq = double_auction(market, split_the_difference);
	auto elapsed = deltaT(t1, now());

	print_market(market, std::cout);

	if (std::get<0>(eq)) {
		std::cerr << "Equilibrium found at p=" << std::get<1>(eq) << ", q=" << std::get<2>(eq) << '.' << std::endl;
	}
	else {
		std::cerr << "No equilibrium found." << std::endl;
	}

	std::cerr << "Elapsed " << elapsed << "s." << std::endl;
}
