#ifndef SOCIALSHAREDIALOG_H
#define SOCIALSHAREDIALOG_H

#include <QDialog>
#include <QPushButton>
#include <QTextEdit>
#include <QLabel>
#include <QCheckBox>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QGridLayout>
#include <QScrollArea>
#include "socialshare.h"

class SocialShareDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SocialShareDialog(const QString &defaultText, 
                               const QPixmap &previewImage,
                               SocialShare *shareManager,
                               QWidget *parent = nullptr);

private slots:
    void onPlatformButtonClicked();
    void onShareComplete(SocialShare::Platform platform, bool success, const QString &message);

private:
    void setupUI();
    QPushButton* createPlatformButton(SocialShare::Platform platform, 
                                      const QString &iconPath,
                                      const QString &label);
    void updatePreview();

    SocialShare *m_shareManager;
    QString m_defaultText;
    QPixmap m_previewImage;
    QTextEdit *m_textEdit;
    QLabel *m_previewLabel;
    QCheckBox *m_includeImageCheck;
    QHBoxLayout *m_buttonLayout;
    
    QHash<SocialShare::Platform, QPushButton*> m_platformButtons;
};

#endif