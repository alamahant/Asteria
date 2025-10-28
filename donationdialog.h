#ifndef DONATIONDIALOG_H
#define DONATIONDIALOG_H

#include <QDialog>

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QDialogButtonBox>
#include <QApplication>
#include <QClipboard>

class DonationDialog : public QDialog
{
    Q_OBJECT

public:
    explicit DonationDialog(QWidget *parent = nullptr);

private slots:
    void copyToClipboard(const QString &url);

private:
    void setupUI();
    QWidget* createDonationSection(const QString &title, const QString &url, const QString &displayUrl);
    QString getDonationContent() const;
};

#endif // DONATIONDIALOG_H
