#include "modelselectordialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QListWidget>
#include <QPushButton>
#include <QLineEdit>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QSpinBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QMessageBox>
#include <QSettings>
#include <QFont>
#include<QLabel>

ModelSelectorDialog::ModelSelectorDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(tr("AI Model Selector"));
    setMinimumSize(400, 300);

    // Create widgets
    m_listWidget = new QListWidget(this);
    m_listWidget->setSelectionMode(QAbstractItemView::SingleSelection);

    QPushButton *addButton = new QPushButton(tr("Add"), this);
    m_editButton = new QPushButton(tr("Edit"), this);
    m_deleteButton = new QPushButton(tr("Delete"), this);
    m_setActiveButton = new QPushButton(tr("Set Active"), this);
    QPushButton *closeButton = new QPushButton(tr("Close"), this);

    // Initially disable buttons that require a selection
    m_editButton->setEnabled(false);
    m_deleteButton->setEnabled(false);
    m_setActiveButton->setEnabled(false);

    // Layout
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(m_listWidget);

    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->addWidget(addButton);
    buttonLayout->addWidget(m_editButton);
    buttonLayout->addWidget(m_deleteButton);
    buttonLayout->addWidget(m_setActiveButton);
    buttonLayout->addStretch();
    buttonLayout->addWidget(closeButton);
    mainLayout->addLayout(buttonLayout);

    // Connect signals
    connect(addButton, &QPushButton::clicked, this, &ModelSelectorDialog::onAddClicked);
    connect(m_editButton, &QPushButton::clicked, this, &ModelSelectorDialog::onEditClicked);
    connect(m_deleteButton, &QPushButton::clicked, this, &ModelSelectorDialog::onDeleteClicked);
    connect(m_setActiveButton, &QPushButton::clicked, this, &ModelSelectorDialog::onSetActiveClicked);
    connect(closeButton, &QPushButton::clicked, this, &QDialog::accept);
    connect(m_listWidget, &QListWidget::itemDoubleClicked, this, &ModelSelectorDialog::onItemDoubleClicked);
    connect(m_listWidget, &QListWidget::itemSelectionChanged, this, [this]() {
        bool hasSelection = m_listWidget->currentItem() != nullptr;
        m_editButton->setEnabled(hasSelection);
        m_deleteButton->setEnabled(hasSelection);
        m_setActiveButton->setEnabled(hasSelection);
    });

    //QLabel *infoLabel = new QLabel(tr("Note: Works best with Mistral, OpenAI (ChatGPT), and Ollama (local) models."));
    //infoLabel->setToolTip("");
    QLabel *infoLabel = new QLabel(tr("Note: Works with any OpenAI-compatible API model.Hover for details.\n"
                                      "You MUST restart Asteria after configuring your first model."));
    infoLabel->setToolTip(tr(
        "<b>✅ Fully compatible (OpenAI format):</b><br>"
        "• Mistral<br>"
        "• OpenAI (ChatGPT, GPT-4)<br>"
        "• Groq (fast inference, free tier)<br>"
        "• Ollama (local models, no API key)<br>"
        "• Together AI<br>"
        "• DeepSeek<br>"
        "• Perplexity API<br>"
        "• Fireworks AI<br>"
        "• AnyLocal (OpenAI compatibility mode)<br><br>"
        "<b>❌ NOT compatible (different formats):</b><br>"
        "• Claude (Anthropic)<br>"
        "• Gemini (Google)<br>"
        "• Cohere<br><br>"
        "For local models: Install Ollama, use endpoint http://localhost:11434/v1/chat/completions"

    ));

    mainLayout->addWidget(infoLabel);
    // Load existing models
    loadModels();
    refreshModelList();
}

ModelSelectorDialog::~ModelSelectorDialog()
{
}

QVector<Model> ModelSelectorDialog::models() const
{
    return m_models;
}

QString ModelSelectorDialog::activeModel() const
{
    return m_activeModel;
}

void ModelSelectorDialog::loadModels()
{
    m_models.clear();
    QSettings settings;

    settings.beginGroup("Models");
    m_activeModel = settings.value("ActiveModel").toString();

    QStringList modelGroups = settings.childGroups();
    for (const QString &name : modelGroups) {
        settings.beginGroup(name);
        Model model;
        model.name = name;
        model.provider = settings.value("provider").toString();
        model.endpoint = settings.value("endpoint").toString();
        model.apiKey = settings.value("apiKey").toString();
        model.modelName = settings.value("modelName").toString();
        model.temperature = settings.value("temperature", 0.7).toDouble();
        model.maxTokens = settings.value("maxTokens", 8192).toInt();
        m_models.append(model);
        settings.endGroup();
    }
    settings.endGroup();
}

