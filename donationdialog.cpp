#include "donationdialog.h"
#include <QMessageBox>

DonationDialog::DonationDialog(QWidget *parent)
    : QDialog(parent)
{
    setupUI();
}

void DonationDialog::setupUI()
{
    setWindowTitle(tr("Support %1").arg(QApplication::applicationName()));
    setFixedSize(500, 500); // Increased height to 500

    QVBoxLayout *layout = new QVBoxLayout(this);

    // Header
    QLabel *headerLabel = new QLabel(getDonationContent());
    headerLabel->setWordWrap(true);
    headerLabel->setTextFormat(Qt::RichText);
    headerLabel->setOpenExternalLinks(true);
    layout->addWidget(headerLabel);

    // Donation platforms
    layout->addWidget(createDonationSection(
        //"☕ Buy Me a Coffee",
        "<span style='color:saddlebrown;'>☕</span> Buy Me a Coffee",
        "https://buymeacoffee.com/Alamahant",
        "buymeacoffee.com/Alamahant"
    ));

    layout->addWidget(createDonationSection(
        //"❤️ Ko-fi",
        "<span style='color:red;'>❤️</span> Ko-fi",
        "https://ko-fi.com/alamahant",
        "ko-fi.com/alamahant"
    ));

    layout->addWidget(createDonationSection(
        //"💰 PayPal",
        "<span style='color:#B8860B; font-weight:bold;'>$</span> PayPal",
        "https://paypal.me/Alamahant",
        "paypal.me/Alamahant"
    ));

    // Support enables section - NOW VISIBLE
    QLabel *supportEnablesLabel = new QLabel(
        R"(
            <div style="background-color: #f8f9fa; padding: 15px; border-radius: 8px; margin: 20px 0;">
                <p style="margin: 0; font-size: 14px; color: #2c3e50; font-weight: bold; line-height: 1.5;">
                    <strong style="font-size: 15px;">✨ Your support enables:</strong><br>
                    • New features and improvements<br>
                    • Bug fixes and maintenance<br>
                    • Future updates and compatibility
                </p>
            </div>
        )"
    );
    supportEnablesLabel->setTextFormat(Qt::RichText);
    supportEnablesLabel->setWordWrap(true);
    layout->addWidget(supportEnablesLabel);

    // Spacer
    layout->addStretch();

    // Close button
    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Close);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    layout->addWidget(buttonBox);
}


QWidget* DonationDialog::createDonationSection(const QString &title, const QString &url, const QString &displayUrl)
{
    QWidget *widget = new QWidget();
    QHBoxLayout *layout = new QHBoxLayout(widget);
    layout->setContentsMargins(20, 10, 20, 10);

    // Title and link
    QLabel *linkLabel = new QLabel(
        QString(R"(<b>%1:</b> <a href="%2" style="color: #3498db; text-decoration: none;">%3</a>)")
            .arg(title, url, displayUrl)
    );
    linkLabel->setTextFormat(Qt::RichText);
    linkLabel->setOpenExternalLinks(true);
    layout->addWidget(linkLabel, 1);

    // Copy button
    QPushButton *copyButton = new QPushButton(tr("Copy"));
    copyButton->setFixedSize(60, 25);
    connect(copyButton, &QPushButton::clicked, [this, url]() {
        copyToClipboard(url);
    	QMessageBox::information(this, "Success", "URL copied to clipboard!");
    });
    layout->addWidget(copyButton);

    return widget;
}

void DonationDialog::copyToClipboard(const QString &url)
{
    QApplication::clipboard()->setText(url);

    // Optional: You can add a temporary tooltip or status message here
    // For example: QToolTip::showText(QCursor::pos(), tr("Link copied!"));
}

QString DonationDialog::getDonationContent() const
{
    return QString(R"(
        <div style="text-align: center; font-family: Arial, sans-serif; color: #2c3e50;">
        <h2 style="color: #2c3e50; margin-bottom: 15px;">
            <span style="color: red;">❤️</span> Support %1
        </h2>
            <p style="font-size: 14px; line-height: 1.5; margin-bottom: 20px;">
                If you find <strong>%1</strong> useful and you enjoy using it,
                please consider supporting its development. Your donation
                helps maintain and improve this application, ensuring it
                remains free and actively developed.
            </p>
            <p style="font-size: 14px; line-height: 1.5; margin-bottom: 25px;">
                Every contribution, no matter how small, makes a difference
                and is greatly appreciated! Choose your preferred platform below.
            </p>
            <div style="background-color: #f8f9fa; padding: 15px; border-radius: 8px; margin: 10px 0;">
                <p style="margin: 0; font-size: 14px; color: #2c3e50; font-weight: bold; line-height: 1.5;">
                <strong style="font-size: 15px;">✨ Your support enables:</strong><br>
                    • New features and improvements<br>
                    • Bug fixes and maintenance<br>
                    • Future updates and compatibility
                </p>
            </div>
        </div>
    )").arg(QApplication::applicationName());
}
