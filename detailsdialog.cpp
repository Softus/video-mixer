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
             << QString().sprintf("(%04X:%04X)", tag.getGTag(), tag.getETag())
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
