# Measuring the resource budgets on a device

> **For the maintainers.** These procedures need shell access to a Remote Two or Remote 3, which
> only Unfolded Circle developers have; a remote in normal use offers no shell. Contributors are
> not expected to take these measurements: the maintainers take them before a release and when a
> change touches a budget. If you suspect a regression, open an issue with what you did, when, and
> the log from the web-configurator's log download.

How to measure the non-functional budgets of the `platform-constraints` spec (idle CPU, resident
memory, start-up time, frame rate, input-to-command latency, binary size, entity scale) on a
Remote Two and a Remote 3. Every number is taken on the device, not in the desktop simulator: the
simulator has a different CPU, GPU, Qt build and power model.

Besides the shell, the procedures only need `/proc`, `journalctl` and the BusyBox tools of the
firmware. Where a procedure needs a log line, it names the Qt logging category; nothing has to be
rebuilt. An account without root can read `/proc/<pid>/stat`, `/proc/<pid>/status` and the same
files under `/proc/<pid>/task/`, which is all the CPU and the resident memory need; the proportional
set size (`smaps_rollup`) and the list of open files (`fd`) need root.

## Before every measurement

- Note the firmware version, the `remote-ui` version (Settings → About), the model and the
  battery state, and whether the remote is on the charger.
- Use a real profile: the one whose numbers matter is a fully configured remote (several pages,
  activities, media players with artwork, 10+ integrations), not a fresh onboarding.
- Standby ends any measurement: the process is frozen during suspend and the numbers after a wake
  are not idle numbers. Set the standby timeout (Settings → Power Saving) longer than the sampling
  window, and set the display-off timeout to what the scenario needs.
- Find the process once: `pid=$(pidof remote-ui)`. A restart changes it.
- Read the power mode through the Core-API (`GET /api/system/power`) right before and after every
  sampling window, and drop a window in which the core's journal shows a power mode change: a
  wake-up in the middle makes it a mixed window.
- Enable debug output only for the categories a procedure names, through the service environment
  (`QT_LOGGING_RULES="uc.core.debug=true;uc.ui.input.debug=true"` **and** `QT_FORCE_STDERR_LOGGING=1`),
  and turn it off afterwards: debug logging itself costs CPU and journald space. The device's
  journald stores entries only up to priority info, so debug lines Qt sends to the journal directly
  are dropped; `QT_FORCE_STDERR_LOGGING=1` routes them through stderr, which the service manager
  stores at info priority. `journalctl _COMM=remote-ui` shows only the UI's lines either way.

## Idle CPU (`Idle CPU usage`, budget: below 5 % of one core with the display off)

The budget is a share of **one core**. Do not read it off `top`: BusyBox `top` reports a share of
all cores, procps `top` a share of one core, and both average over whatever interval they happen
to use. Count the process' CPU ticks in `/proc` over a known window instead:

```sh
#!/bin/sh
# cpu-sample.sh [seconds] — CPU time of remote-ui as a percentage of one core over the window
pid=$(pidof remote-ui) || exit 1
secs=${1:-60}
hz=$(getconf CLK_TCK 2>/dev/null || echo 100)
ticks() { set -- $(sed 's/.*) //' /proc/$pid/stat); echo $(( ${12} + ${13} )); }   # utime + stime
switches() { awk '/ctxt_switches/ {s += $2} END {print s}'; }  # voluntary + involuntary, from stdin
t0=$(ticks); m0=$(switches < /proc/$pid/status); a0=$(cat /proc/$pid/task/*/status | switches)
sleep "$secs"
t1=$(ticks); m1=$(switches < /proc/$pid/status); a1=$(cat /proc/$pid/task/*/status | switches)
awk -v d=$((t1 - t0)) -v hz="$hz" -v s="$secs" -v m=$((m1 - m0)) -v a=$((a1 - a0)) \
    'BEGIN { printf "remote-ui: %.2f %% of one core, %.1f wakeups/s, main thread %.1f, over %d s\n", d * 100 / (hz * s), a / s, m / s, s }'
```

`/proc/<pid>/stat` counts the CPU time of all threads, but `/proc/<pid>/status` counts the context
switches of the main thread only; the script therefore sums `/proc/<pid>/task/*/status` for the
whole process and prints the main thread's share separately.

