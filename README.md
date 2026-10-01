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

**Ready for the Zibo / LevelUp 737-800** (captain's CDU) out of the box.
A keymap for X-Plane's default 737 is included too; see
[Using X-Plane's default 737 instead](#using-x-planes-default-737-instead).

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
version number such as `2.2.0`.

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
   flight-simulator-keys 2.2.0: loaded 69 key bindings from /etc/flight-simulator/keys.conf (8 rows x 9 columns)
   Scanning keypad. Press Control-C to stop.
   ```
3. **Press each button on the panel, one at a time.** Each press should
   print two lines, like this:
   ```
   KEY PRESS   row=7 col=3  would send laminar/B738/button/fmc1_exec
   KEY RELEASE row=7 col=3
   ```
   The `KEY RELEASE` line should appear the moment you let go. If it comes
   late, or `Multiple keys pressed` appears when you press the next button,
   that button is **sticking** (see [Troubleshooting](#troubleshooting)).
   Check that the command matches the button's label. For example, the EXEC
   key should say `laminar/B738/button/fmc1_exec`.
4. When you have tried every button, press **Ctrl + C** to stop the test.

**Check:** every button printed a `KEY PRESS` line with the right command.
If not, see [Troubleshooting](#troubleshooting) before going on.

### Part 5: Fly with it

1. On the flight simulator computer, start **X-Plane 12** and load a flight
   in the **Zibo 737-800**. Use the captain's (left) CDU in the cockpit.
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
   `sent laminar/B738/button/fmc1_...`, and the FMC page changes in X-Plane.
5. Press **Ctrl + C** to stop watching the log. The program keeps running.

**Done.** From now on, the program starts by itself every time the Pi is
switched on. Just turn on the Pi and start X-Plane. You never need to run
these steps again.

If `found X-Plane` never appears, see the next section.

### Part 6 (optional): Show the CDU screen on the Pi's display

With a screen plugged into the Pi's HDMI port, the Pi can show the live
CDU screen next to the buttons. The screen comes from **WebFMC**, an
X-Plane plugin that publishes the CDU on your home network. The Pi
switches to it automatically whenever X-Plane is running, and shows
"WAITING FOR X-PLANE..." when it isn't.

The Pi and the X-Plane computer must be on the **same home network**
(the same Wi-Fi or router). Nothing else connects them.

**On the X-Plane computer (once, about 30 minutes):**

You need the X-Plane computer, a phone on the same Wi-Fi, and the Pi.

1. **Update X-Plane 12.** Run the **X-Plane 12 Installer** and choose
   **Update X-Plane**. Let it finish.
2. **Install the Zibo 737-800 for X-Plane 12.** The X-Plane 11 copy does
   not work in X-Plane 12, so download the X-Plane 12 version and unzip it
   into `X-Plane 12\Aircraft\`. Start X-Plane, check that the Zibo loads
   and flies, then quit X-Plane.
3. **Install WebFMC (free) for X-Plane 12** by Green Arc Studios. The
   **free** version supports the Zibo 737, so there's nothing to buy.
   Download it from [WebFMC on X-Plane.org](https://forums.x-plane.org/files/file/43314-webfmc/)
   (a free X-Plane.org account is needed). Pick the **X-Plane 12**
   version, unzip it and follow the instructions that come with it; it
   normally goes into `X-Plane 12\Resources\plugins\`.
   *WebFMC Pro* (paid) only adds other add-on aircraft such as ToLiss or
   FlightFactor. Either version works with this program.
4. **Make sure Windows treats your home network as Private.** If it is set
   to *Public*, Windows blocks the phone and the Pi. Click **Start** >
   **Settings** > **Network & internet**, click **Wi-Fi** or **Ethernet**
   (whichever this computer uses), click your network, and under
   **Network profile type** choose **Private network**.
5. **Start X-Plane 12 and load a flight in the Zibo 737,** in the
   captain's (left) seat. If a **Windows Security** box asks about
   X-Plane, tick **Private networks** and click **Allow**. In X-Plane's top
   menu, open **Plugins** and check that **WebFMC** is listed.
6. **Find this computer's address.** Click **Start**, type `cmd`, press
   **Enter**, then type `ipconfig` and press **Enter**. Write down the
   **IPv4 Address**, for example `192.168.1.20`.
7. **Test it from a phone** on the same Wi-Fi. Open the browser and go to
   `http://` + that address + `:9090`, for example
   `http://192.168.1.20:9090`. You should see the 737's CDU. Press LEGS on
   the phone and the CDU in X-Plane changes too.
   **The CDU must appear on the phone before you continue.** If it doesn't,
   the Pi won't be able to show it either; check steps 3 to 5 again.

**On the Pi:**

8. **Turn off the Pi's desktop.** The CDU needs the whole screen, and
   while the Raspberry Pi desktop is running it keeps the screen for
   itself. (A Pi set up with *Raspberry Pi OS Lite* has no desktop, so this
   changes nothing there.) Type:
   ```
   sudo systemctl set-default multi-user.target
   sudo reboot
   ```
   After the restart, the Pi's screen shows a text login prompt instead of
   the desktop. That's expected. Reconnect over SSH to continue. To get the
   desktop back later: `sudo systemctl set-default graphical.target`, then
   `sudo reboot`.
9. Download and install the display program. Use the same terminal as
   Part 2 (SSH is fine). This also installs a web browser, so it takes
   several minutes on a Pi 3:
   ```
   wget https://github.com/variable31/Pi-XPlane-FMC-CDU-Keys-Only/releases/latest/download/flight-simulator-display_all.deb
   sudo apt install ./flight-simulator-display_all.deb
   ```
10. Look at the Pi's screen. It shows **WAITING FOR X-PLANE...** in green.
    Once X-Plane and WebFMC are running, it switches to the CDU by itself,
    usually within 10 seconds of the buttons finding X-Plane. It shows only
    the CDU **screen**, without WebFMC's on-screen keys, because the panel
    has real buttons. Press **LEGS** on the panel and the page changes on the
    Pi's screen and in X-Plane.

**Every flight from now on:** turn on the X-Plane computer, start X-Plane
with the Zibo, and turn on the Pi. Nothing else is needed.

**Good to know:**

- The Pi's own screen now always shows the CDU, so it no longer shows a
  login prompt. Use SSH from another computer to type commands.
- To change the WebFMC port, set a fixed X-Plane address, or make the text
  bigger, edit `/etc/flight-simulator/display.conf`, then type
  `sudo systemctl restart flight-simulator-display`.
- To show WebFMC's on-screen keys as well, set `screen_only = no` in that
  file. To show the first officer's (right) CDU, set `side = 1` (and use
  the `fmc2_` keymap, see [Changing what the buttons do](#changing-what-the-buttons-do)).
- To remove it, type `sudo apt remove flight-simulator-display`. The
  buttons keep working.

**CDU screen doesn't fill the display?** WebFMC keeps the CDU's shape, so
it leaves black bars on a screen with a different shape. To stretch it to
fill the whole display (once; the Pi remembers it):
1. Plug a USB mouse into the Pi.
2. On the Pi's screen, click WebFMC's **settings** button (the gear).
3. Turn **Keep Aspect Ratio** off, then close the settings.
4. Unplug the mouse.

If the text is still too small or too large, set
`chromium_flags = --force-device-scale-factor=1.2` (try 0.8 to 1.5) in
`/etc/flight-simulator/display.conf`, then type
`sudo systemctl restart flight-simulator-display`.

**Screen blank, "no signal", or the wrong size?** HDMI-to-VGA adapters
often don't tell the Pi which resolutions the screen supports. Set one by
hand:
1. Type `sudo nano /boot/firmware/cmdline.txt`. The file is a single long
   line.
2. Press **End** to go to the end of the line, then type a **space** and
   `video=HDMI-A-1:1024x768@60`. Use your screen's resolution; 1024x768
   and 800x600 suit most small VGA screens.
3. Save with **Ctrl + O**, then **Enter**, then exit with **Ctrl + X**.
4. Type `sudo reboot`.

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
| The log says `sent ...` but nothing happens in X-Plane | The keymap doesn't match the aircraft | The standard keymap is for the Zibo 737. For X-Plane's default 737, see [Using X-Plane's default 737 instead](#using-x-planes-default-737-instead). For any other aircraft, the commands need changing in `keys.conf`. |
| The phone test (Part 6 step 7) shows nothing | Windows is blocking it, or WebFMC isn't loaded | Check the network is **Private** (Part 6 step 4), allow X-Plane in the Windows Security box, and check **WebFMC** is listed under X-Plane's **Plugins** menu. Use the address from `ipconfig` and add `:9090`. |
| The Pi's screen stays on **WAITING FOR X-PLANE...** | WebFMC can't be reached from the Pi | Do the phone test in Part 6 step 7 first. If the phone works, type `journalctl -t display-launch -n 30 --no-pager` and check the address it tries. If the buttons haven't found X-Plane yet, the screen can't either; set `host =` in `/etc/flight-simulator/display.conf`. |
| The Pi's screen shows the **Raspberry Pi desktop** instead of the CDU | The desktop is keeping the screen | Turn off the desktop (Part 6 step 8): `sudo systemctl set-default multi-user.target`, then `sudo reboot`. |
| The Pi's screen shows WebFMC's **on-screen keys** | Older display program, or `screen_only` is off | Update the display program (repeat Part 6 step 9) to version 2.2.0 or later. Check `screen_only = yes` in `/etc/flight-simulator/display.conf`, then type `sudo systemctl restart flight-simulator-display`. |
| The CDU screen has black bars or is small | WebFMC keeps the CDU's shape | See "CDU screen doesn't fill the display" in Part 6. |
| Pi screen blank or "no signal" | The HDMI-to-VGA adapter needs a fixed resolution | See "Screen blank" at the end of Part 6. |
| CDU text too small or too large | Screen size | In `/etc/flight-simulator/display.conf`, set `chromium_flags = --force-device-scale-factor=1.5` (or 0.8), then restart the display. |
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

- **Update:** repeat Part 2, and Part 6 step 9 if you use the screen.
  Your changes to `keys.conf` and `display.conf` are kept.
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

### Using X-Plane's default 737 instead

The standard keymap is for the **Zibo 737**. For X-Plane's own default
737-800, switch to the included default keymap by typing:
```
sudo cp /etc/flight-simulator/keys-xplane-default.conf /etc/flight-simulator/keys.conf
sudo systemctl restart flight-simulator-keys
```
The key positions are the same, so the wiring doesn't change. To go back
to the Zibo keymap, type:
```
sudo cp /etc/flight-simulator/keys-zibo.conf /etc/flight-simulator/keys.conf
sudo systemctl restart flight-simulator-keys
``` For the Zibo **first
officer's** CDU, change every `fmc1_` to `fmc2_` in `keys.conf`.

When updating, if apt asks whether to replace `keys.conf` with the
package's version, answer **N** to keep your own changes.

### Finding other commands

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
