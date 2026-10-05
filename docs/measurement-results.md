# Resource measurement results

The results of the procedures in [measuring-resource-usage.md](measuring-resource-usage.md), newest
first, compared with the budgets of the `platform-constraints` spec. A number that misses its budget
is recorded as a defect change, never by relaxing the budget.

## 2026-10-04, overnight run: idle, start-up, reload and memory, Remote 3 and Remote Two

The three remotes of the second run (Remote 3 A with the large configuration, Remote 3 B with the
small one, and the Remote Two), in the dock and untouched, with the display-off timeout at 60 s and
the standby timeout at one hour. Each ran a custom build of commit `a9c281c4`
(`0.82.1-82-ga9c281c4`), which contains all fixes of the second run, installed while the remote was in
`Low_power`. Sampling and validity as in the second run; one sample was discarded, the Remote Two's
first, taken while its accelerometer kept it awake. Every setting that was changed for the run was
recorded before and restored after it.

| Remote         | Remote 3 A | Remote 3 B | Remote Two |
|----------------|------------|------------|------------|
| Configuration  | 497 entities, 1 profile with 6 pages and 34 page items | 55 entities, 2 profiles with 6 pages, 23 page items and 2 groups | 33 entities |

### Display off

`Low_power`, % of one core (main-thread wakeups/s). The render thread was idle in every sample.

| Situation                                         | Remote 3 A              | Remote 3 B   | Remote Two      |
|---------------------------------------------------|-------------------------|--------------|-----------------|
| Right after the install, 60 s samples             | 0.97 (5.1)              | 0.38 (3.2)   | 0.39 (3.2)      |
| After a restart of the UI service, 60 s           | 0.94                    | 0.39         | 0.40            |
| `Low_power` set through the Core-API from `Normal`, 30 s | 0.94             | 0.40         | 0.37            |
| After a restart of the core, 60 s                 |                         | 0.40 (3.2–3.3) | 0.38–0.40 (3.2) |
| Media player playing / paused, 60 s               | 0.91 (4.7) / 0.97 (4.9) | 0.39 / 0.39  |                 |
| After a brightness change and back in `Low_power`, 60 s |                   |              | 0.48–0.50 (3.2–3.3) |

`Idle` set through the Core-API from `Low_power`, 20 s samples: the render thread drew (46 to 50 ticks
on A, 45 to 50 on B, 35 to 38 on the Remote Two), so the window was shown, with the charging screen's
clock; 3.45, 2.9 and 2.35 % of one core.

### Findings

1. **The window fix holds on all three remotes.** The window is hidden in `Low_power` after an
   install, after a restart of the UI service and when `Low_power` is set through the Core-API, with
   the same values as in normal `Low_power`, and it is shown when `Idle` is set through the
   Core-API.
2. **A brightness change through the Core-API turns the display on**, on every model: the core
   leaves `Low_power` for it, and a change in the web-configurator does the same, also on a Remote 3
   in the dock. The only brightness change with the display off is therefore the configured
   brightness arriving when the UI starts, and with the fix the main thread stays quiet then (second
   run, finding 5). Back in `Low_power` after a brightness change, the Remote Two used 0.48 to 0.50 %.
3. **A playing media player costs nothing measurable with the display off.**
4. **The connecting indicator could not be checked on a remote.** The Remote Two's three
   integrations stayed connecting throughout, but the UI never showed the indicator. It updates the
   connecting state only on a driver state event, and the core sends none while it keeps retrying; the
   status load at a start or a reconnect does not update it, a known gap in the `integrations` spec.
   Whether the indicator's animation stops with the window hidden is therefore still open on a
   remote; the desktop simulator showed that it keeps running.
