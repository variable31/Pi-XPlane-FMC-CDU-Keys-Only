/*
 * test_udp.cpp - X-Plane wire protocol: CMND packets and BECN discovery,
 * against local sockets standing in for X-Plane.
 *
 * Copyright (c) 2026 Pi-XPlane-FMC-CDU-Keys-Only contributors. GPL-3.0-or-later.
 */

#include "check.h"
#include "xplane_link.h"

#include <arpa/inet.h>
#include <chrono>
#include <cstring>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>

using namespace flightsim;

namespace {

// A UDP socket on 127.0.0.1 with an ephemeral port; plays X-Plane.
struct FakeXPlane {
	int fd;
	uint16_t port;

	FakeXPlane() {
		fd = socket(AF_INET, SOCK_DGRAM, 0);
		struct sockaddr_in a;
		memset(&a, 0, sizeof(a));
		a.sin_family = AF_INET;
		a.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
		bind(fd, (struct sockaddr *) &a, sizeof(a));
		socklen_t len = sizeof(a);
		getsockname(fd, (struct sockaddr *) &a, &len);
		port = ntohs(a.sin_port);
		struct timeval tv = { 3, 0 };
		setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
	}
	~FakeXPlane() {
		close(fd);
	}

	std::string receive() {
		char buf[2048];
		ssize_t n = recv(fd, buf, sizeof(buf), 0);
		return n > 0 ? std::string(buf, (size_t) n) : std::string();
	}
};

// Builds a BECN datagram per X-Plane's "Exchanging Data" spec (v1.1).
std::string beacon(int32_t role, uint16_t port, const std::string & name) {
	std::string p("BECN\0", 5);
	p += (char) 1;			// beacon_major_version
	p += (char) 1;			// beacon_minor_version
	int32_t hostId = 1;		// X-Plane
	int32_t version = 121400;
	p.append((const char *) &hostId, 4);
	p.append((const char *) &version, 4);
	p.append((const char *) &role, 4);
	p.append((const char *) &port, 2);
	p += name;
	p += '\0';
	return p;
}

void sendTo(const std::string & payload, uint16_t port) {
	int fd = socket(AF_INET, SOCK_DGRAM, 0);
	struct sockaddr_in a;
	memset(&a, 0, sizeof(a));
	a.sin_family = AF_INET;
	a.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	a.sin_port = htons(port);
	sendto(fd, payload.data(), payload.size(), 0, (struct sockaddr *) &a,
			sizeof(a));
	close(fd);
}

}

int main() {
	// resolveIPv4 accepts both hostnames and dotted quads.
	CHECK(resolveIPv4("127.0.0.1") == "127.0.0.1");
	CHECK(resolveIPv4("localhost") == "127.0.0.1");
	CHECK_THROWS(resolveIPv4("no-such-host.invalid"), "cannot resolve");

	// Static host: CMND packet is "CMND\0" + command + NUL.
	{
		FakeXPlane xp;
		XPlaneLink link("localhost", xp.port);
		std::string change;
		CHECK(link.refresh(change));
		CHECK(change.find("127.0.0.1:" + std::to_string(xp.port)) != std::string::npos);
		CHECK(link.connected());

		link.sendCommand("sim/FMS/exec");
		std::string got = xp.receive();
		CHECK(got == std::string("CMND\0sim/FMS/exec\0", 18));

		CHECK(link.refresh(change) && change.empty());	// no change reported twice
	}

	// Beacon discovery: a BECN from the "master" wins over other roles, and
	// commands go to the port the beacon advertised.
	{
		FakeXPlane xp;
		XPlaneLink link("", 0);
		std::string change;
		CHECK(!link.refresh(change));	// nothing heard yet (starts listener)

		bool found = false;
		auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(15);
		while (!found && std::chrono::steady_clock::now() < deadline) {
			sendTo(beacon(2, 1, "VisualSlave"), 49707);
			sendTo(beacon(1, xp.port, "TestSim"), 49707);
			std::this_thread::sleep_for(std::chrono::milliseconds(300));
			found = link.refresh(change);
		}
		CHECK(found);
		CHECK(link.target().ip == "127.0.0.1");
		CHECK(link.target().port == xp.port);
		CHECK(link.target().name == "TestSim");
		CHECK(change.find("TestSim") != std::string::npos);

		link.sendCommand("sim/FMS/key_A");
		CHECK(xp.receive() == std::string("CMND\0sim/FMS/key_A\0", 19));
	}

	TEST_MAIN_END();
}
