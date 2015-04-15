/*
 * Copyright (C) 2012 Waruna Tennakoon
 * Copyright (C) 2013-2015 Irkutsk Diagnostic Center.
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

#ifndef HOTKEYEDIT_H
#define HOTKEYEDIT_H

#include <QxtLineEdit>

class HotKeyEdit : public QxtLineEdit
{
    Q_OBJECT

public:
    explicit HotKeyEdit(QWidget *parent = 0);

    //
    //   0 => not set
    // > 0 => keyboard shortcut, compatible with QKeySequence
    // < 0 => mouse shortcut
    //
    int key() const;

public slots:
    void setKey(int key);
signals:
    void keyChanged(int key);

protected:
    virtual void focusInEvent(QFocusEvent *evt);
    virtual void focusOutEvent(QFocusEvent *evt);
    virtual bool event(QEvent *evt);
private:
    int    m_key;
    qint64 m_ts;
    bool   m_ignoreNextMouseEvent;
    int    m_stickyKey;
    void   handleMouseReleaseEvent(QMouseEvent *evt);
    void   handleKeyReleaseEvent(QKeyEvent *evt);
    void   updateText();
};

#endif // HOTKEYEDIT_H
