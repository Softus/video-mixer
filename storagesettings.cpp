#include "storagesettings.h"
#include <QHBoxLayout>
#include <QFileDialog>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSettings>
#include "qwaitcursor.h"

static const ushort trippleDot = 0x2026;

StorageSettings::StorageSettings(QWidget *parent) :
    QWidget(parent)
{
    QSettings settings;
    QFormLayout* layoutMain = new QFormLayout;

    QHBoxLayout* layoutPath = new QHBoxLayout;
    textOutputPath = new QLineEdit(settings.value("output-path", "/video").toString());
    QPushButton* browseButton = new QPushButton(QString::fromUtf16(&trippleDot, 1));
    browseButton->setMaximumWidth(32);
    connect(browseButton, SIGNAL(clicked()), this, SLOT(onClickBrowse()));
    QLabel* lblPath = new QLabel(tr("&Output path"));
    lblPath->setBuddy(textOutputPath);
    layoutPath->addWidget(lblPath);
    layoutPath->addWidget(textOutputPath, 1);
    layoutPath->addWidget(browseButton);
    layoutMain->addRow(layoutPath);

    QGroupBox *grpFolder = new QGroupBox;
    grpFolder->setStyleSheet("QGroupBox{ border:2px solid silver; }");
    QFormLayout* layoutFolder = new QFormLayout;
    QLineEdit* textFolderTemplate = new QLineEdit(settings.value("folder-template", "%yyyy%/%mm%/%dd%/").toString());
    layoutFolder->addRow(tr("&Folder template"), textFolderTemplate);
    grpFolder->setLayout(layoutFolder);
    layoutMain->addRow(grpFolder);

    QLineEdit* textImageTemplate;
    QLineEdit* textClipTemplate;
    QLineEdit* textVideoTemplate;
    QGroupBox *fileGroup = new QGroupBox;
    fileGroup->setStyleSheet("QGroupBox{ border:2px solid silver; }");
    QFormLayout* fileLayout = new QFormLayout;
    fileLayout->addRow(tr("&Picture template"), textImageTemplate = new QLineEdit(settings.value("image-file", "image-%name%-%study%").toString()));
    fileLayout->addRow(tr("&Clip template"), textClipTemplate = new QLineEdit(settings.value("clip-file", "clip-%name%-%study%").toString()));
    fileLayout->addRow(tr("&Video template"), textVideoTemplate = new QLineEdit(settings.value("video-file", "video-%name%-%study%").toString()));
    fileGroup->setLayout(fileLayout);
    layoutMain->addRow(fileGroup);
    layoutMain->addRow(new QLabel(tr("%yyyy%\t\tyear\n%mm%\t\tmonth\n%dd%\t\tday\n%hh%\t\thour\n%MM%\t\tminute\n"
                                     "%name%\tpatient name, if specified\n%study%\tstudy name")));

    setLayout(layoutMain);
}


void StorageSettings::onClickBrowse()
{
    QWaitCursor wait(this);
    QFileDialog dlg(this);
    dlg.setFileMode(QFileDialog::DirectoryOnly);
    dlg.selectFile(textOutputPath->text());
    if (dlg.exec())
    {
        textOutputPath->setText(dlg.selectedFiles().at(0));
    }
}