void ModelSelectorDialog::saveModels()
{
    QSettings settings;
    settings.remove("Models"); // clear existing

    settings.beginGroup("Models");
    settings.setValue("ActiveModel", m_activeModel);
    for (const Model &model : m_models) {
        settings.beginGroup(model.name);
        settings.setValue("provider", model.provider);
        settings.setValue("endpoint", model.endpoint);
        settings.setValue("apiKey", model.apiKey);
        settings.setValue("modelName", model.modelName);
        settings.setValue("temperature", model.temperature);
        settings.setValue("maxTokens", model.maxTokens);
        settings.endGroup();
    }
    settings.endGroup();
    settings.sync();
}

void ModelSelectorDialog::setActiveModel(const QString &name)
{
    if (m_activeModel != name) {
        m_activeModel = name;
        saveModels();
        emit activeModelChanged(name);
    }
}

bool ModelSelectorDialog::modelNameExists(const QString &name, const QString &ignoreName) const
{
    for (const Model &m : m_models) {
        if (m.name == name && m.name != ignoreName)
            return true;
    }
    return false;
}

void ModelSelectorDialog::refreshModelList()
{
    m_listWidget->clear();
    for (const Model &model : m_models) {
        QListWidgetItem *item = new QListWidgetItem(model.name);
        if (model.name == m_activeModel) {
            QFont font = item->font();
            font.setBold(true);
            item->setFont(font);
        }
        m_listWidget->addItem(item);
    }
}

void ModelSelectorDialog::onAddClicked()
{
    showEditDialog(nullptr);
}

void ModelSelectorDialog::onEditClicked()
{
    QListWidgetItem *current = m_listWidget->currentItem();
    if (!current) return;

    QString name = current->text();
    // Find the model by name
    for (int i = 0; i < m_models.size(); ++i) {
        if (m_models[i].name == name) {
            showEditDialog(&m_models[i]);
            break;
        }
    }
}

void ModelSelectorDialog::onDeleteClicked()
{
    QListWidgetItem *current = m_listWidget->currentItem();
    if (!current) return;

    QString name = current->text();
    int ret = QMessageBox::question(this, tr("Delete Model"),
                                    tr("Are you sure you want to delete model '%1'?").arg(name),
                                    QMessageBox::Yes | QMessageBox::No);
    if (ret != QMessageBox::Yes) return;

    // Remove from vector
    for (int i = 0; i < m_models.size(); ++i) {
        if (m_models[i].name == name) {
            m_models.removeAt(i);
            break;
        }
    }

    // If the active model was deleted, clear active
    if (m_activeModel == name) {
        setActiveModel(QString());
    }

    saveModels();
    refreshModelList();
    emit modelsChanged();
}

void ModelSelectorDialog::onSetActiveClicked()
{
    QListWidgetItem *current = m_listWidget->currentItem();
    if (!current) return;

    QString name = current->text();
    setActiveModel(name);
    refreshModelList(); // to update bold
    emit modelsChanged(); // though active changed already emitted
}

void ModelSelectorDialog::onItemDoubleClicked(QListWidgetItem *item)
{
    // Treat double-click as edit
    onEditClicked();
}

