cmake_minimum_required(VERSION 3.16)

project(flight-simulator-keys
	VERSION 2.2.0
	DESCRIPTION "Raspberry Pi GPIO key matrix to X-Plane command bridge"
	LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE RelWithDebInfo)
endif()

option(FLIGHTSIM_WERROR "Treat warnings in project code as errors" OFF)
option(FLIGHTSIM_BUILD_TESTS "Build unit tests" ON)

find_package(Threads REQUIRED)
include(GNUInstallDirs)

# --- vendored libXPlane-UDP-Client (LGPL-3, unmodified; see third_party/) ---
add_library(xplaneudp STATIC
	third_party/libXPlane-UDP-Client/XPlaneBeaconListener.cpp
	third_party/libXPlane-UDP-Client/XPlaneUDPClient.cpp
	third_party/libXPlane-UDP-Client/XPUtils.cpp)
target_include_directories(xplaneudp SYSTEM PUBLIC third_party/libXPlane-UDP-Client)
target_link_libraries(xplaneudp PUBLIC Threads::Threads)
# Upstream code predates modern warnings; keep it quiet and untouched.
target_compile_options(xplaneudp PRIVATE -w)

# --- project code ---
set(FLIGHTSIM_WARNINGS -Wall -Wextra)
if(FLIGHTSIM_WERROR)
	list(APPEND FLIGHTSIM_WARNINGS -Werror)
endif()

add_library(flightsim_core STATIC
	src/config.cpp
	src/gpio_chardev.cpp
	src/host_file.cpp
	src/keypad_scanner.cpp
	src/xplane_link.cpp)
target_include_directories(flightsim_core PUBLIC src)
target_link_libraries(flightsim_core PUBLIC xplaneudp)
target_compile_options(flightsim_core PRIVATE ${FLIGHTSIM_WARNINGS})

add_executable(flight-simulator-keys src/main.cpp)
target_link_libraries(flight-simulator-keys PRIVATE flightsim_core)
target_compile_options(flight-simulator-keys PRIVATE ${FLIGHTSIM_WARNINGS})
target_compile_definitions(flight-simulator-keys PRIVATE
	FLIGHTSIM_VERSION="${PROJECT_VERSION}"
	FLIGHTSIM_DEFAULT_CONFIG="/etc/flight-simulator/keys.conf")
# Only libc is needed on the Pi; no libstdc++ version coupling.
target_link_options(flight-simulator-keys PRIVATE -static-libstdc++ -static-libgcc)

# --- tests ---
if(FLIGHTSIM_BUILD_TESTS)
	enable_testing()
	foreach(t config scanner udp hostfile)
		add_executable(test_${t} tests/test_${t}.cpp)
		target_link_libraries(test_${t} PRIVATE flightsim_core)
		target_compile_options(test_${t} PRIVATE ${FLIGHTSIM_WARNINGS})
		target_compile_definitions(test_${t} PRIVATE
			SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
		add_test(NAME ${t} COMMAND test_${t})
	endforeach()
	set_tests_properties(udp PROPERTIES TIMEOUT 30)

	# CDU display launcher state machine (needs python3 + curl).
	find_program(FLIGHTSIM_PYTHON3 python3)
	find_program(FLIGHTSIM_CURL curl)
	if(FLIGHTSIM_PYTHON3 AND FLIGHTSIM_CURL)
		add_test(NAME display_launch COMMAND sh
			${CMAKE_CURRENT_SOURCE_DIR}/tests/test_display_launch.sh
			${CMAKE_CURRENT_SOURCE_DIR})
		set_tests_properties(display_launch PROPERTIES TIMEOUT 60)
	endif()

	# End-to-end: shipped config + fake keypad, no GPIO or X-Plane needed.
	add_test(NAME dry_run_e2e COMMAND flight-simulator-keys
		--config ${CMAKE_CURRENT_SOURCE_DIR}/config/keys.conf
		--dry-run --fake-keys "1,1 7,3 5,9 8,9")
	set_tests_properties(dry_run_e2e PROPERTIES
		PASS_REGULAR_EXPRESSION "row=1 col=1  would send laminar/B738/button/fmc1_1L.*row=7 col=3  would send laminar/B738/button/fmc1_exec.*row=5 col=9  would send laminar/B738/button/fmc1_3.*row=8 col=9  \\(no binding\\)")
endif()

# --- install / package ---
install(TARGETS flight-simulator-keys RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
install(FILES config/keys.conf config/keys-xplane-default.conf
	DESTINATION /etc/flight-simulator)
# Pristine copy so users can switch back after trying another keymap.
install(FILES config/keys.conf DESTINATION /etc/flight-simulator RENAME keys-zibo.conf)
install(FILES packaging/flight-simulator-keys.service
	DESTINATION /lib/systemd/system)
install(FILES README.md LICENSE.txt DESTINATION ${CMAKE_INSTALL_DOCDIR})

file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/conffiles
	"/etc/flight-simulator/keys.conf\n/etc/flight-simulator/keys-zibo.conf\n/etc/flight-simulator/keys-xplane-default.conf\n")

set(CPACK_GENERATOR DEB)
set(CPACK_PACKAGE_NAME flight-simulator-keys)
set(CPACK_PACKAGE_CONTACT "variable31 <https://github.com/variable31/pi-xplane-fmc-cdu-keys-only>")
set(CPACK_DEBIAN_PACKAGE_HOMEPAGE "https://github.com/variable31/pi-xplane-fmc-cdu-keys-only")
set(CPACK_DEBIAN_PACKAGE_SECTION misc)
# Release builds run in debian:bookworm so the binary needs glibc <= 2.36
# (CI enforces this with objdump); it then runs on Bookworm and Trixie.
set(FLIGHTSIM_MIN_GLIBC "2.36" CACHE STRING "glibc version recorded in the .deb Depends")
set(CPACK_DEBIAN_PACKAGE_DEPENDS "libc6 (>= ${FLIGHTSIM_MIN_GLIBC}), adduser")
set(CPACK_STRIP_FILES ON)
set(CPACK_DEBIAN_FILE_NAME DEB-DEFAULT)
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "${PROJECT_DESCRIPTION}")
set(CPACK_PACKAGE_DESCRIPTION
	"Scans a key matrix on the Raspberry Pi GPIO header and sends a configurable\nX-Plane command for each key. Revival of Pi-XPlane-FMC-CDU-Keys-Only by\nShahada Abubakar.")
if(NOT CPACK_DEBIAN_PACKAGE_ARCHITECTURE)
	# Set explicitly when cross-compiling, e.g. -DCPACK_DEBIAN_PACKAGE_ARCHITECTURE=armhf
	execute_process(COMMAND dpkg --print-architecture
		OUTPUT_VARIABLE CPACK_DEBIAN_PACKAGE_ARCHITECTURE
		OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET)
endif()
set(CPACK_DEBIAN_PACKAGE_CONTROL_EXTRA
	${CMAKE_CURRENT_BINARY_DIR}/conffiles
	${CMAKE_CURRENT_SOURCE_DIR}/packaging/debian/postinst
	${CMAKE_CURRENT_SOURCE_DIR}/packaging/debian/prerm)
set(CPACK_DEBIAN_PACKAGE_CONTROL_STRICT_PERMISSION ON)
include(CPack)
