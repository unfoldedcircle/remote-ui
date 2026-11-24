## ADDED Requirements

### Requirement: Full-screen image as a page background
A page's background image MAY be one of the full-screen sizes 480 x 800 or 480 x 854 besides 480 x 275. The page header SHALL then show the image's top 480 x 275 part, dimmed, exactly as it shows a 480 x 275 image (see `ui-resources`, "Main page background image"). A 480 x 275 image SHALL look as before.

#### Scenario: Existing header-sized background image
- **WHEN** a page's background image is 480 x 275 pixels
- **THEN** the header shows it dimmed exactly as before this change

#### Scenario: Full-screen background image on a page
- **WHEN** a page's background image is 480 x 854 pixels
- **THEN** the header shows its top 480 x 275 part, dimmed, exactly as it would show a 480 x 275 image with that content