5. **The first run's wake-ups were most likely spurious accelerometer interrupts.** The core's journal
   shows about 80 wake-ups of A between 12:40 and 14:35 UTC on 2026-10-03, 14 to 126 s apart, while
   it lay untouched in the dock. They stopped when it was picked up and docked again, and none came in
   the nine hours since. Its accelerometer had raised 12 158 interrupts in 59 hours, against 110 on B
   in 54 hours; A's wake-up sensitivity is 3, B's 1. No input, charger or reconnect event
   correlates with the wake-ups. The Remote Two showed the same early in this run: its accelerometer
   kept raising interrupts in the dock until about 23:45 UTC and it did not go idle, and even a
   `Low_power` set through the Core-API ended within a second. With the wake-up sensitivity at 0 it
   stayed in `Low_power`; a reboot had cleared the same state the day before. The 30 restarts of the
   core later in the run raised no accelerometer interrupt, so a core restart does not cause it.
   Whether A sat loosely in the dock is not known; all remotes were mostly docked that day. Later,
   `evtest` showed the Remote Two's accelerometer reporting motion every few seconds while it sat in
   the dock without vibration. This requires further investigation in the firmware, not in the UI.
   Since these two Remotes were early development units, this test should also be repeated with 
   production units.
   
6. **A's display-off value without wake-ups is 0.91 to 0.97 % of one core**, B's 0.38 to 0.40 %. The
   difference is A's entity changes (second run, finding 4).

### Start-up time

Five restarts of the UI service per remote, with the display off and `Low_power` before and after
each. The markers come from a temporary test build that logs them at info level, measured from the
service start on the monotonic clock; median (lowest to highest) in ms:

| Marker                                         | Remote 3 A          | Remote 3 B          | Remote Two          |
|------------------------------------------------|---------------------|---------------------|---------------------|
| QML loaded (`Init done`)                        | 4523 (4492–4580)    | 4754 (4676–4775)    | 4584 (4558–4600)    |
| Connected and authenticated                     | 5110 (5062–5170)    | 5342 (5243–5348)    | 5253 (5238–5273)    |
| Last page of the entity load                    | 7472 (7296–7611), 5 pages | 5667 (5554–5758), 1 page | 5524 (5508–6222), 1 page |
| Main page in place (`ACTIVE CONTROL -> MainContainer`) | 7775 (7690–7914) | 7388 (7255–7441) | 7218 (7148–7279) |
| CPU of the new process in its first 50 s        | 10.9 to 11.1 s      | 9.8 to 10.0 s       | 8.8 to 8.9 s        |

### Reload after a restart of the core

Ten restarts of the core per remote, 120 s apart, with the display off before each. The core starts
in `Normal`, so the display was on for about 60 s after every restart; each window ended in
`Low_power`. Same test build; median (lowest to highest) in ms from the start of the core:

| Marker                              | Remote 3 A          | Remote 3 B          | Remote Two          |
|-------------------------------------|---------------------|---------------------|---------------------|
| Authenticated                       | 1487 (1437–1543)    | 1324 (1277–1396)    | 1316 (1265–2033)    |
| Last page of the entity load        | 3711 (3676–4374)    | 2096 (1971–2703)    | 2235 (1747–2548)    |
| Main page rebuilt                   | 3707 (3523–4846)    | 3813 (3686–4399)    | 3553 (3419–4300)    |
| UI CPU in the 113 s around a restart | 11.7 s (11.2–12.0) | 8.2 s (7.9–9.0)     | 7.6 s (7.0–7.7)     |

Resident memory with the normal build, before the first and after the last restart:

| Remote     | VmRSS              | RssAnon            | Highest VmHWM | Threads |
|------------|--------------------|--------------------|---------------|---------|
| Remote 3 A | 75.9 → 83.0 MiB    | 31.9 → 37.7 MiB    | 84.6 MiB      | 10      |
| Remote 3 B | 72.5 → 81.5 MiB    | 29.1 → 38.0 MiB    | 81.8 MiB      | 10      |
| Remote Two | 68.3 → 75.3 MiB    | 25.4 → 32.3 MiB    | 75.8 MiB      | 9       |

### Long-session memory

A sampler records VmHWM, VmRSS, RssAnon, RssFile, the threads and the accelerometer interrupts every
10 minutes on all three remotes since 2026-10-04 00:33 UTC, with the normal build and the original
settings. It pauses while a remote is in standby. Initial readings, about 30 to 55 s after the start
of the UI: Remote 3 A 82.1 MiB (heap 36.6 MiB, 9 threads), Remote 3 B 76.1 MiB (33.0 MiB, 13
threads), Remote Two 71.7 MiB (28.7 MiB, 12 threads).

