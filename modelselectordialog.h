#ifndef MODELSELECTORDIALOG_H
#define MODELSELECTORDIALOG_H

#include "model.h"
#include <QDialog>
#include <QObject>
#include <QDialog>
#include <QVector>
#include<QListWidget>
#include<QListWidgetItem>
#include<QPushButton>



class ModelSelectorDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ModelSelectorDialog(QWidget *parent = nullptr);
    ~ModelSelectorDialog();

    // Returns the list of all models
    QVector<Model> models() const;

    // Returns the name of the currently active model
    QString activeModel() const;

signals:
    // Emitted when the list of models changes (add, edit, delete)
    void modelsChanged();

    // Emitted when the active model changes
    void activeModelChanged(const QString &name);

private slots:
    void onAddClicked();
    void onEditClicked();
    void onDeleteClicked();
    void onSetActiveClicked();
    void onItemDoubleClicked(QListWidgetItem *item);
    void refreshModelList();

private:
    void loadModels();
    void saveModels();
    void setActiveModel(const QString &name);
    bool modelNameExists(const QString &name, const QString &ignoreName = QString()) const;
    void showEditDialog(Model *model = nullptr); // nullptr means add new

    QVector<Model> m_models;
    QString m_activeModel;
    QListWidget *m_listWidget;
    QPushButton *m_editButton;
    QPushButton *m_deleteButton;
    QPushButton *m_setActiveButton;
};
#endif // MODELSELECTORDIALOG_H
