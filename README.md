# Pi-XPlane-FMC-CDU-Keys-Only (2026 revival)

Turns buttons wired to a Raspberry Pi into X-Plane commands. It started as
the keypad for a home-built 737 FMC. The keymap is now a plain config
file, so any button box works: rows and columns of switches, one X-Plane
command per button.

This is a revival of Shahada Abubakar's
[Pi-XPlane-FMC-CDU-Keys-Only](https://github.com/dotsha747/Pi-XPlane-FMC-CDU-Keys-Only).
The original stopped installing when its author's apt repository
(`repo.shahada.abubakar.net`) went offline, taking its libraries with it.

The rebuild has **no dependencies outside the Raspberry Pi OS base system**:
- X-Plane's UDP library is vendored in `third_party/`.
- GPIO goes straight through the kernel, with no wiringPi or libgpiod.

> Looking for the Zibo 737 CDU with a screen? That is the separate
> [Pi-XPlane-FMC-CDU](https://github.com/dotsha747/Pi-XPlane-FMC-CDU)
> project. This program only sends key presses.

## What changed from the 2019 version

| 2019 | Now |
|---|---|
| Keymap hard-coded in C++ | `/etc/flight-simulator/keys.conf`, editable without rebuilding |
| wiringPi (deprecated, not on Pi 5) | Linux GPIO character device (works on Pi 3, 4, 5) |
| Push-pull column drive | Open-drain: pressing two keys can't short two GPIOs |
| 5 ms sleep on press only | 20 ms stable-reading debounce on press *and* release |
| Finds X-Plane once; restart the Pi if X-Plane restarts | Follows the X-Plane beacon, and reconnects after X-Plane restarts or changes IP |
| No fallback when multicast is blocked | `host =` setting for a fixed X-Plane address |
| Raspbian Stretch `.deb` from a private apt repo | `.deb` for armhf/arm64 on GitHub Releases, for Raspberry Pi OS Bookworm and newer |

## Install (Raspberry Pi OS Bookworm or newer)

1. Pick the package for your OS from
   [Releases](https://github.com/variable31/pi-xplane-fmc-cdu-keys-only/releases):
   - 32-bit OS: `armhf`
   - 64-bit OS: `arm64`

   To check which you have, run `dpkg --print-architecture`.
2. Install it:
   ```sh
   sudo apt install ./flight-simulator-keys_*_armhf.deb
   ```
   The service starts on boot. Watch it with:
   ```sh
   journalctl -u flight-simulator-keys -f
   ```
3. **Free the pins used by the default wiring:**
   - GPIO 14/15 are the serial console.
   - GPIO 2/3 are I2C.

   To free them, run `sudo raspi-config`, open *Interface Options*, then:
   - *Serial Port*: login shell **No**, hardware **No**
   - *I2C*: **No**

   Then reboot. If you skip this, the log says which pin is busy.

## Wiring

GPIO numbers are **BCM** numbers, not physical pin numbers. The default
`keys.conf` matches the original PCB:

| | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 |
|---|---|---|---|---|---|---|---|---|---|
| **Columns** (outputs) | 14 | 15 | 18 | 23 | 24 | 25 | 8 | 7 | 16 |
| **Rows** (inputs, pull-up) | 21 | 2 | 3 | 4 | 17 | 27 | 22 | 10 | |

- Each button connects one column wire to one row wire.
- For your own box, list whatever pins you used under `[gpio]`.
- Number the keys `row,col` in the order the pins are listed.
- Add a diode per switch if you want reliable detection with three or more keys held.

## Configure

Edit `/etc/flight-simulator/keys.conf`, then restart the service:
```sh
sudo systemctl restart flight-simulator-keys
```

```ini
[gpio]
columns = 14 15 18
rows    = 21 2
[xplane]
host =                 ; empty: find X-Plane automatically
[keys]
1,1 = sim/lights/landing_lights_toggle
1,2 = sim/autopilot/heading
2,3 = sim/FMS/exec
```

- **Commands:** find command names in X-Plane under *Settings > Keyboard*
  (hover a command), or in DataRefTool. Add-on aircraft publish their own.
  For example, the Zibo/LevelUp 737 uses `laminar/B738/button/fmc1_*`.
- **Upgrades:** package upgrades keep your edited `keys.conf` (it's a Debian conffile).

### Test the wiring without X-Plane

```sh
sudo systemctl stop flight-simulator-keys
flight-simulator-keys --dry-run
```

Each button should print `KEY PRESS row=R col=C would send <command>`.

## X-Plane 12 setup

- **Automatic discovery:** by default the Pi listens for X-Plane's network
  beacon (UDP multicast 239.255.1.1:49707). X-Plane sends it when
  networking is on.
- **If the Pi never logs `found X-Plane`:** usually Wi-Fi or a router is
  dropping multicast. Set `host =` to the X-Plane PC's IP, with
  `port = 49000`, which is X-Plane's default UDP receive port.
- **Firewall:** allow inbound UDP 49000 to X-Plane on the sim PC.

## Build from source

This works on the Pi itself or on any Linux PC:

```sh
sudo apt install cmake g++
cmake -S . -B build && cmake --build build
ctest --test-dir build          # unit tests + end-to-end dry run
sudo cmake --install build
```

To build release packages exactly as CI does:
```sh
docker run --rm -v "$PWD":/src -w /src debian:bookworm scripts/build-deb.sh armhf
```

To release a new version:
1. Bump `VERSION` in `CMakeLists.txt`.
2. Push a matching tag, e.g. `v2.0.1`.

CI builds both packages and attaches them to a GitHub Release.

## License

GPL-3.0-or-later (see `LICENSE.txt`).
- Original program copyright (C) 2019 Shahada Abubakar.
- The vendored libXPlane-UDP-Client is LGPL-3.0-or-later; see
  `third_party/libXPlane-UDP-Client/VENDORED.md`.