Per thread, to see *who* is awake (the render thread, the main thread, the WebSocket thread):

```sh
for t in /proc/$pid/task/*; do
    printf '%s %s\n' "$(cat $t/comm)" "$(sed 's/.*) //' $t/stat | awk '{print $12 + $13}')"
done
```

Run it twice, `sleep 60` apart, and subtract per thread.

Scenarios, each sampled at least three times. Without input the remote stays in Normal and in Idle
for at most about 30 s each, half of the display-off timeout, which is at most 60 s. Sample these
two in windows shorter than that (`cpu-sample.sh 25`) and Low_power in 60 s windows. Setting the
same mode again through the Core-API does not extend it.

1. **Display off (Low_power).** Leave the main page with animated content (a playing media
   player) on screen, wait for the display to turn off through the timeout (Normal, Idle,
   Low_power), then sample. This is the 5 % budget. Check the render thread (`QSGRenderThread`) in
   the per-thread numbers: it uses no CPU while the window is hidden. Builds before commit
   `b80b9a53` hid the window only when the mode changed from Idle to Low_power; with those, note how
   the display went off, since a UI started while the display was off, or a Low_power set through
   the Core-API, kept rendering.
2. **Display dimmed (Idle).** Same, sampled after the dim step and before the display goes off.
   Not budgeted, but the number shows what the visible content costs. With builds before commit
   `b80b9a53`, reach it from a lit display: Idle set through the Core-API from Low_power left the
   window hidden.
3. **Display on, nobody touching it.** Main page, media player playing (artwork and position
   updates arriving), then a static settings page for comparison. From Low_power, set Idle and
   then Normal through the Core-API (`PUT /api/system/power?power_mode=IDLE`, then `NORMAL`); the
   timeout then runs through scenarios 3, 2 and 1 in turn. Setting Normal directly from Low_power
   while the remote charges opens the charging screen instead of the page. No API opens a page,
   so someone opens the main page or the settings page by hand.
4. **Bursts** are expected: note a burst (page load, an event storm) separately instead of
   averaging it away.

Wakeups per second are the better leak detector for idle work than the percentage: a timer that
fires every second shows up as one wakeup per second even when its CPU share rounds to zero. The
main thread's wakeups show the event loop, its timers and the Core-API messages; the rest come from
the render, network and worker threads.
Whole-device context — what remote-core and the integrations use at the same time — comes from
`top -b -n 2 -d 30`; read only the second iteration, the first is the average since boot.

## Resident memory (`Memory usage`, budget: at most 1 GB, target below 512 MB)

The budget is `VmRSS`. Read it, its peak and its composition:

```sh
grep -E 'VmRSS|VmHWM|RssAnon|RssFile|RssShmem|VmSwap|Threads' /proc/$pid/status
ls /proc/$pid/fd | wc -l           # file descriptors: a second leak indicator
awk '/^Pss:/ {print}' /proc/$pid/smaps_rollup 2>/dev/null   # proportional set size, if the kernel has it
```

`RssAnon` is the heap (QML objects, models, decoded images); `RssFile` is the mapped binary and
fonts and is shared with nothing else on a static build; `VmHWM` is the high-water mark since the
process started, so one reading after a long session is enough to know the peak.

GPU memory is not in RSS: textures and the eglfs framebuffers live in driver-owned buffers. To
include them, compare `/proc/meminfo` (`MemAvailable`, and `CmaFree` if present) with the UI
stopped (`systemctl stop` of the UI service) and running with the same content; the difference
minus RSS is what the GPU driver holds for the UI.

Long-running session (the spec scenario):

```sh
while :; do
    printf '%s %s\n' "$(date '+%F %T')" "$(grep VmRSS /proc/$(pidof remote-ui)/status)"
    sleep 600
done >> /tmp/remote-ui-rss.log
```

Use the remote normally for a day or two (page swipes, media browsing with artwork, entity
lists, a few standbys). RSS that still climbs after hours of the same usage is a leak; RSS that
grows in steps and then stays is a cache — compare against the media image cache (12 images) and
the entity lists. Note the highest value with the profile size it was reached with.

## Start-up time (`Start-up time`, budget: not slower than the previous release)

Measured from the start of the UI service to the main page in place with a running core. Both
ends are available in every build: the service's start, and the info line
`ACTIVE CONTROL -> MainContainer`, written when the main page takes the input. The debug line
`Init done` only marks the end of the QML loading, before the UI connects to the core.

