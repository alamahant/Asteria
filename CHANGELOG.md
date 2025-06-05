# Changelog

## Version 2.0.1 (2025-06-05)
### Improvements
- Changed degree display format from 0-360° decimal system to traditional in-sign degrees with minutes for improved astrological readability (e.g., 285.5° → 15°30' Capricorn)
- Only House display shows both formats for reference

## Version 2.0.0 (2025-05-15)
### Major Improvements
- Completely rebuilt the calculation engine: removed Python dependency, virtual environment, flatlib, and all related scripts
- Now using direct Swiss Ephemeris integration through native Qt/C++ code for significantly improved performance and reduced complexity

### New Features
- Added an orb slider to set how many aspects are calculated and displayed
- Added additional celestial bodies to be calculated and displayed:
  - Black Moon Lilith
  - Ceres
  - Pallas
  - Juno
  - Vesta
  - Vertex
  - East Point
  - Part of Spirit
- Added a checkbox to toggle displaying additional bodies in the chart
- Added Aspect Display Settings Dialog where users can customize the thickness and style of major and minor aspects
- Added relationship charts functionality with Composite and Davison charts (synastry to be implemented in a future release)

### Performance Improvements
- Preloaded OpenStreetMap QML at application startup to eliminate pause when opening the map dialog

### Changes
- Updated license to AGPL-3 to conform with Swiss Ephemeris requirements (see credits.txt for full attribution)

## Version 1.1.0 (2025-04-30)
### New Features
- Added [OpenStreetMap](https://www.openstreetmap.org/) dialog to assist users in easily searching and setting their birth location
- Expanded retrograde planet depiction beyond the planets drawn red on the chart itself and the tooltips, to also include the planet widgets, planet view, and aspects view

### Improvements
- Made minor UI improvements for better usability
- Added multiple screenshots to better showcase the application's features
- Edited application summary to conform with Flathub quality guidelines

## Version 1.0.0 (2025-04-22)
### Initial Release
- Natal chart calculation and display
- Aspect analysis
- AI-powered interpretations
- Transit calculations



