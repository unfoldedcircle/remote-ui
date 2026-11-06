## MODIFIED Requirements

### Requirement: Artwork download
When `media_image_url` changes to a new value, the player SHALL fetch the image: a `data:image/…;base64,` URL is decoded in place; any other URL is fetched over HTTP(S) with redirects followed, a transfer timeout of 15 s, and certificate verification disabled (all SSL errors, including host name mismatch, are ignored and logged). The URL SHALL NOT be logged. The image data SHALL be decoded within fixed limits, whatever the URL said about its type: the format is detected from the data; a bitmap whose longer side exceeds 1024 px SHALL be downscaled to fit 1024 x 1024 while it is decoded; an SVG SHALL be rasterized to fit 1024 x 1024, never at the size it declares. Data that is gzip-compressed, an SVG larger than 256 KB, an SVG without a size, a bitmap larger than 32 megapixels, or data in no format the UI decodes SHALL be treated as undecodable and the reason logged. A failed download SHALL be retried after 1 s, up to 3 attempts in total; after the third failure the artwork is cleared. A response for a URL that is no longer current SHALL be ignored. An unchanged URL SHALL NOT trigger a new download.

#### Scenario: Download succeeds
- **WHEN** the image is downloaded and decodes
- **THEN** it is scaled so that its longer side is at most 1024 px and shown as the player's artwork

#### Scenario: SVG artwork
- **WHEN** the download delivers an SVG document of 200 x 100 declared units
- **THEN** it is rasterized at 1024 x 512 and shown as the player's artwork

#### Scenario: SVG declaring a huge size
- **WHEN** the download delivers an SVG document declaring 20000 x 20000
- **THEN** it is rasterized at 1024 x 1024 and no larger raster is ever allocated

#### Scenario: Compressed or oversized data
- **WHEN** the download delivers gzip-compressed data, an SVG larger than 256 KB or a bitmap larger than 32 megapixels
- **THEN** the data counts as undecodable, the reason (not the URL) is logged, and the retry rule applies

#### Scenario: Download fails three times
- **WHEN** three consecutive attempts fail (network error, timeout or undecodable data)
- **THEN** no artwork is shown and the artwork colour is reset to the default dark grey

#### Scenario: URL changes during a download
- **WHEN** a new `media_image_url` arrives while a previous download is running
- **THEN** the previous result is discarded when it arrives and only the new image is shown
