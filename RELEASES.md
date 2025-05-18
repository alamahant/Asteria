# Asteria Releases

## Available Distribution Formats

### Flathub
Asteria is available on Flathub, which provides an easy installation method for most Linux distributions with Flatpak support.

[![Flathub Badge](https://flathub.org/assets/badges/flathub-badge-en.png)](https://flathub.org/apps/io.github.alamahant.Asteria)

To install from Flathub:
```bash
flatpak install flathub io.github.alamahant.Asteria
```

### AppImage
Asteria is now also available as an AppImage, which allows you to run the application without installation on most Linux distributions.

[Download the latest AppImage](https://github.com/alamahant/Asteria/releases/latest)

To use the AppImage:
1. Download the AppImage file
2. Make it executable: `chmod +x Asteria-x86_64.AppImage`
3. Run it: `./Asteria-x86_64.AppImage`

## Version History

### v2.0.0 (2025-05-15)
- First release available as AppImage (previously only on Flathub)
- Major Improvements:
  - Completely rebuilt the calculation engine: removed Python dependency, virtual environment, flatlib, and all related scripts
  - Now using direct Swiss Ephemeris integration through native Qt/C++ code for significantly improved performance and reduced complexity
- New Features:
  - Added an orb slider to set how many aspects are calculated and displayed
  - Added additional celestial bodies (Black Moon Lilith, Ceres, Pallas, Juno, Vesta, Vertex, East Point, Part of Spirit)
  - Added a checkbox to toggle displaying additional bodies in the chart
  - Added Aspect Display Settings Dialog for customizing the thickness and style of major and minor aspects
  - Added relationship charts functionality with Composite and Davison charts
- Performance Improvements:
  - Preloaded OpenStreetMap QML at application startup to eliminate pause when opening the map dialog
- Changes:
  - Updated license to AGPL-3 to conform with Swiss Ephemeris requirements

### v1.1.0 (2025-04-30)
- New Features:
  - Added OpenStreetMap dialog to assist users in easily searching and setting their birth location
  - Expanded retrograde planet depiction across the UI
- Improvements:
  - Minor UI improvements for better usability
  - Added multiple screenshots
  - Edited application summary to conform with Flathub quality guidelines

### v1.0.0 (2025-04-22)
- Initial Release on Flathub
- Features:
  - Natal chart calculation and display
  - Aspect analysis
  - AI-powered interpretations
  - Transit calculations

## Reporting Issues

If you encounter any problems with Asteria, please [open an issue](https://github.com/alamahant/Asteria/issues) on our GitHub repository.
