#include "storagesettings.h"
#include <QHBoxLayout>
#include <QFileDialog>
#include <QFormLayout>
#include <QFrame>
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

    QFrame *frameFolder = new QFrame;
    frameFolder->setFrameShape(QFrame::Box);
    frameFolder->setFrameShadow(QFrame::Sunken);
    QFormLayout* layoutFolder = new QFormLayout;
    textFolderTemplate = new QLineEdit(settings.value("folder-template", "/%yyyy%-%MM%/%dd%/%name%/").toString());
    layoutFolder->addRow(tr("&Folder template"), textFolderTemplate);
    frameFolder->setLayout(layoutFolder);
    layoutMain->addRow(frameFolder);

    QFrame *frameFile = new QFrame;
    frameFile->setFrameShape(QFrame::Box);
    frameFile->setFrameShadow(QFrame::Sunken);
    QFormLayout* fileLayout = new QFormLayout;
    fileLayout->addRow(tr("&Pictures template"), textImageTemplate = new QLineEdit(settings.value("image-file", "image-%study%-%nn%").toString()));
    fileLayout->addRow(tr("&Clips template"), textClipTemplate = new QLineEdit(settings.value("clip-file", "clip-%study%-%nn%").toString()));
    fileLayout->addRow(tr("&Video template"), textVideoTemplate = new QLineEdit(settings.value("video-file", "video-%study%").toString()));
    frameFile->setLayout(fileLayout);
    layoutMain->addRow(frameFile);
    layoutMain->addRow(new QLabel(tr("%yyyy%\t\tyear\n%MM%\t\tmonth\n%dd%\t\tday\n%hh%\t\thour\n%mm%\t\tminute\n"
                                     "%name%\tpatient name, if specified\n%study%\tstudy name\n%nn%\t\tsequential number")));

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

void StorageSettings::save()
{
    QSettings settings;
    settings.setValue("output-path", textOutputPath->text());
    settings.setValue("folder-template", textFolderTemplate->text());
    settings.setValue("image-template", textImageTemplate->text());
    settings.setValue("clip-template", textClipTemplate->text());
    settings.setValue("video-template", textVideoTemplate->text());
}
