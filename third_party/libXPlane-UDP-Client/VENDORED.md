# Vendored: libXPlane-UDP-Client

- Upstream: https://github.com/dotsha747/libXPlane-UDP-Client
- Commit:   f2d8338a06975696bc4946797cd2adae3473b8cd (2019-02-10)
- Author:   Shahada Abubakar
- License:  LGPL-3.0-or-later (see COPYING, COPYING.LESSER)

Files here are copied **unmodified** from `src/libsrc/` so the program no
longer depends on the author's apt repository (repo.shahada.abubakar.net),
which no longer exists. It is built as a static library by the top-level
CMakeLists.txt.

Known upstream quirks, worked around in `src/xplane_link.cpp` rather than
patched here:

- `XPlaneUDPClient` accepts only dotted-quad IPs (`inet_addr`), so hostnames
  are resolved first.
- `registerNotificationCallback` adds to a list the listener thread may be
  iterating without a lock, so the beacon cache is polled via `get()` instead.