### Findings, continued

7. **The main page is in place 7.2 to 7.8 s after the UI starts.** About 4.5 s of it is loading the
   QML, before the UI connects to the core; connecting and loading the profile, the pages and the
   entities take the remaining 2.5 to 3 s, longest with A's 497 entities in five pages. This is the
   baseline for the `Start-up time` budget, not slower than the previous release.
8. **After a restart of the core, the main page is rebuilt within about 3.5 to 3.8 s** of the core's
   start; the UI is authenticated after about 1.3 to 1.5 s.
9. **The bulk entity load stops at the last page.** A's 497 entities were loaded in pages 1 to 5 of
   5 at all 5 starts and all 10 reloads, with no request past the end.
10. **Memory grows once at the first reconnect, then hardly.** The first restart of the core added 6
    to 8 MB on every remote; the nine after it added nothing on A and about 0.1 to 0.2 MB per restart
    on B and the Remote Two. Ten restarts do not show a leak; the long-session sampler continues the
    check. Every reading stays far below the 512 MB target.

### Not measured yet

- The long-session memory sampler's results.
- Frame rate, input-to-command latency and the entity scale of 1000 to 4000 entities.
- Whether a connecting indicator keeps animating with the display off on a remote (finding 4).

## 2026-10-03, second run: idle CPU in every power mode, Remote 3 and Remote Two

Two Remote 3 and one Remote Two, all in the dock, with the display-off timeout at 60 s: dimmed
after 30 s, off after 60 s. Every build contains the fix of the status bar animation (finding 1),
since merged as commit `30309ded`: first `v0.82.1-80-g617b394e`, then, for the recheck of the open
points, `v0.82.1-81-g787c2f91`, which is commit `3fa36a06` of `main` plus that fix.

| Remote               | Remote 3 A (the large configuration of the first run) | Remote 3 B (the small one) | Remote Two |
|----------------------|------------------------------------------------------|----------------------------|------------|
| Firmware             | 2.11.0                                               | 2.10.3, development build  | not recorded |
| Configuration        | 497 entities, 8 activities, 1 profile with 6 pages, 5 docks | 55 entities, 6 activities, 2 profiles with 6 pages, 4 docks | three integrations that cannot reach their driver and stay connecting |
| Entity changes while idle | about 3.6 per second, Home Assistant power and energy sensors | none | not recorded |

No integration of the two Remote 3 was reconnecting.

Each run waited for `Low_power` and took three 60 s samples there, then three cycles of: set `Idle`
and then `Normal` through the Core-API, wait 2 s, sample 25 s in `Normal`, wait for the timeout to
`Idle`, sample 25 s in `Idle`. A sample counted only when the power mode was the same before and
after it and the core's journal showed no transition inside it. The values are the means of the
valid samples, as "% of one core (wakeups/s)"; the wakeups are the context switches of **all
threads**. Render-thread values are clock ticks (100 per second) per 25 s sample.

### Display off

| Remote     | Low_power                         | Main thread                     |
|------------|-----------------------------------|---------------------------------|
| Remote 3 A | 1.01 to 1.10 (5.4 to 5.7)         | 62 ticks per 60 s, 5.2 wakeups/s |
| Remote 3 B | 0.38 to 0.53 (3.3 to 3.5)         | 28 ticks per 60 s, 4.3 wakeups/s |
| Remote Two | 0.38 to 0.43 (3.3 to 3.4)         |                                 |

The Remote Two's value is the one after the display had been on once; right after an install it
was higher until the fix of finding 5.

### Display on, by screen

Each screen was opened by hand. `Normal` / `Idle`, % of one core:

| Screen                                 | Remote 3 A    | Remote 3 B    | Remote Two    | Render thread                       |
|----------------------------------------|---------------|---------------|---------------|-------------------------------------|
| About page, static                     | 0.88 / 1.08   | 0.35 / 0.52   | 0.35 / 0.51   | 0 in `Normal`, 3 to 5 per `Idle` sample |
| Charging screen with its analogue clock | 3.79 / 3.95  | 3.47 / 3.44   | 2.87 / 2.80   | about 70 (2.8 %); Remote Two 58 (2.3 %) |
| Main page with a playing media player  | 5.09 / 4.89   | 4.20 / 4.33   | 3.59 / 3.68   | about 93 (3.7 %); Remote Two 77 (3.1 %) |

