## MODIFIED Requirements

### Requirement: Resource files from the resource directory
TV channel icons, page background images and sounds SHALL be read at runtime from the directory given by `UC_RESOURCE_PATH`, in the sub-folders `TvChannelIcon`, `BackgroundImage` and `Sound`. A background image or sound identifier `<prefix>:<file>` SHALL resolve to `<sub-folder>/<file>` for any prefix; an empty file name, an identifier without a colon or a missing file SHALL resolve to nothing.

#### Scenario: Page with background image
- **WHEN** a page's image is `custom:living.jpg` and `<UC_RESOURCE_PATH>/BackgroundImage/living.jpg` exists
- **THEN** the page header shows the image cropped to the header

#### Scenario: Missing background image
- **WHEN** the image file does not exist
- **THEN** the page header shows no image

## ADDED Requirements

### Requirement: Main page background image
A main page background image SHALL be decoded asynchronously at the header size, cropped to fill the header and cached. Background images can be 480 x 275, 480 x 800 or 480 x 854 pixels. A 480 x 275 image SHALL be shown exactly as before full-screen images were allowed. Of an image taller than 480 x 275, only its top 480 x 275 part SHALL be used, and that part SHALL be shown exactly like a 480 x 275 image.

#### Scenario: Existing header-sized image
- **WHEN** a main page's background image is 480 x 275 pixels
- **THEN** the header shows it exactly as before, in the 260 px header and in the larger header of the activity bar

#### Scenario: Full-screen image on a main page
- **WHEN** a main page's background image is 480 x 800 pixels
- **THEN** the header shows the image's top 480 x 275 part exactly as it would show a 480 x 275 image with that content, and the rest of the image is never shown

#### Scenario: Full-screen image with the activity bar
- **WHEN** the header of that page grows to 680 px for the activity bar
- **THEN** the header still shows only the image's top 480 x 275 part, scaled to the header like a 480 x 275 image

### Requirement: UI page background image
A page of an activity's or a remote's `user_interface` MAY carry a `background` with an image identifier `image` and the settings `scale`, `align` and `dim`. While such a page is the current page, its image SHALL fill the whole activity or remote screen behind the title, the page items and the page indicator. Without a `background` (absent or `null`), or when its image resolves to nothing, the screen SHALL stay black. The image SHALL be decoded asynchronously and cached.

#### Scenario: Page with a background image
- **WHEN** the current page's background image is `custom:tv.jpg` and the file exists
- **THEN** the image fills the screen and the title, buttons and page indicator are drawn over it

#### Scenario: Missing image file
- **WHEN** the current page's background image does not exist in the `BackgroundImage` folder
- **THEN** the screen behind the page is black

#### Scenario: Background removed
- **WHEN** a `user_interface` update sets the current page's `background` to `null`
- **THEN** the background fades to black

#### Scenario: Failed activity
- **WHEN** an activity with a page background is in Error
- **THEN** the red title, the activity menu and the overlay of an unavailable entity are still drawn over the background

### Requirement: UI page background scaling
The page background SHALL keep the image's aspect ratio. `scale` `fill`, the default, SHALL scale the image to cover the screen and crop what extends beyond it; `fit` SHALL scale it so that all of it is visible on black; `none` SHALL show it unscaled, cut off or framed in black. `align` `top`, `center` (default) or `bottom` SHALL pick the vertical part kept or the vertical position; horizontally the image SHALL be centred. Unknown `scale` or `align` values SHALL count as the default.

#### Scenario: Full-screen image filling the Remote 3 screen
- **WHEN** the current page's background is a 480 x 854 image with `scale` `fill` and `align` `center` on a Remote 3
- **THEN** the image fills the 480 x 800 screen and 27 px are cut off at its top and at its bottom

#### Scenario: Original size cut off at the bottom
- **WHEN** the current page's background is a 480 x 854 image with `scale` `none` and `align` `top` on a Remote 3
- **THEN** the image is shown unscaled from its top edge and cut off at 800 px

#### Scenario: Remote 3 image on a Remote Two
- **WHEN** the current page's background is a 480 x 800 image with `scale` `fill` on a Remote Two
- **THEN** the image is scaled to 512 x 854 and 16 px are cut off at each side

#### Scenario: Whole image visible
- **WHEN** the current page's background is a 480 x 800 image with `scale` `fit` and `align` `top` on a Remote Two
- **THEN** the whole image is shown unscaled at the top and a 54 px black strip remains below it

#### Scenario: Unknown settings from a newer core
- **WHEN** the current page's background has `scale` `tile` and `align` `left`
- **THEN** it is shown as `fill` and `center`

### Requirement: UI page background dimming
A black scrim SHALL be drawn over the page background image with an opacity of `dim` percent. `dim` SHALL range from 0 to 80; a value outside that range SHALL be clamped to it, and a missing or non-numeric value SHALL count as 40.

#### Scenario: Scrim
- **WHEN** the current page's background has `dim` 60
- **THEN** the image is drawn under a black layer of 60 % opacity

#### Scenario: Default scrim
- **WHEN** the current page's background has only an `image`
- **THEN** the image fills the screen centred, under a black layer of 40 % opacity

#### Scenario: Value out of range
- **WHEN** the current page's background has `dim` 95
- **THEN** the black layer has 80 % opacity

### Requirement: UI page background transitions
When the current page changes, or a `user_interface` update changes the current page's background, the background SHALL cross-fade to the new one in 200 ms once the new image is decoded. When the old and the new background are the same image with the same settings, the background SHALL NOT change visibly. Only the background of the most recent page change SHALL be shown.

#### Scenario: Swiping to a page with another image
- **WHEN** the user swipes from a page with background A to a page with background B
- **THEN** the background cross-fades from A to B in 200 ms

#### Scenario: Swiping between pages that share a background
- **WHEN** the user swipes between two pages whose backgrounds are the same image with the same settings
- **THEN** the background does not change visibly

#### Scenario: Background changed in the web-configurator
- **WHEN** a `user_interface` update changes the background of the current page
- **THEN** the screen cross-fades to the new background without leaving the page

#### Scenario: Fast page changes
- **WHEN** the user switches pages twice before the first page's image is decoded
- **THEN** the background of the second target page is shown, not the first one
