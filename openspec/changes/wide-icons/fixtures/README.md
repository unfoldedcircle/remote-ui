# Wide icons: device check fixtures

Files for task 4.1. Copy them into `<UC_RESOURCE_PATH>/Icon/` of the core (or the core-simulator)
and use them as `custom:<file name>` icon items.

Every SVG is drawn edge to edge: a white frame at the viewBox border, white diagonals, a circle in
the middle and a yellow square in the top left corner. A gap between the frame and the button
edge means letterboxing; a squashed circle or off-angle diagonals mean distortion. Both are bugs.

| File | Aspect ratio | Expected |
| --- | --- | --- |
| `ratio-1x1.svg` | 1:1 | square, as before |
| `ratio-2x1.svg` | 2:1 | fills a 2 x 1 button, half height in a 1 x 1 button |
| `ratio-3x1.svg` | 3:1 | centred with side gaps in a 4 x 1 button |
| `ratio-4x1.svg` | 4:1 | fills a 4 x 1 button, a quarter of the height in a 1 x 1 button |
| `ratio-1x2.svg` | 1:2 | fills a 1 x 2 button |
| `ratio-1x4.svg` | 1:4 | fills a 1 x 4 button |
| `mismatch-2x1-declared-100px.svg` | 2:1 | viewBox 2:1, root size 100px x 100px: drawn 2:1, not square |
| `units-2x1-percent.svg`, `units-2x1-em.svg` | 2:1 | root size in `%` / `em`: drawn 2:1 |
| `size-32kb-4x1.svg`, `size-32kb-1x4.svg` | 4:1 / 1:4 | about 32 KB (task 4.2: lookup timing and sharpness on a device) |
| `bitmap-1x1.png` | 1:1 | PNG, unchanged: square |
| `broken.svg` | none | not valid SVG: the placeholder icon |

Button sizes to try on the default 4 x 6 grid: 1 x 1, 2 x 1, 4 x 1, 4 x 2, 1 x 4 and 4 x 6.
