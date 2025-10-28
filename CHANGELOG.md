# Changelog

## Version 2.1.2 (2025-10-29) – Polish & Compatibility Update

### Platform & Compatibility
- **Updated KDE Runtime**: Upgraded to KDE Platform 6.9 for enhanced stability and performance
- **Improved Qt 6.9 Compatibility**: Optimized QTableWidget rendering and font handling for the latest Qt framework

### Astrological Accuracy
- **Western Chart Convention**: Chart rendering now places the Ascendant at 9 o'clock position to align with standard Western astrological practices
- **Zodiac Sign Coloring**: Planet list widget now displays zodiac signs in their traditional corresponding colors for better visual recognition

### Code Quality & Performance
- **Code Polish**: Various code improvements and optimizations throughout the application
- **Enhanced Stability**: General bug fixes and performance enhancements for smoother user experience

---


## Version 2.1.1 (2025-09-23) – Major Update

### Major Improvements & New Features

- **Stunning AI Interpretations**: AI responses are now transformed from Markdown into rich HTML, making interpretations beautifully formatted, clear, and engaging—almost like reading a professional digital publication.
- **Smarter Chart Tagging**: Every chart type (Natal, Relationship, Progression, Return, Zodiac Signs) now carries its own tag, so AI provides context-specific interpretations instead of defaulting to natal charts.  
  - **Exception**: Composite charts remain excluded from AI interpretation for accuracy.
- **New Zodiac Sign Chart**: Added a dedicated zodiac signs chart that can generate magazine-style horoscopes for all twelve signs at any chosen date/time.
- **Multi-Window Support**: From **File → New Window**, users can now open multiple Asteria windows—either within the same instance or as new instances. Charts can also be opened in new windows for direct comparison.
- **Drag-and-Drop Between Windows**: With **Ctrl + Left-Click**, charts can be dragged and dropped into another Asteria window (into input or interpretation docks) to instantly load all chart data there.
- **Enhanced Viewing Options**: New checkboxes in the **View menu** let you toggle:
  - **Chart-Only View** for distraction-free focus.
  - **Info Overlay Visibility**, now hidden by default but switchable at will.
- **Clear Interpretation Button**: Added to the interpretation dock, allowing users to clear both the displayed interpretation and stored memory of previous readings in one click.

---

## Version 2.1.0 (2025-07-14) – Major Release

### Major Improvements & New Features

- **Rendering System Overhaul**: Redesigned the chart rendering engine—now all chart elements are drawn in an anticlockwise direction for improved astrological convention and clarity.
- **Return Charts for All Major Planets**: Added support for calculating and displaying return charts for all basic planets (Sun, Moon, Mercury, Venus, Mars, Jupiter, Saturn, Uranus, Neptune, Pluto).
- **Secondary Progression Charts**: Introduced secondary progression chart calculation and display.
- **Eclipse Calculations**: Added functionality to calculate both lunar and solar eclipses.
- **Transits Calculation Enhancements**:
  - Added a new method and UI button to calculate planetary transits for any chart over a period of up to one year.
  - Introduced an advanced transit search dialog, allowing users to filter transits by date, transiting planet, aspect, natal planet, orb, and to apply exclusion filters for precise results.
- **Julian/Gregorian Calendar Toggle**: Implemented a settings menu checkbox to toggle between Julian and Gregorian calendars for dates prior to 1582, ensuring historical accuracy.
- **Chart Type Flag for Transits**: Each calculated transit now includes a chart type flag, which is also passed to the AI interpretation prompt for more targeted and context-aware readings.
- **Date Range Expansion**: Extended the allowed date span for chart and transit calculations to 0001–3000 CE (excluding BCE years due to QDate limitations).
- **Export Chart Data**: Added an option to export all chart data as text, saving the contents of every table in the "Chart Details" tab for easy sharing and analysis.
- **Angles Table**: Added a new Angles table displaying the values of ASC (Ascendant), DESC (Descendant), IC (Imum Coeli), and MC (Medium Coeli).
- **SQQ Aspect**: Added the SQQ (Sesquiquadrate, 135°) aspect to aspect calculations and filtering.
- **General Improvements & Bug Fixes**: Various minor UI polishes, bug fixes, and code optimizations for a smoother user experience.

---

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



