The implementation is on the branch `feat/svg-icons`. The change stays open until the UI change is merged and the core accepts SVG uploads; then it is archived.

## 1. Bounded image decoder (on the branch)

- [x] 1.1 `src/ui/imageDecoder.{h,cpp}`: `ImageDecoder::decode` (format sniffed, gzip rejected,
      SVG capped at 256 KB and rasterized to fit the box, bitmaps above 32 MP rejected and larger
      ones downscaled while decoding), `checkSvgFile`, `isSvgFileName`, `isGzip`
- [x] 1.2 `remote-ui.pro`: header and source registered
- [x] 1.3 `src/ui/entity/mediaPlayer.cpp`: downloaded and base64 artwork decoded through
      `ImageDecoder::decode`; `normalizeMediaImage` removed; the reason of a failed decode logged,
      never the URL
- [x] 1.4 `test/ui/test_image_decoder.cpp` (`testImageDecoder`): small bitmap unchanged, large PNG
      and JPEG downscaled, SVG fitted, SVG with huge declared size bounded, SVG with only a
      viewBox, gzip rejected, oversized SVG rejected, garbage and empty data rejected, file checks,
      file names; SVG cases skip without the Qt SVG plugin

## 2. Custom and TV channel SVG icons (on the branch)

- [x] 2.1 `src/ui/resources.cpp`: `getIcon` uses the file name as is (the dead suffix parsing
      removed), `getCustomIconList` lists `*.svg` and leaves out rejected files with a warning,
      `getIconFile` resolves a rejected file to the placeholder glyph, `getIconPlaceholder` added
- [x] 2.2 `src/qml/components/Icon.qml`: `Image.Error` switches the box to the placeholder glyph
      and logs the icon identifier
- [x] 2.3 `test/ui/test_icon_files.cpp` (`testIconFiles`): list offers PNG, JPG and SVG and skips
      compressed, oversized and non-image files; SVG custom and TV channel icons resolve to their
      file; missing files stay empty; rejected files resolve to the placeholder
- [x] 2.4 `test/ui/CMakeLists.txt`: `imageDecoder.cpp` added to `testIconFont`,
      `testEntityController`, `testEntityScreens` and `testResources`; `testImageDecoder` and
      `testIconFiles` targets
- [x] 2.5 `CHANGELOG.md` entries under Unreleased / Added
- [x] 2.6 unit tests 36/36, `./cpplint.sh` clean, the desktop build compiles
- [x] 2.7 qmlscene harness of `Icon.qml`: a 200 x 100 SVG file icon in a 100 px box paints
      100 x 50, a broken file shows the placeholder glyph and logs `Cannot load icon: custom:broken.svg`

## 3. Outside this repository

- [ ] 3.1 remote-core: `svg` in `file_formats` of `Icon` and `TvChannelIcon`, upload validation
      (format gate, element allow-list, attribute rules, geometry, complexity limits, parse test)
- [ ] 3.2 web-configurator: upload SVG files untouched, bypassing the canvas cropper

## 4. Spec, verification and closing

- [x] 4.1 `ui-resources` delta: `MODIFIED` icon identifiers, missing and default icons, icon
      rendering, icon selector
- [x] 4.2 `media-player` delta: `MODIFIED` artwork download
- [x] 4.3 `openspec validate svg-custom-icons --strict` green
- [ ] 4.4 On a device with a core that accepts SVG: upload an SVG icon, pick it in the selector,
      see it on a tile and in the selector grid; a compressed and an oversized SVG copied into the
      icon directory show the placeholder and log the reason
- [ ] 4.5 Archive the change so the deltas land in the living specs
