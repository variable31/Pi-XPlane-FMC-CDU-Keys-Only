/*
 * test_scanner.cpp - debounce and chord handling
 *
 * Copyright (c) 2026 Pi-XPlane-FMC-CDU-Keys-Only contributors. GPL-3.0-or-later.
 */

#include "check.h"
#include "gpio_fake.h"
#include "keypad_scanner.h"

using namespace flightsim;
using namespace std::chrono;
using Ev = KeyEvent;
using T = KeyEvent::Type;

namespace {

// Polls every 1 ms from `from` to `to` (exclusive), collecting events.
std::vector<Ev> run(KeypadScanner & s, KeypadScanner::Clock::time_point t0,
		int fromMs, int toMs) {
	std::vector<Ev> all;
	for (int ms = fromMs; ms < toMs; ms++) {
		for (const Ev & e : s.poll(t0 + milliseconds(ms))) {
			all.push_back(e);
		}
	}
	return all;
}

}

int main() {
	const auto t0 = KeypadScanner::Clock::time_point();

	// Clean press and release: each reported once, after the debounce time.
	{
		FakeGpioMatrix m(8, 9);
		KeypadScanner s(m, milliseconds(20));
		CHECK(run(s, t0, 0, 50).empty());

		m.press(3, 7);
		CHECK(run(s, t0, 50, 69).empty());		// not stable for 20 ms yet
		auto ev = run(s, t0, 69, 100);
		CHECK(ev.size() == 1 && ev[0] == (Ev { T::Press, 3, 7 }));

		CHECK(run(s, t0, 100, 500).empty());	// holding produces nothing

		m.release(3, 7);
		ev = run(s, t0, 500, 530);
		CHECK(ev.size() == 1 && ev[0] == (Ev { T::Release, 3, 7 }));
	}

	// Contact bounce on press and on release produces exactly one of each.
	{
		FakeGpioMatrix m(8, 9);
		KeypadScanner s(m, milliseconds(20));
		std::vector<Ev> ev;
		for (int ms = 0; ms < 30; ms++) {
			if ((ms / 3) % 2 == 0) {
				m.press(1, 1);
			} else {
				m.release(1, 1);
			}
			for (auto & e : s.poll(t0 + milliseconds(ms))) {
				ev.push_back(e);
			}
		}
		CHECK(ev.empty());
		m.press(1, 1);
		ev = run(s, t0, 30, 100);
		CHECK(ev.size() == 1 && ev[0] == (Ev { T::Press, 1, 1 }));

		for (int ms = 100; ms < 130; ms++) {
			if ((ms / 4) % 2 == 0) {
				m.release(1, 1);
			} else {
				m.press(1, 1);
			}
			for (auto & e : s.poll(t0 + milliseconds(ms))) {
				ev.push_back(e);
			}
		}
		CHECK(ev.size() == 1);	// still just the press
		m.release(1, 1);
		ev = run(s, t0, 130, 200);
		CHECK(ev.size() == 1 && ev[0] == (Ev { T::Release, 1, 1 }));
	}

	// A glitch shorter than the debounce time is ignored entirely.
	{
		FakeGpioMatrix m(8, 9);
		KeypadScanner s(m, milliseconds(20));
		m.press(2, 2);
		auto ev = run(s, t0, 0, 10);
		m.release(2, 2);
		auto ev2 = run(s, t0, 10, 100);
		CHECK(ev.empty() && ev2.empty());
	}

	// Chord: second key is ignored while both are down; rolling onto it
	// releases the first and presses the second.
	{
		FakeGpioMatrix m(8, 9);
		KeypadScanner s(m, milliseconds(20));
		m.press(1, 1);
		auto ev = run(s, t0, 0, 40);
		CHECK(ev.size() == 1 && ev[0] == (Ev { T::Press, 1, 1 }));

		m.press(4, 5);
		ev = run(s, t0, 40, 80);
		CHECK(ev.size() == 1 && ev[0].type == T::MultipleIgnored);

		m.release(1, 1);
		ev = run(s, t0, 80, 120);
		CHECK(ev.size() == 2);
		CHECK(ev.size() == 2 && ev[0] == (Ev { T::Release, 1, 1 }));
		CHECK(ev.size() == 2 && ev[1] == (Ev { T::Press, 4, 5 }));

		m.release(4, 5);
		ev = run(s, t0, 120, 160);
		CHECK(ev.size() == 1 && ev[0] == (Ev { T::Release, 4, 5 }));
	}

	// Chord that returns to the originally held key does not re-press it.
	{
		FakeGpioMatrix m(8, 9);
		KeypadScanner s(m, milliseconds(20));
		m.press(6, 2);
		run(s, t0, 0, 40);
		m.press(6, 3);			// same row, different column
		run(s, t0, 40, 80);
		m.release(6, 3);
		auto ev = run(s, t0, 80, 120);
		CHECK(ev.empty());
	}

	// The matrix is left with every column released after a scan.
	{
		FakeGpioMatrix m(2, 2);
		KeypadScanner s(m, milliseconds(20));
		m.press(1, 1);
		s.poll(t0);
		CHECK(m.readActiveRows() == 0);
	}

	TEST_MAIN_END();
}
