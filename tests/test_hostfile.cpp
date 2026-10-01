/*
 * test_hostfile.cpp - X-Plane address file used by the CDU display
 *
 * Copyright (c) 2026 Pi-XPlane-FMC-CDU-Keys-Only contributors. GPL-3.0-or-later.
 */

#include "check.h"
#include "host_file.h"

#include <cstdlib>
#include <fstream>
#include <sys/stat.h>
#include <unistd.h>

using namespace flightsim;

namespace {

bool exists(const std::string & p) {
	struct stat st;
	return stat(p.c_str(), &st) == 0;
}

std::string slurp(const std::string & p) {
	std::ifstream in(p);
	std::string s;
	std::getline(in, s);
	return s;
}

}

int main() {
	char tmpl[] = "/tmp/fsk-hostfile-XXXXXX";
	std::string dir = mkdtemp(tmpl);
	std::string path = dir + "/xplane-host";

	// A stale file from an earlier run is removed on start.
	{
		std::ofstream(path) << "10.9.9.9\n";
		HostFile hf(path);
		CHECK(!exists(path));
	}

	{
		HostFile hf(path);
		hf.set("192.168.1.20");
		CHECK(slurp(path) == "192.168.1.20");
		CHECK(!exists(path + ".tmp"));

		hf.set("192.168.1.21");			// X-Plane moved
		CHECK(slurp(path) == "192.168.1.21");

		hf.set("");						// X-Plane lost
		CHECK(!exists(path));

		hf.set("192.168.1.22");
		CHECK(slurp(path) == "192.168.1.22");
	}
	CHECK(!exists(path));				// removed on shutdown

	// An unwritable location only warns; it must never throw.
	{
		HostFile hf("/nonexistent-dir/xplane-host");
		hf.set("1.2.3.4");
		hf.set("1.2.3.5");
	}

	// Empty path disables the feature entirely.
	{
		HostFile hf("");
		hf.set("1.2.3.4");
	}

	rmdir(dir.c_str());
	TEST_MAIN_END();
}
