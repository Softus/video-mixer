/*
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

#include "detailsdialog.h"

#include <QHBoxLayout>
#include <QTreeWidget>

#define HAVE_CONFIG_H
#include <dcmtk/config/osconfig.h>   /* make sure OS specific configuration is included first */
#include <dcmtk/dcmdata/dcdatset.h>
#include <dcmtk/dcmdata/dcelem.h>
#include <dcmtk/dcmdata/dcsequen.h>
#include <dcmtk/dcmdata/dctag.h>

static void populateTree(DcmObject* parent, QTreeWidgetItem* parentItem)
{
    DcmObject* obj = nullptr;
    while (obj = parent->nextInContainer(obj), obj != nullptr)
    {
        DcmTag tag = obj->getTag();
        char *val = nullptr;
        if (obj->isaString())
        {
            dynamic_cast<DcmElement*>(obj)->getString(val);
        }
        QStringList text;
        text << QString::fromUtf8(tag.getTagName())
             << QString::fromUtf8(tag.toString().c_str())
             << QString::fromUtf8(tag.getVRName())
             << QString::fromUtf8(val)
             ;
        auto item = new QTreeWidgetItem(text);
        parentItem->addChild(item);

        if (!obj->isLeaf())
        {
            populateTree(obj, item);
            item->setExpanded(true);
        }
    }
}

DetailsDialog::DetailsDialog(DcmDataset* ds, QWidget *parent) :
    QDialog(parent)
{
    setWindowTitle(tr("Details"));
    setMinimumSize(720, 480);

    QTreeWidget* tree = new QTreeWidget();
    QStringList hdr;
    hdr << tr("Tag name") << tr("Code") << tr("VR") << tr("Value");
    tree->setColumnCount(hdr.size());
    tree->setHeaderLabels(hdr);

    populateTree(ds, tree->invisibleRootItem());
    for (int i = 0; i < hdr.size(); ++i)
    {
        tree->resizeColumnToContents(i);
    }

    auto layout = new QVBoxLayout();
    layout->addWidget(tree);
    setLayout(layout);
}