```sh
systemctl show -p ExecMainStartTimestampMonotonic <ui-service>      # service start, µs on the monotonic clock
journalctl _COMM=remote-ui -b -o short-monotonic | grep 'ACTIVE CONTROL -> MainContainer' | tail -n 1
```

Both use the monotonic clock, so their difference is the start-up time; `/proc/uptime` is not
comparable, it includes the time the remote was suspended. Restart the UI service five times
(`sudo systemctl restart <ui-service>`) with the remote in Low_power, and keep the median per
release; the first start after a firmware update is not comparable (caches, font loading). Record the number of pages and
entities of the profile: the first page waits for the core's answers (profile, pages, the
entities of the first page), so the core's answer time is part of the number and must stay
comparable between releases.

## Frame rate (`Rendering frame rate`, budget: 60 fps, never below 50)

Qt's scene graph reports its own timing per frame. Enable it for a short window only, it logs a
line per frame:

```sh
QT_LOGGING_RULES="qt.scenegraph.time.renderloop=true"     # in the service environment
journalctl _COMM=remote-ui -f -o short-precise | grep 'Frame rendered'
```

Each line carries the total, sync, render and swap times of one frame in milliseconds (the
threaded render loop, which eglfs uses; the basic loop adds polish). Frames per second is the
number of lines per second while the animation runs; a frame whose total exceeds 16.7 ms is a
dropped frame (20 ms is the 50 fps floor). Measure:

1. A page swipe on the main screen, forward and back, with tiles that show artwork.
2. Scrolling the entity list of the largest integration (the entity scale run below uses the
   same list).
3. Opening and closing a media player's control screen.

Note the frame count and the worst frame per scenario and per model; the Remote 3 and Remote Two
render on different GPUs and both have to pass.

## Input-to-command latency (`Input-to-command latency`, budget: 20 ms to the request)

The app has no timing hook; the measurement uses two debug lines and journald's microsecond
timestamps:

```sh
QT_LOGGING_RULES="uc.ui.input.debug=true;uc.core.debug=true"
journalctl _COMM=remote-ui -f -o short-precise | grep -E 'Key pressed|Key released|Sending request'
```

`Key pressed:` / `Key released:` is written when the input controller sees the event; `Sending
request:` when the command leaves for the WebSocket. Their difference is the in-app path. It
excludes the time from the physical press until Qt delivers the event, which is the kernel input
path plus the event loop. To include it, add the platform event's timestamp to the `Key pressed`
line (`QKeyEvent::timestamp()`, milliseconds from the evdev event) and compare it with the
timestamp of `Sending request` — a change of its own that has not been made yet.

Cases to measure, ten presses each, report the median and the maximum:

- a physical button mapped to an entity command (press → request);
- a key with a long-press action, short press (release → request);
- a touch on a tile (touch → request). Touch events are not logged by the input controller;
  for the touch timestamp enable `qt.qpa.input.events=true` (the evdevtouch plugin's own debug
  line) for the duration of the test.

A brightness or volume key is not a latency case: it coalesces and sends after the shared delay
(`Repeated value change` scenario); verify instead that exactly one command follows the release.

## Binary size (`Binary size`, budget: below 100 MB)

`ls -l remote-ui` of the static device binary built with the toolchain image
(`docs/cross-compile.md`); the CI cross-build job prints the same. Compare per release together
with the list of embedded resources (`resources/qrc/*.qrc`) when it grows.

## Entity scale (`Capacity and limits`, 1000 to 4000 entities)

Configure an integration that exposes 1000 to 4000 entities (a large Home Assistant instance, or
a test driver that generates them) and repeat the memory and frame-rate procedures on:

- the entity list of that integration, scrolled to the end;
- search in that list (the time from the search request to its answer is the difference between
  the `Sending request` line and the matching response line in `uc.core` debug output);
- a page with 20+ tiles from that integration.

Record RSS before and after, the worst frame, and whether the UI stayed responsive to the keypad
while the list loaded.

## Recording the results

Add the numbers to [measurement-results.md](measurement-results.md), newest first, as a table per
model with the conditions above. The budgets are the `platform-constraints` capability. A number that misses its
budget is recorded as a defect change, never by relaxing the budget.
