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

#include "storage.h"
#include "../qwaitcursor.h"
#include "../defaults.h"

#include <QCheckBox>
#include <QFileDialog>
#include <QFormLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QPushButton>
#include <QSettings>
#include <QSpinBox>
#include <QVBoxLayout>
#include <QxtLineEdit>

StorageSettings::StorageSettings(QWidget *parent)
    : QWidget(parent)
{
    QSettings settings;
    settings.beginGroup("storage");
    auto layoutMain = new QVBoxLayout;
    auto layoutImages = new QFormLayout;

    auto grpImages = new QGroupBox(tr("Images and clips"));

    layoutImages->addRow(tr("&Output path"), textOutputPath = new QxtLineEdit(settings.value("output-path", DEFAULT_OUTPUT_PATH).toString()));
    textOutputPath->setButtonIcon(QIcon(":/buttons/folder"));
    textOutputPath->setButtonPosition(QxtLineEdit::OuterRight);
    textOutputPath->setResetButtonMode(QxtLineEdit::ShowResetNotEmpty);
    connect(textOutputPath, SIGNAL(buttonClicked()), this, SLOT(onClickBrowse()));

    textFolderTemplate = new QxtLineEdit(settings.value("folder-template", DEFAULT_FOLDER_TEMPLATE).toString());
    layoutImages->addRow(tr("&Folder template"), textFolderTemplate);

    layoutImages->addRow(tr("&Pictures template"), textImageTemplate = new QLineEdit(settings.value("image-template", DEFAULT_IMAGE_TEMPLATE).toString()));
    layoutImages->addRow(tr("&Clips template"), textClipTemplate = new QLineEdit(settings.value("clip-template", DEFAULT_CLIP_TEMPLATE).toString()));

    grpImages->setLayout(layoutImages);
    layoutMain->addWidget(grpImages);

    auto grpVideo = new QGroupBox(tr("Video logs"));
    auto layoutVideoLog = new QFormLayout;

    layoutVideoLog->addRow(tr("O&utput path"), textVideoOutputPath = new QxtLineEdit(settings.value("video-output-path").toString()));
    textVideoOutputPath->setSampleText(tr("(share with images and clips)"));
    textVideoOutputPath->setButtonIcon(QIcon(":/buttons/folder"));
    textVideoOutputPath->setButtonPosition(QxtLineEdit::OuterRight);
    textVideoOutputPath->setResetButtonMode(QxtLineEdit::ShowResetNotEmpty);
    connect(textVideoOutputPath, SIGNAL(buttonClicked()), this, SLOT(onClickVideoBrowse()));

    textVideoFolderTemplate = new QxtLineEdit(settings.value("video-folder-template").toString());
    textVideoFolderTemplate->setSampleText(tr("(share with images and clips)"));
    layoutVideoLog->addRow(tr("Fol&der template"), textVideoFolderTemplate);
    layoutVideoLog->addRow(tr("&Video template"), textVideoTemplate = new QLineEdit(settings.value("video-template", DEFAULT_VIDEO_TEMPLATE).toString()));
    grpVideo->setLayout(layoutVideoLog);

    layoutMain->addWidget(grpVideo);

    auto grpLegend = new QGroupBox(tr("Substitutes"));
    auto layoutLegend = new QGridLayout;
    auto str = tr("%yyyy%|year|%mm%|month|%dd%|day|%hh%|hour|%min%|minute|"
                  "%nn%|sequential number|%id%|patient id|%name%|patient name|%sex%|patient sex|"
                  "%birthdate%|patient birthdate|%physician%|physician name|%study%|study name");

    int cnt = 0;
    Q_FOREACH(auto lbl, str.split('|'))
    {
        layoutLegend->addWidget(new QLabel(lbl), cnt / 4, cnt % 4);
        ++cnt;
    }

    grpLegend->setLayout(layoutLegend);
    layoutMain->addWidget(grpLegend);

    layoutMain->addStretch(1); // For really huge displays we need this

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

void StorageSettings::save(QSettings& settings)
{
    settings.beginGroup("storage");
    settings.setValue("output-path", textOutputPath->text());
    settings.setValue("video-output-path", textVideoOutputPath->text());
    settings.setValue("folder-template", textFolderTemplate->text());
    settings.setValue("video-folder-template", textVideoFolderTemplate->text());
    settings.setValue("image-template", textImageTemplate->text());
    settings.setValue("clip-template", textClipTemplate->text());
    settings.setValue("video-template", textVideoTemplate->text());
    settings.endGroup();
}
