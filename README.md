# Pi-XPlane-FMC-CDU-Keys-Only (2026 revival)

This program turns buttons wired to a Raspberry Pi into X-Plane commands. It
started as the keypad for a home-built 737 FMC. The keymap is now a plain
config file, so any button box works: rows and columns of switches, with one
X-Plane command per button.

It revives Shahada Abubakar's
[Pi-XPlane-FMC-CDU-Keys-Only](https://github.com/dotsha747/Pi-XPlane-FMC-CDU-Keys-Only).
The original stopped installing when its author's apt repository went
offline and took its libraries with it. This version has **no dependencies
outside Raspberry Pi OS**.

> Looking for the Zibo 737 CDU with a screen? That is the separate
> [Pi-XPlane-FMC-CDU](https://github.com/dotsha747/Pi-XPlane-FMC-CDU)
> project. This program only sends key presses.

---

## Step-by-step guide (start here)

This guide takes you from a Raspberry Pi with buttons wired up to pressing
those buttons inside X-Plane. It takes about 20 minutes. Do the parts in
order. Each part ends with a check, so you know it worked before moving on.

When the guide says **type**, type the grey text exactly as shown and press
**Enter**. Commands that start with `sudo` may ask for the Pi's password.
That is the password you chose when you set up the Pi, and the screen shows
nothing while you type it.

### What you need

- A **Raspberry Pi 3** (a Pi 4 or 5 also works) running **Raspberry Pi OS**
  ("Bookworm" or newer). Any Pi OS installed from 2023 onwards is fine.
- The **button panel wired to the Pi's GPIO pins** (see [Wiring](#wiring)).
- The Pi and the **X-Plane 12 computer on the same home network**, either on
  the same Wi-Fi or plugged into the same router.
- A keyboard and screen on the Pi, or a remote login to it (SSH).
- If the Pi's memory card is new or needs wiping, set it up first with
  [Preparing a new memory card](#preparing-a-new-memory-card-optional).

### Part 1: Open a terminal on the Pi

1. On the Pi's desktop, click the **black screen icon** in the top bar, or
   press **Ctrl + Alt + T**.
2. A window with a text prompt opens. All the commands below are typed here.

**Using another computer instead (SSH)?** Open PowerShell (Windows) or
Terminal (Mac) and type `ssh yourname@yourpi.local`, using the Pi's user
name and host name. Before typing any command below, **check the prompt**.
It must show the Pi's name, for example `yourname@yourpi:~ $`. If it shows
`PS C:\...`, you are still typing on Windows, not on the Pi. The connection
also drops every time the Pi restarts, so run the `ssh` line again after a
reboot.

### Part 2: Download and install the program

1. Find out which version your Pi needs. Type:
   ```
   dpkg --print-architecture
   ```
   It prints either **`armhf`** (most Pi 3s) or **`arm64`**. Remember which.

2. Download the program. If step 1 said **armhf**, type:
   ```
   wget https://github.com/variable31/Pi-XPlane-FMC-CDU-Keys-Only/releases/latest/download/flight-simulator-keys_armhf.deb
   ```
   If it said **arm64**, type this instead:
   ```
   wget https://github.com/variable31/Pi-XPlane-FMC-CDU-Keys-Only/releases/latest/download/flight-simulator-keys_arm64.deb
   ```
   The download takes a few seconds. It ends with a line containing `saved`.

3. Install it (use `arm64` in place of `armhf` if that is what step 1 said):
   ```
   sudo apt install ./flight-simulator-keys_armhf.deb
   ```
   If asked `Do you want to continue? [Y/n]`, type **Y** and press Enter.
   A line starting with `Notice: Download is performed unsandboxed as root`
   is normal and not an error.

**Check:** type `flight-simulator-keys --version`. It should print a
version number such as `2.0.1`.

### Part 3: Free up the pins the buttons use

Two features of the Pi (the serial console and I2C) can hold some of the
pins the button panel uses. A freshly installed Pi OS usually has them off
already, but older setups may not. Switching them off is harmless either
way:

1. Type:
   ```
   sudo raspi-config
   ```
   A blue menu opens. Use the **arrow keys** to move, **Enter** to choose,
   and **Tab** to reach `<Finish>`.
2. Choose **Interface Options**, then **Serial Port**.
   - "Would you like a login shell to be accessible over serial?" Choose **No**.
   - "Would you like the serial port hardware to be enabled?" Choose **No**.
   - Press Enter on **OK**.
3. Choose **Interface Options** again, then **I2C**.
   - "Would you like the ARM I2C interface to be enabled?" Choose **No**.
   - Press Enter on **OK**.
4. Press **Tab** until `<Finish>` is highlighted, then press Enter. When
   asked to reboot, choose **Yes**. If it doesn't ask, type `sudo reboot`.

After the Pi restarts, open the terminal again (Part 1).

### Part 4: Test the buttons (X-Plane not needed yet)

The program starts automatically in the background. For this test, stop the
background copy first so the test can use the pins.

1. Type:
   ```
   sudo systemctl stop flight-simulator-keys
   ```
2. Start the button test:
   ```
   sudo flight-simulator-keys --dry-run
   ```
   You should see:
   ```
   flight-simulator-keys 2.0.1: loaded 69 key bindings from /etc/flight-simulator/keys.conf (8 rows x 9 columns)
   Scanning keypad. Press Control-C to stop.
   ```
3. **Press each button on the panel, one at a time.** Each press should
   print two lines, like this:
   ```
   KEY PRESS   row=7 col=3  would send sim/FMS/exec
   KEY RELEASE row=7 col=3
   ```
   The `KEY RELEASE` line should appear the moment you let go. If it comes
   late, or `Multiple keys pressed` appears when you press the next button,
   that button is **sticking** (see [Troubleshooting](#troubleshooting)).
   Check that the command matches the button's label. For example, the EXEC
   key should say `sim/FMS/exec`.
4. When you have tried every button, press **Ctrl + C** to stop the test.

**Check:** every button printed a `KEY PRESS` line with the right command.
If not, see [Troubleshooting](#troubleshooting) before going on.

### Part 5: Fly with it

1. On the flight simulator computer, start **X-Plane 12** and load a flight
   in the default **Boeing 737-800**. The default keymap drives X-Plane's
   built-in FMS.
2. On the Pi, start the program in the background again:
   ```
   sudo systemctl start flight-simulator-keys
   ```
3. Watch what it is doing:
   ```
   journalctl -u flight-simulator-keys -f
   ```
   Within about 10 seconds you should see a line like:
   ```
   found X-Plane "SIM-PC" at 192.168.1.20:49000
   ```
4. Press a button, such as the **INIT REF** or **LEGS** key. The log shows
   `sent sim/FMS/...`, and the FMC page changes in X-Plane.
5. Press **Ctrl + C** to stop watching the log. The program keeps running.

**Done.** From now on, the program starts by itself every time the Pi is
switched on. Just turn on the Pi and start X-Plane. You never need to run
these steps again.

If `found X-Plane` never appears, see the next section.

### If the Pi can't find X-Plane

Some routers and Wi-Fi networks block the signal X-Plane uses to announce
itself. In that case, tell the Pi the X-Plane computer's address directly:

1. **On the X-Plane computer** (Windows), press the **Windows key**, type
   `cmd`, and press Enter. In the black window, type `ipconfig` and press
   Enter. Look for **IPv4 Address**, for example `192.168.1.20`, and write it
   down.
2. **On the Pi**, open the settings file:
   ```
   sudo nano /etc/flight-simulator/keys.conf
   ```
3. Use the arrow keys to find the line `host =` under `[xplane]`. Add the
   address after it:
   ```
   host = 192.168.1.20
   ```
4. Save and close the file: press **Ctrl + O**, then **Enter**, then **Ctrl + X**.
5. Restart the program:
   ```
   sudo systemctl restart flight-simulator-keys
   ```
6. Watch the log again (Part 5, step 3). It should say `sending to 192.168.1.20`.

If buttons still do nothing in X-Plane, the Windows firewall may be blocking
the Pi. On the X-Plane computer, allow **X-Plane** through **Windows
Defender Firewall** (*Allow an app through firewall*) for **Private**
networks.

### Troubleshooting

| What you see | What it means | What to do |
|---|---|---|
| `wget: ... 404 Not Found` | No release has been published yet, or the name was mistyped | Check the spelling. Open the [Releases page](https://github.com/variable31/Pi-XPlane-FMC-CDU-Keys-Only/releases) in a browser to see what is available. |
| `Device or resource busy` | Something else is using the pins | Run `sudo systemctl stop flight-simulator-keys` and try again. If it persists, redo Part 3. |
| `Permission denied` | The command was run without `sudo` | Put `sudo ` in front of the command. |
| Pressing a button prints nothing | The button isn't connected | Check that button's two wires. See [Wiring](#wiring). |
| Wrong command for a button | The row/column wires are swapped, or the keymap differs from your panel | Fix the wiring, or change that button's line in `keys.conf` (see [Changing what the buttons do](#changing-what-the-buttons-do)). |
| `(no binding)` | That button has no command yet | Add a line for it in `keys.conf`. |
| `Multiple keys pressed; ignoring.` when pressing only one button | Another button is **stuck down** (often one on the same row or column), or two wires are touching | Press each nearby button once and watch for a late or missing `KEY RELEASE`; that one is sticking. With the Pi off, clean it (a drop of 90% isopropyl alcohol or contact cleaner, then press it 20–30 times) or replace the switch. |
| A button seems dead, but it works on a second try | A sticky or dirty switch | Clean or replace it as above. |
| One press registers twice | The switch bounces longer than the filter allows | In `keys.conf`, change `debounce_ms = 20` to `debounce_ms = 30`, then restart the program. |
| `Sudo is disabled on this machine` | You typed the command on Windows, not on the Pi | Connect to the Pi first (see Part 1). Don't change the Windows setting. |
| `ssh: ... Connection refused` | SSH is switched off on the Pi | See [Preparing a new memory card](#preparing-a-new-memory-card-optional), step 3. |
| `X-Plane not found; dropped ...` | The Pi can't see X-Plane | Make sure X-Plane is running, then follow [If the Pi can't find X-Plane](#if-the-pi-cant-find-x-plane). |

To see the last 50 lines of the program's log at any time, type:
`journalctl -u flight-simulator-keys -n 50`

### Preparing a new memory card (optional)

Only needed for a new card, or to start over. This **erases** the card.

1. On a Windows or Mac computer, install **Raspberry Pi Imager** from
   <https://www.raspberrypi.com/software/> and put the Pi's memory card
   into the computer.
2. In Imager, choose the **Raspberry Pi 3** (or your model), **Raspberry Pi
   OS**, and the memory card. Check the size so you don't pick a different
   drive.
3. Click **Next**, then **Edit settings**:
   - **General:** a host name (e.g. `flightsim`), a user name and
     password (write them down), your Wi-Fi, and your time zone.
   - **Services:** tick **Enable SSH** and choose **Use password
     authentication**. Missing this step is why SSH later says
     "Connection refused".
4. Click **Save**. When asked **"apply OS customisation settings?"**, click
   **Yes**, then **Yes** to erase the card.
5. If Windows offers to **format** the card afterwards, click **Cancel**.
6. Put the card in the Pi, power it on, and wait **5 minutes** before the
   first connection. The first start takes a while.

### Updating or removing

- **Update:** repeat Part 2. Your `keys.conf` changes are kept.
- **Remove:** type `sudo apt remove flight-simulator-keys`.

---

## Wiring

GPIO numbers are **BCM** numbers, not physical pin numbers. The default
`keys.conf` matches the original PCB:

| | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 |
|---|---|---|---|---|---|---|---|---|---|
| **Columns** (outputs) | 14 | 15 | 18 | 23 | 24 | 25 | 8 | 7 | 16 |
| **Rows** (inputs, pull-up) | 21 | 2 | 3 | 4 | 17 | 27 | 22 | 10 | |

- Each button connects one column wire to one row wire.
- For your own box, list whatever pins you used under `[gpio]`. The keys are
  numbered `row,col` in the order the pins are listed.
- Add a diode per switch if you want reliable detection with three or more
  keys held.

## Changing what the buttons do

Edit `/etc/flight-simulator/keys.conf` (`sudo nano /etc/flight-simulator/keys.conf`),
then run `sudo systemctl restart flight-simulator-keys`.

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

- **Finding command names:** in X-Plane, open *Settings > Keyboard* and
  search for a command; the name appears under it. Add-on aircraft publish
  their own commands. For example, the Zibo/LevelUp 737 uses
  `laminar/B738/button/fmc1_*`.
- **Checking your changes:** run `sudo flight-simulator-keys --dry-run`
  (stop the service first) to check the file. A mistake is reported with its
  line number.

## What changed from the 2019 version

| 2019 | Now |
|---|---|
| Keymap hard-coded in C++ | `/etc/flight-simulator/keys.conf`, editable without rebuilding |
| wiringPi (deprecated, not on Pi 5) | Linux GPIO character device (Pi 3, 4, 5) |
| Push-pull column drive | Open-drain, so two keys held down can't short two GPIOs |
| 5 ms sleep on press only | 20 ms stable-reading debounce on press *and* release |
| Finds X-Plane once; restart the Pi if X-Plane restarts | Follows the X-Plane beacon; reconnects after restarts and IP changes |
| No fallback when multicast is blocked | `host =` setting for a fixed X-Plane address |
| Raspbian Stretch `.deb` from a private apt repo | armhf/arm64 `.deb` on GitHub Releases, for Raspberry Pi OS Bookworm and newer |

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

**Releasing a new version:**
1. Bump `VERSION` in `CMakeLists.txt` and merge the change to `master`.
2. Create a matching tag, e.g. `v2.0.1`. Either push it with git, or on
   GitHub go to *Releases*, *Draft a new release*, type the tag, pick
   `master`, and click *Publish release*.

CI builds both packages and attaches them to the release. A tag that
doesn't match `VERSION` fails the build. The release also
carries version-less copies (`flight-simulator-keys_armhf.deb`), so the
download links in the guide above always fetch the newest version.

## License

GPL-3.0-or-later (see `LICENSE.txt`).
- Original program: copyright (C) 2019 Shahada Abubakar.
- The vendored libXPlane-UDP-Client is LGPL-3.0-or-later; see
  `third_party/libXPlane-UDP-Client/VENDORED.md`.