Resident memory after a fresh start: about 87–88 MB (`VmRSS`) with 41 MB heap (`RssAnon`) on the
Remote 3, 76 MB with 32 MB heap on the Remote Two.

### Findings

1. **The status bar ran an animation all the time.** In `StatusBar.qml` the integration loading
   indicator's animation has `running: show`. The unqualified `show` does not name the indicator's
   own property but resolves to the window's `show()` function, which is always true. The animation
   kept the render loop busy with the display on and woke the main thread with it off; it explains
   the 27 main-thread wakeups per second of the first run. Fixed in commit `30309ded`.
2. **With the fix, the display-off budget is met with a wide margin** on all three remotes: 0.38 to
   1.10 % of one core and 3.3 to 5.7 wakeups per second.
3. **With the display on, the render thread draws what changes on screen, once per second.** A
   static page draws nothing in `Normal`; only the dimming draws a frame in `Idle`. The charging
   screen's seconds hand follows the 1 s clock timer of the UI controller, and a playing media
   player follows its 1 s position timer, so both draw one frame per second; the main thread wakes
   about twice more per second than on a static page. One such frame costs 28 to 37 ms of
   render-thread CPU on the Remote 3 and 23 to 31 ms on the Remote Two. Why a single frame costs that
   much is not known yet; it needs Qt's render timing on the device (`qt.scenegraph.time.renderloop`,
   see the frame rate procedure). Display-on CPU has no budget.
4. **Entity changes cost CPU also with the display off: about 1.6 ms of main-thread time each.**
   Remote 3 A uses about 0.6 % of a core more than B in `Low_power`, all of it in the main thread.
   A receives about 3.6 entity changes per second from Home Assistant power and energy sensors, B
   none. The same difference shows on the static page with the display on. Hardware and firmware do
   not explain it.
5. **Defect: on a Remote Two, a brightness change with the display off keeps an animation running
   until the display turns on.** Only the Remote Two dims in software, through a black overlay in
   `main.qml` whose opacity changes with an `OpacityAnimator` (`Behavior on opacity`). An animator
   runs on the render thread, but the window is hidden while the display is off, so it cannot
   finish, and Qt drives its timer on the main thread instead: 2.0 to 2.5 % of a core and 64 to 66
   wakeups per second in `Low_power`, until the display turns on. The brightness changes with the
   display off only when the configured brightness arrives at a start of the UI, after an install or
   a restart; a brightness change through the Core-API, also in the web-configurator, turns the
   display on (overnight run, finding 2). The fix,
   commit `a9c281c4`, animates the opacity on the main thread instead. With it, a Remote Two installed
   with the display off measured 0.38 to 0.40 % of one core and 3.3 wakeups per second before the
   display had been on, the same as in normal `Low_power`.
6. **An integration that keeps reconnecting keeps the connecting indicator animating, also with the
   window hidden.** The desktop simulator showed about 36 wakeups per second with a driver
   reconnecting and the window hidden. A remote could not show it (overnight run, finding 4), where
   `Idle CPU usage` requires animations to stop with the display off.

## 2026-10-03, first run: idle CPU with the display off and resident memory, Remote 3

Two Remote 3, put into the dock, with the display turned off by the regular timeout; the power mode
was not set through the API, and `Low_power` was confirmed through the Core-API before sampling. Both
ran the custom development build `v0.82.1-72-g66048ae1`, which does not contain the window fix of
commit `b80b9a53`, started on 2026-10-02, about a day before the measurements. The screen shown when
the display turned off was not recorded.

| Configuration       | Large                                                           | Small                                                                       |
|---------------------|-----------------------------------------------------------------|-----------------------------------------------------------------------------|
| Integrations        | 8 enabled: 5 bundled ones connected, 3 external ones reconnecting | 7 enabled: 3 bundled and 1 custom one connected, 3 external ones reconnecting |
| Home Assistant      | 435 configured entities, frequent sensor updates                | 5 configured entities                                                       |
| Firmware            | 2.11.0                                                          | 2.10.3, development build                                                   |
| Power mode during the samples | **woke up every 15 s to 2 min**, about 11 s with the display on or dimmed each time | `Low_power` without interruption, for about 21 hours |