void ModelSelectorDialog::showEditDialog(Model *model)
{
    QDialog dialog(this);
    dialog.setWindowTitle(model ? tr("Edit Model") : tr("Add Model"));

    QLineEdit *nameEdit = new QLineEdit(&dialog);
    nameEdit->setText("Default");
    nameEdit->setToolTip(tr("A unique name to identify this model configuration"));
    QLineEdit *providerEdit = new QLineEdit(&dialog);
    providerEdit->setText("Mistral");
    providerEdit->setToolTip(tr("The AI provider (e.g., Mistral, OpenAI, Ollama, etc.)"));
    QLineEdit *endpointEdit = new QLineEdit(&dialog);
    endpointEdit->setText("https://api.mistral.ai/v1/chat/completions");
    endpointEdit->setToolTip(tr("The full API URL endpoint for this provider"));
    QLineEdit *apiKeyEdit = new QLineEdit(&dialog);
    apiKeyEdit->setEchoMode(QLineEdit::Password);
    apiKeyEdit->setToolTip(tr("API key for authentication. MUST BE PROVIDED! (may not be needed for local models like Ollama)"));
    QLineEdit *modelNameEdit = new QLineEdit(&dialog);

    modelNameEdit->setText("mistral-medium");
    modelNameEdit->setToolTip(tr("The specific model to use (e.g., mistral-medium, gpt-4, llama3)"));
    QDoubleSpinBox *tempSpin = new QDoubleSpinBox(&dialog);
    tempSpin->setRange(0.0, 2.0);
    tempSpin->setSingleStep(0.1);
    tempSpin->setValue(0.7);
    tempSpin->setToolTip(tr("Controls randomness: lower values are more deterministic, higher values more creative\n"
                            "Keep 0.70 for best results"));
    QSpinBox *maxTokensSpin = new QSpinBox(&dialog);
    maxTokensSpin->setRange(1, 100000);
    maxTokensSpin->setValue(8192);
    maxTokensSpin->setToolTip(tr("Maximum number of tokens in the response\n"
                                 "Keep 8192 for Mistral"));
    // If editing, populate fields
    if (model) {
        nameEdit->setText(model->name);
        providerEdit->setText(model->provider);
        endpointEdit->setText(model->endpoint);
        apiKeyEdit->setText(model->apiKey);
        modelNameEdit->setText(model->modelName);
        tempSpin->setValue(model->temperature);
        maxTokensSpin->setValue(model->maxTokens);
    }

    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);

    QFormLayout *form = new QFormLayout(&dialog);
    form->addRow(tr("Friendly Name:"), nameEdit);
    form->addRow(tr("Provider:"), providerEdit);
    form->addRow(tr("Endpoint URL:"), endpointEdit);
    form->addRow(tr("API Key:"), apiKeyEdit);
    form->addRow(tr("Model Name:"), modelNameEdit);
    form->addRow(tr("Temperature:"), tempSpin);
    form->addRow(tr("Max Tokens:"), maxTokensSpin);
    form->addRow(buttonBox);

    connect(buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

    // Validate on accept
    connect(&dialog, &QDialog::accepted, [&]() {
        QString newName = nameEdit->text().trimmed();
        QString newProvider = providerEdit->text().trimmed();
        QString newEndpoint = endpointEdit->text().trimmed();
        QString newModelName = modelNameEdit->text().trimmed();

        // Check each required field
        if (newName.isEmpty()) {
            QMessageBox::warning(&dialog, tr("Invalid Friendly Name"), tr("Model friendly name cannot be empty."));
            dialog.reject();
            return;
        }

        // Check uniqueness
        if (modelNameExists(newName, model ? model->name : QString())) {
            QMessageBox::warning(&dialog, tr("Duplicate Friendly Name"), tr("A model with this friendly name already exists."));
            dialog.reject();
            return;
        }

        if (newProvider.isEmpty()) {
            QMessageBox::warning(&dialog, tr("Invalid Provider"), tr("Provider cannot be empty."));
            dialog.reject();
            return;
        }

        if (newEndpoint.isEmpty()) {
            QMessageBox::warning(&dialog, tr("Invalid Endpoint"), tr("Endpoint URL cannot be empty."));
            dialog.reject();
            return;
        }

        if (newModelName.isEmpty()) {
            QMessageBox::warning(&dialog, tr("Invalid Model Name"), tr("Model name cannot be empty."));
            dialog.reject();
            return;
        }

        // Create or update model
        Model updatedModel;
        updatedModel.name = newName;
        updatedModel.provider = providerEdit->text().trimmed();
        updatedModel.endpoint = endpointEdit->text().trimmed();
        updatedModel.apiKey = apiKeyEdit->text();
        updatedModel.modelName = modelNameEdit->text().trimmed();
        updatedModel.temperature = tempSpin->value();
        updatedModel.maxTokens = maxTokensSpin->value();

        if (model) {
            // Editing: replace existing
            *model = updatedModel;
            if (model->name == m_activeModel) {
                            emit activeModelChanged(m_activeModel);
                        }
        } else {
            // Adding: append to vector
            m_models.append(updatedModel);
        }

        saveModels();
        refreshModelList();
        emit modelsChanged();
    });

    dialog.exec();
}
