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

#ifndef SETTINGS_H
#define SETTINGS_H

#include <QDialog>
#include <QSettings>

QT_BEGIN_NAMESPACE
class QListWidget;
class QListWidgetItem;
class QPushButton;
class QStackedWidget;
QT_END_NAMESPACE

class Settings : public QDialog
{
    Q_OBJECT
    QPushButton *btnCancel;
public:
    Settings(const QString &page, QWidget *parent = 0, Qt::WindowFlags flags = 0);
signals:
    void save(QSettings& settings);
    void apply();

public slots:
    void resetSettings();

private slots:
    void saveToFile();
    void loadFromFile();
    void launchEditor();
    void onClickApply();
    void changePage(QListWidgetItem *current, QListWidgetItem *previous);

    virtual void accept();

private:
    void createPages();
    void recreatePages();
    void createPage(const QString& title, const QMetaObject& page);

    QListWidget *listWidget;
    QStackedWidget *pagesWidget;
};

#endif
