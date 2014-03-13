/*
 * Copyright (C) 2013-2014 Irkutsk Diagnostic Center.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; version 2.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "storagesettings.h"
#include "../qwaitcursor.h"
#include "../defaults.h"

#include <QCheckBox>
#include <QFileDialog>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QSettings>
#include <QSpinBox>
#include <QxtLineEdit>

#include <QGst/ElementFactory>

StorageSettings::StorageSettings(QWidget *parent)
    : QWidget(parent)
    , checkMaxVideoSize(nullptr)
    , spinMaxVideoSize(nullptr)
{
    QSettings settings;
    auto layoutMain = new QVBoxLayout;
    auto layoutImages = new QFormLayout;

    auto grpImages = new QGroupBox(tr("Images and clips"));
    auto layoutPath = new QHBoxLayout;
    textOutputPath = new QLineEdit(settings.value("output-path", DEFAULT_OUTPUT_PATH).toString());
    auto browseButton = new QPushButton(tr("Browse").append(QString(0x2026)));
    connect(browseButton, SIGNAL(clicked()), this, SLOT(onClickBrowse()));
    auto lblPath = new QLabel(tr("&Output path"));
    lblPath->setBuddy(textOutputPath);
    layoutPath->addWidget(lblPath);
    layoutPath->addWidget(textOutputPath, 1);
    layoutPath->addWidget(browseButton);
    layoutImages->addRow(layoutPath);

    textFolderTemplate = new QLineEdit(settings.value("folder-template", DEFAULT_FOLDER_TEMPLATE).toString());
    layoutImages->addRow(tr("&Folder template"), textFolderTemplate);

    layoutImages->addRow(tr("&Pictures template"), textImageTemplate = new QLineEdit(settings.value("image-template", DEFAULT_IMAGE_TEMPLATE).toString()));
    layoutImages->addRow(tr("&Clips template"), textClipTemplate = new QLineEdit(settings.value("clip-template", DEFAULT_CLIP_TEMPLATE).toString()));

    grpImages->setLayout(layoutImages);
    layoutMain->addWidget(grpImages);

    auto grpVideo = new QGroupBox(tr("Video logs"));
    auto layoutVideoLog = new QFormLayout;

    auto layoutVideoPath = new QHBoxLayout;
    textVideoOutputPath = new QxtLineEdit(settings.value("video-output-path").toString());
    textVideoOutputPath->setSampleText(tr("(share with images and clips)"));
    auto browseVideoButton = new QPushButton(tr("Browse").append(QString(0x2026)));
    connect(browseVideoButton, SIGNAL(clicked()), this, SLOT(onClickVideoBrowse()));
    auto lblVideoPath = new QLabel(tr("O&utput path"));
    lblVideoPath->setBuddy(textVideoOutputPath);
    layoutVideoPath->addWidget(lblVideoPath);
    layoutVideoPath->addWidget(textVideoOutputPath, 1);
    layoutVideoPath->addWidget(browseVideoButton);
    layoutVideoLog->addRow(layoutVideoPath);
    textVideoFolderTemplate = new QxtLineEdit(settings.value("video-folder-template").toString());
    textVideoFolderTemplate->setSampleText(tr("(share with images and clips)"));
    layoutVideoLog->addRow(tr("Fol&der template"), textVideoFolderTemplate);
    layoutVideoLog->addRow(tr("&Video template"), textVideoTemplate = new QLineEdit(settings.value("video-template", DEFAULT_VIDEO_TEMPLATE).toString()));

    auto elm = QGst::ElementFactory::make("multifilesink");
    if (elm && elm->findProperty("max-file-size"))
    {
        layoutVideoLog->addRow(checkMaxVideoSize = new QCheckBox(tr("&Split files by")), spinMaxVideoSize = new QSpinBox);
        connect(checkMaxVideoSize, SIGNAL(toggled(bool)), spinMaxVideoSize, SLOT(setEnabled(bool)));
        checkMaxVideoSize->setChecked(settings.value("split-video-files", DEFAULT_SPLIT_VIDEO_FILES).toBool());

        spinMaxVideoSize->setSuffix(tr(" Mb"));
        spinMaxVideoSize->setRange(1, 1024*1024);
        spinMaxVideoSize->setValue(settings.value("video-max-file-size", DEFAULT_VIDEO_MAX_FILE_SIZE).toInt());
        spinMaxVideoSize->setEnabled(checkMaxVideoSize->isChecked());
    }
    grpVideo->setLayout(layoutVideoLog);

    layoutMain->addWidget(grpVideo);

    auto grpLegend = new QGroupBox(tr("Substitutes"));
    auto layoutLegend = new QVBoxLayout;
    layoutLegend->addWidget(new QLabel(tr("%yyyy%\t\tyear\t\t\t%mm%\t\tmonth\n"
                                        "%dd%\t\tday\t\t\t%hh%\t\thour\n"
                                        "%min%\t\tminute\t\t\t%nn%\t\tsequential number\n"
                                        "%id%\t\tpatient id\t\t%name%\tpatient name\n"
                                        "%sex%\t\tpatient sex\t\t%birthdate%\tpatient birthdate\n"
                                        "%physician%\tphysician name\t\t%study%\tstudy name"
                                     )));
    grpLegend->setLayout(layoutLegend);
    layoutMain->addWidget(grpLegend);
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
        textOutputPath->setText(dlg.selectedFiles().first());
    }
}

void StorageSettings::onClickVideoBrowse()
{
    QWaitCursor wait(this);
    QFileDialog dlg(this);
    dlg.setFileMode(QFileDialog::DirectoryOnly);
    dlg.selectFile(textVideoOutputPath->text());
    if (dlg.exec())
    {
        textVideoOutputPath->setText(dlg.selectedFiles().first());
    }
}

void StorageSettings::save()
{
    QSettings settings;
    settings.setValue("output-path", textOutputPath->text());
    settings.setValue("video-output-path", textVideoOutputPath->text());
    settings.setValue("folder-template", textFolderTemplate->text());
    settings.setValue("video-folder-template", textVideoFolderTemplate->text());
    settings.setValue("image-template", textImageTemplate->text());
    settings.setValue("clip-template", textClipTemplate->text());
    settings.setValue("video-template", textVideoTemplate->text());
    if (spinMaxVideoSize)
    {
        settings.setValue("split-video-files", checkMaxVideoSize->isChecked());
        settings.setValue("video-max-file-size", spinMaxVideoSize->value());
    }
}
