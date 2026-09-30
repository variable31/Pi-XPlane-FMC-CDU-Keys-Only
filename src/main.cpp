/*
 This file is part of Pi-XPlane-FMC-CDU-Keys-Only
 A Raspberry Pi-based External FMC for XPlane

 Copyright (C) 2019 shahada abubakar
 <shahada@abubakar.net>
 Copyright (C) 2026 Pi-XPlane-FMC-CDU-Keys-Only contributors
 (2026: config-file keymap, kernel GPIO chardev, debounce, beacon re-discovery)

 This program is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program.  If not, see <https://www.gnu.org/licenses/>.

 */

#include "config.h"
#include "gpio_chardev.h"
#include "gpio_fake.h"
#include "keypad_scanner.h"
#include "xplane_link.h"

#include <atomic>
#include <csignal>
#include <cstring>
#include <iostream>
#include <memory>
#include <sstream>
#include <thread>

#ifndef FLIGHTSIM_DEFAULT_CONFIG
#define FLIGHTSIM_DEFAULT_CONFIG "/etc/flight-simulator/keys.conf"
#endif
#ifndef FLIGHTSIM_VERSION
#define FLIGHTSIM_VERSION "dev"
#endif

using namespace flightsim;
using namespace std::chrono;

namespace {

std::atomic<bool> stopRequested(false);

void onSignal(int) {
	stopRequested = true;
}

void usage(const char * prog) {
	std::cerr << "Usage: " << prog << " [options]\n"
			"  -c, --config PATH     keymap/config file (default "
			FLIGHTSIM_DEFAULT_CONFIG ")\n"
			"  -n, --dry-run         log key presses, do not contact X-Plane\n"
			"      --fake-keys KEYS  no GPIO: simulate pressing each row,col in\n"
			"                        KEYS (e.g. \"1,1 5,3\") once, then exit\n"
			"  -V, --version         print version and exit\n"
			"  -h, --help            this help\n";
}

std::vector<std::pair<int, int>> parseFakeKeys(const std::string & text) {
	std::vector<std::pair<int, int>> keys;
	std::istringstream in(text);
	std::string tok;
	while (in >> tok) {
		int r = 0, c = 0;
		char comma = 0;
		std::istringstream t(tok);
		if (!(t >> r >> comma >> c) || comma != ',' || r < 1 || c < 1) {
			throw std::runtime_error("bad --fake-keys entry \"" + tok
					+ "\" (expected row,col)");
		}
		keys.push_back( { r, c });
	}
	return keys;
}

// Drives a FakeGpioMatrix: each key is held for 100 ms then released for
// 100 ms. Returns false once the script is finished.
class FakeKeyScript {
public:
	FakeKeyScript(FakeGpioMatrix & _m, std::vector<std::pair<int, int>> _keys,
			steady_clock::time_point start) :
			m(_m), keys(std::move(_keys)), t0(start) {
	}

	bool advance(steady_clock::time_point now) {
		long step = (long) duration_cast<milliseconds>(now - t0).count() / 100;
		size_t idx = (size_t) (step / 2);
		m.releaseAll();
		if (idx >= keys.size()) {
			// one extra idle step so the final release gets debounced
			return step < (long) (keys.size() * 2 + 2);
		}
		if (step % 2 == 0) {
			m.press(keys[idx].first, keys[idx].second);
		}
		return true;
	}

private:
	FakeGpioMatrix & m;
	std::vector<std::pair<int, int>> keys;
	steady_clock::time_point t0;
};

}

int main(int argc, char * argv[]) {

	std::string configPath = FLIGHTSIM_DEFAULT_CONFIG;
	bool dryRun = false;
	bool fake = false;
	std::string fakeKeys;

	for (int i = 1; i < argc; i++) {
		std::string a = argv[i];
		if ((a == "-c" || a == "--config") && i + 1 < argc) {
			configPath = argv[++i];
		} else if (a == "-n" || a == "--dry-run") {
			dryRun = true;
		} else if (a == "--fake-keys" && i + 1 < argc) {
			fake = true;
			fakeKeys = argv[++i];
		} else if (a == "-V" || a == "--version") {
			std::cout << FLIGHTSIM_VERSION << std::endl;
			return 0;
		} else if (a == "-h" || a == "--help") {
			usage(argv[0]);
			return 0;
		} else {
			usage(argv[0]);
			return 2;
		}
	}

	struct sigaction sa;
	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = onSignal;
	sigaction(SIGINT, &sa, nullptr);
	sigaction(SIGTERM, &sa, nullptr);

	try {
		Config cfg = loadConfig(configPath);
		std::cout << "flight-simulator-keys " << FLIGHTSIM_VERSION
				<< ": loaded " << cfg.keys.size() << " key bindings from "
				<< configPath << " (" << cfg.rows.size() << " rows x "
				<< cfg.columns.size() << " columns)" << std::endl;

		std::unique_ptr<GpioMatrix> matrix;
		std::unique_ptr<FakeKeyScript> script;
		auto start = steady_clock::now();
		if (fake) {
			auto * fm = new FakeGpioMatrix(cfg.rows.size(), cfg.columns.size());
			matrix.reset(fm);
			script.reset(new FakeKeyScript(*fm, parseFakeKeys(fakeKeys), start));
		} else {
			matrix.reset(new ChardevGpioMatrix(cfg.chip, cfg.columns, cfg.rows));
		}

		std::unique_ptr<XPlaneLink> link;
		if (!dryRun) {
			link.reset(new XPlaneLink(cfg.host, cfg.port));
			std::cout << (cfg.host.empty() ?
					"Listening for the X-Plane beacon ..." :
					"Using X-Plane at " + cfg.host) << std::endl;
		}

		KeypadScanner scanner(*matrix, milliseconds(cfg.debounceMs));
		auto nextRefresh = start;

		std::cout << "Scanning keypad. Press Control-C to stop." << std::endl;

		while (!stopRequested) {
			auto now = steady_clock::now();

			if (script && !script->advance(now)) {
				break;
			}

			if (link && now >= nextRefresh) {
				std::string change;
				link->refresh(change);
				if (!change.empty()) {
					std::cout << change << std::endl;
				}
				nextRefresh = now + seconds(1);
			}

			for (const KeyEvent & ev : scanner.poll(now)) {
				if (ev.type == KeyEvent::Type::MultipleIgnored) {
					std::cout << "Multiple keys pressed; ignoring." << std::endl;
					continue;
				}
				bool press = ev.type == KeyEvent::Type::Press;
				std::cout << (press ? "KEY PRESS   " : "KEY RELEASE ") << "row="
						<< ev.row << " col=" << ev.col;
				if (press) {
					auto it = cfg.keys.find( { ev.row, ev.col });
					if (it == cfg.keys.end()) {
						std::cout << "  (no binding)";
					} else if (dryRun) {
						std::cout << "  would send " << it->second;
					} else if (link->connected()) {
						link->sendCommand(it->second);
						std::cout << "  sent " << it->second;
					} else {
						std::cout << "  X-Plane not found; dropped " << it->second;
					}
				}
				std::cout << std::endl;
			}

			std::this_thread::sleep_for(milliseconds(1));
		}

		std::cout << "Stopped." << std::endl;
		return 0;

	} catch (const std::exception & e) {
		std::cerr << "error: " << e.what() << std::endl;
		return 1;
	}
}
