#ifndef SYMBOLSDIALOG_H
#define SYMBOLSDIALOG_H

#include <QDialog>
#include <QTableWidget>
#include <QVBoxLayout>
#include <QTabWidget>

class SymbolsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SymbolsDialog(QWidget *parent = nullptr, const QString &astroFontFamily = "");
    ~SymbolsDialog();

private:
    void setupUI();
    void populateAspectTable();
    void populatePlanetTable();
    void populateSignTable();

    QTabWidget *m_tabWidget;
    QTableWidget *m_aspectTable;
    QTableWidget *m_planetTable;
    QTableWidget *m_signTable;
    QString m_astroFontFamily;
};

#endif // SYMBOLSDIALOG_H
