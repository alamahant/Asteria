#include "socialsharedialog.h"
#include <QGridLayout>
#include <QScrollArea>
#include <QMessageBox>
#include <QDialogButtonBox>
#include <QIcon>
#include <QFile>
#include<QCoreApplication>

SocialShareDialog::SocialShareDialog(const QString &defaultText,
                                     const QPixmap &previewImage,
                                     SocialShare *shareManager,
                                     QWidget *parent)
    : QDialog(parent)
    , m_shareManager(shareManager)
    , m_defaultText(defaultText)
    , m_previewImage(previewImage)
{
    setupUI();
    
    connect(m_shareManager, &SocialShare::shareCompleted,
            this, &SocialShareDialog::onShareComplete);
    
    setWindowTitle(QString("Share %1 Chart").arg(QCoreApplication::applicationName()));
    setMinimumSize(500, 450);
    setModal(true);
}

void SocialShareDialog::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    
    // Instructions
    QLabel *instructionLabel = new QLabel("Share your chart with the community:");
    instructionLabel->setStyleSheet("font-weight: bold; margin-bottom: 10px;");
    mainLayout->addWidget(instructionLabel);
    
    QGridLayout *gridLayout = new QGridLayout();
    gridLayout->setSpacing(15);
    //gridLayout->setVerticalSpacing(15);

    // Force all rows to have same height
    //for (int i = 0; i < 4; i++) {
      //  gridLayout->setRowMinimumHeight(i, 80);
      //  gridLayout->setRowStretch(i, 0);
   // }


    // Row 0
    gridLayout->addWidget(createPlatformButton(SocialShare::Twitter, ":/icons/x.svg", "X"), 0, 0);
    gridLayout->addWidget(createPlatformButton(SocialShare::Facebook, ":/icons/facebook.svg", "Facebook"), 0, 1);
    gridLayout->addWidget(createPlatformButton(SocialShare::Reddit, ":/icons/reddit.svg", "Reddit"), 0, 2);

    // Row 1
    gridLayout->addWidget(createPlatformButton(SocialShare::Telegram, ":/icons/telegram.svg", "Telegram"), 1, 1);
    gridLayout->addWidget(createPlatformButton(SocialShare::Instagram, ":/icons/instagram.svg", "Instagram"), 1, 0);
    gridLayout->addWidget(createPlatformButton(SocialShare::Email, ":/icons/send.svg", "Email"), 1, 2);

    // Row 2
    gridLayout->addWidget(createPlatformButton(SocialShare::CopyToClipboard, ":/icons/copy.svg", "Copy"), 2, 1);
    gridLayout->addWidget(createPlatformButton(SocialShare::SaveToFile, ":/icons/save.svg", "Save Image"), 2, 2);
    gridLayout->addWidget(createPlatformButton(SocialShare::OpenFolder, ":/icons/folder.svg", "Open Folder"), 2, 0);

    // Row 3
    gridLayout->addWidget(createPlatformButton(SocialShare::WhatsApp, ":/icons/whatsapp.svg", "WhatsApp"), 3, 0);

    // Empty at 3,1 and 3,2

    QWidget *spacer = new QWidget();
    spacer->setFixedSize(60, 60);
    spacer->setVisible(false);
    gridLayout->addWidget(spacer, 3, 2);

    mainLayout->addLayout(gridLayout);
    mainLayout->addLayout(gridLayout);
    // Separator
    QFrame *line = new QFrame();
    line->setFrameShape(QFrame::HLine);
    line->setFrameShadow(QFrame::Sunken);
    mainLayout->addWidget(line);
    
    // Text editor for custom message
    QLabel *textLabel = new QLabel("Your message:");
    mainLayout->addWidget(textLabel);
    
    m_textEdit = new QTextEdit();
    m_textEdit->setPlainText(m_defaultText);
    m_textEdit->setMaximumHeight(100);
    mainLayout->addWidget(m_textEdit);
    
    // Include screenshot checkbox
    m_includeImageCheck = new QCheckBox("Include screenshot of the spread");
    m_includeImageCheck->setChecked(true);
    m_includeImageCheck->setVisible(false);
    mainLayout->addWidget(m_includeImageCheck);
    
    // Preview area (optional - shows small thumbnail)
    if (!m_previewImage.isNull()) {
        QLabel *previewTitle = new QLabel("Preview:");
        mainLayout->addWidget(previewTitle);
        
        m_previewLabel = new QLabel();
        m_previewLabel->setFixedSize(150, 100);
        m_previewLabel->setScaledContents(true);
        m_previewLabel->setStyleSheet("border: 1px solid #ccc; background-color: #f0f0f0;");
        updatePreview();
        mainLayout->addWidget(m_previewLabel);
    }
    
    // Bottom buttons
    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Close);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    mainLayout->addWidget(buttonBox);
}

QPushButton* SocialShareDialog::createPlatformButton(SocialShare::Platform platform,
                                                      const QString &iconPath,
                                                      const QString &label)
{
    QPushButton *button = new QPushButton();
    button->setIcon(QIcon(iconPath));
    //button->setIconSize(QSize(32, 32));
    button->setToolTip(label);
    button->setFixedSize(32, 32);
    button->setProperty("platform", platform);
    
    button->setStyleSheet(R"(
        QPushButton {
            background-color: #f5f5f5;
            border: 1px solid #ddd;
            border-radius: 8px;
        }
        QPushButton:hover {
            background-color: #e0e0e0;
            border-color: #bbb;
        }
        QPushButton:pressed {
            background-color: #ccc;
        }
    )");
    
    connect(button, &QPushButton::clicked, this, &SocialShareDialog::onPlatformButtonClicked);
    
    m_platformButtons[platform] = button;
    return button;
}

void SocialShareDialog::onPlatformButtonClicked()
{
    QPushButton *button = qobject_cast<QPushButton*>(sender());
    if (!button) return;
    
    SocialShare::Platform platform = static_cast<SocialShare::Platform>(button->property("platform").toInt());
    
    QString shareText = m_textEdit->toPlainText();
    QPixmap imageToShare = m_includeImageCheck->isChecked() ? m_previewImage : QPixmap();
    

    m_shareManager->shareTo(platform, shareText, imageToShare);
}

void SocialShareDialog::onShareComplete(SocialShare::Platform platform, bool success, const QString &message)
{


    if (success) {
        //QMessageBox::information(this, "Success",
          //                       QString("Shared to %1 successfully!\n%2")
            //                     .arg(m_shareManager->platformDisplayName(platform))
            //                     .arg(message));
        
        // Close only for non-copy/save actions that are "done"
        if (platform == SocialShare::CopyToClipboard) {
            QMessageBox::information(this, "Copied",
                "✓ Screenshot and caption copied to clipboard!\n\n"
                "You can now paste (Ctrl+V) anywhere.");
        }
        if (platform != SocialShare::CopyToClipboard && 
            platform != SocialShare::SaveToFile &&
            platform != SocialShare::OpenFolder) {
            accept();
        }
    } else {
        QMessageBox::warning(this, "Share Failed", 
                             QString("Could not share to %1\n%2")
                             .arg(m_shareManager->platformDisplayName(platform))
                             .arg(message));
    }

}

void SocialShareDialog::updatePreview()
{
    if (m_previewLabel && !m_previewImage.isNull()) {
        QPixmap scaled = m_previewImage.scaled(m_previewLabel->size(), 
                                                Qt::KeepAspectRatio, 
                                                Qt::SmoothTransformation);
        m_previewLabel->setPixmap(scaled);
    }
}
