#include "mandatoryfieldssettings.h"
#include <QBoxLayout>
#include <QGroupBox>
#include <QListWidget>
#include <QPushButton>
#include <QSettings>

MandatoryFieldsSettings::MandatoryFieldsSettings(QWidget *parent) :
    QWidget(parent)
{
    QSettings settings;
    auto fieldLabels = QStringList() << tr("Patient ID") << tr("Name") << tr("Sex") << tr("Birthday") << tr("Physician") << tr("Study type");
    auto fieldNames = QStringList() << "PatientID" << "Name" << "Sex" << "Birthday" << "Physician" << "StudyType";
    auto listMandatory = settings.value("new-study-mandatory-fields", QStringList() << "PatientID" << "Name").toStringList();

    auto layoutMain = new QVBoxLayout;
    layoutMain->setContentsMargins(4,0,4,0);

    listFields = new QListWidget();
    listFields->addItems(fieldLabels);
    for (auto i = 0; i < listFields->count(); ++i)
    {
        auto item = listFields->item(i);
        item->setCheckState(listMandatory.contains(fieldNames[i])? Qt::Checked: Qt::Unchecked);
        item->setData(Qt::UserRole, fieldNames[i]);
    }

    layoutMain->addWidget(listFields);
    setLayout(layoutMain);
}

void MandatoryFieldsSettings::save()
{
    QSettings settings;
    QStringList fields;

    for (auto i = 0; i < listFields->count(); ++i)
    {
        auto item = listFields->item(i);
        if (item->checkState() != Qt::Checked)
        {
            continue;
        }
        fields.append(item->data(Qt::UserRole).toString());
    }

    settings.setValue("new-study-mandatory-fields", fields);
}