The connected integrations were Apple TV, Denon and Marantz AVR, Home Assistant, Philips Hue and Roon
on the large configuration, and Android TV, Apple TV (custom), Home Assistant and Philips Hue on the
small one. The core's journal shows the power transitions; nothing in it was caused by the API or by
the measurement's shell session.

### Idle CPU, display off

Six samples of 60 s each (`cpu-sample.sh`). The wakeups are the context switches of the **main
thread** only. Only the small configuration's numbers are display-off numbers: every sample of the
large configuration contains one to three wake-ups, with the window shown and rendering.

| Configuration | CPU, % of one core   | Main-thread wakeups/s | Budget       |
|---------------|----------------------|-----------------------|--------------|
| Large, with wake-ups | 1.72 to 3.63, median 2.9 | 28.3 to 39.0, median 35.3 | below 5 % |
| Small         | 1.12 to 1.13         | 26.7 to 26.9          | below 5 %    |

CPU per thread over one 60 s window, in clock ticks (100 per second) and as a share of one core:

| Thread                              | Large, with wake-ups | Small          |
|-------------------------------------|----------------------|----------------|
| Main thread (`remote-ui`)           | 127 (2.1 %)          | 74 (1.2 %)     |
| Scene graph render thread (`QSGRenderThread`) | 96 (1.6 %) | 0              |
| Qt bearer thread                    | 5 (0.08 %)           | 4 (0.07 %)     |
| All other threads                   | 1                    | 0              |

### Resident memory

One reading per remote after about a day of running, not the long-running session of the spec
scenario.

| Configuration | VmRSS     | VmHWM     | RssAnon  | RssFile  | PSS       | Threads | Open files | Budget                     |
|---------------|-----------|-----------|----------|----------|-----------|---------|------------|----------------------------|
| Large         | 107.2 MiB | 107.2 MiB | 59.8 MiB | 47.4 MiB | 100.8 MiB | 10      | 33         | at most 1 GB, target below 512 MB |
| Small         | 103.8 MiB | 111.1 MiB | 57.9 MiB | 45.7 MiB | 97.6 MiB  | 10      | 32         | at most 1 GB, target below 512 MB |

No swap was used.

### Findings

1. **Both budgets are met.** With the display off, the small configuration uses 1.1 % of one core.
   The large configuration stays below 5 % even with its wake-ups included; its display-off value
   alone is not known yet. Resident memory is about a fifth of the 512 MB target.
2. **The large configuration kept waking up in the dock.** Every 15 s to 2 min the core went from
   `Low_power` to `Normal`, the display came on for about 5 s and stayed dimmed for about 6 s, and
   the UI showed and rendered its window in that time. One to three wake-ups per sample explain its
   render thread's 1.6 % and the spread of its samples. What wakes it is not logged; it is not the
   API and not the shell session (see the overnight run, finding 5). A sample with a power
   transition in it is discarded.
3. **The cost of many entities with the display off is not known from this run.** The large
   configuration's main thread used 2.1 % of a core against 1.2 % on the small one, but its time
   includes the wake-ups. The second run traced the cost to the entity changes (its finding 4).
4. **About 27 main-thread wakeups per second remain on a quiet remote.** The second run traced them
   to the status bar animation (its finding 1).
5. **The Qt bearer thread is negligible.** It polls the network configuration every 10 s and costs
   less than 0.1 % of a core.
6. **The number of entities hardly changes the memory.** The heap (`RssAnon`) differs by 2 MiB
   between 435 and 5 configured entities. The mapped binary and fonts (`RssFile`) take 46 to 47 MiB.
7. **A defect found on the way: the window could stay visible with the display off.** The UI hid
   its window only when the mode changed from `Idle` to `Low_power`. A UI started while the display
   was off, or a `Low_power` set through the Core-API, left it visible and rendering until the next
   wake-up; on 2026-10-02 both remotes were in that state after the UI was installed, for 50 and 3
   minutes. It did not affect these samples. Commit `b80b9a53` fixed it: the window now follows the
   current power mode.
