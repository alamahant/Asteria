#ifndef GLOBALS_H
#define GLOBALS_H

#include <QString>
#include <Qt>
#include <QColor>
#include <QSettings>

// Global orb setting functions
double getOrbMax();
void setOrbMax(double value);

// Global font setting functions
//QString getAstroFontFamily();
//void setAstroFontFamily(const QString &fontFamily);


#include <Qt>
#include <QColor>
// Aspect line style settings
class AspectSettings {
public:
    static AspectSettings& instance() {
        static AspectSettings instance;
        return instance;
    }

    // Show/Hide all aspects
    bool getShowAspectLines() const { return m_showAspectLines; }
    void setShowAspectLines(bool show) { m_showAspectLines = show; }

    // Major aspects
    qreal getMajorAspectWidth() const { return m_majorAspectWidth; }
    void setMajorAspectWidth(qreal width) { m_majorAspectWidth = width; }

    Qt::PenStyle getMajorAspectStyle() const { return m_majorAspectStyle; }
    void setMajorAspectStyle(Qt::PenStyle style) { m_majorAspectStyle = style; }

    // Minor aspects
    qreal getMinorAspectWidth() const { return m_minorAspectWidth; }
    void setMinorAspectWidth(qreal width) { m_minorAspectWidth = width; }

    Qt::PenStyle getMinorAspectStyle() const { return m_minorAspectStyle; }
    void setMinorAspectStyle(Qt::PenStyle style) { m_minorAspectStyle = style; }

    // Reset to defaults
    void resetToDefaults() {
        m_showAspectLines = true;
        m_majorAspectWidth = 1.5;
        m_majorAspectStyle = Qt::SolidLine;
        m_minorAspectWidth = 1.0;
        m_minorAspectStyle = Qt::SolidLine;
    }


private:
    AspectSettings() {
        resetToDefaults();
    }

    // Prevent copying
    AspectSettings(const AspectSettings&) = delete;
    AspectSettings& operator=(const AspectSettings&) = delete;

    bool m_showAspectLines;
    qreal m_majorAspectWidth;
    Qt::PenStyle m_majorAspectStyle;
    qreal m_minorAspectWidth;
    Qt::PenStyle m_minorAspectStyle;

public:
    void saveToSettings(QSettings& settings) {
        settings.beginGroup("aspectDisplay");
        settings.setValue("showAspectLines", m_showAspectLines);
        settings.setValue("majorAspectWidth", m_majorAspectWidth);
        settings.setValue("majorAspectStyle", (int)m_majorAspectStyle);
        settings.setValue("minorAspectWidth", m_minorAspectWidth);
        settings.setValue("minorAspectStyle", (int)m_minorAspectStyle);
        settings.endGroup();
    }

    void loadFromSettings(QSettings& settings) {
        settings.beginGroup("aspectDisplay");
        m_showAspectLines = settings.value("showAspectLines", true).toBool();
        m_majorAspectWidth = settings.value("majorAspectWidth", 1.5).toDouble();
        m_majorAspectStyle = (Qt::PenStyle)settings.value("majorAspectStyle", (int)Qt::SolidLine).toInt();
        m_minorAspectWidth = settings.value("minorAspectWidth", 1.0).toDouble();
        m_minorAspectStyle = (Qt::PenStyle)settings.value("minorAspectStyle", (int)Qt::SolidLine).toInt();
        settings.endGroup();
    }
};

#endif // GLOBALS_H
