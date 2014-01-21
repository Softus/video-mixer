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

#ifndef MOUSESHORTCUT_H
#define MOUSESHORTCUT_H

#include <QObject>
#include <QKeySequence>

QT_BEGIN_NAMESPACE
class QEvent;
class QAbstractButton;
class QAction;
QT_END_NAMESPACE

class MouseShortcut : public QObject
{
    Q_OBJECT
    int m_key;

public:
    static void removeMouseShortcut(QObject *parent);
    static QString toString(int key, QKeySequence::SequenceFormat format = QKeySequence::PortableText);
    QString toString(QKeySequence::SequenceFormat format = QKeySequence::PortableText) const;
    MouseShortcut(int key, QAbstractButton *parent);
    MouseShortcut(int key, QAction *parent);
    ~MouseShortcut();

protected:
    bool eventFilter(QObject *o, QEvent *e);
};

#endif // MOUSESHORTCUT_H
