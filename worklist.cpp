#include "worklist.h"
#include "detailsdialog.h"
#include "dcmclient.h"

#include <QAction>
#include <QApplication>
#include <QBoxLayout>
#include <QDateTime>
#include <QDebug>
#include <QHeaderView>
#include <QMessageBox>
#include <QPushButton>
#include <QSettings>
#include <QTableWidget>
#include <QTimer>
#include <QToolBar>
#include "qwaitcursor.h"

#include <dcmtk/dcmdata/dcdatset.h>
#include <dcmtk/dcmdata/dcdeftag.h>
#include <dcmtk/dcmdata/dcitem.h>
#include <dcmtk/dcmdata/dcuid.h>

Q_DECLARE_METATYPE(DcmDataset)
static int DcmDatasetMetaType = qRegisterMetaType<DcmDataset>();

static inline int next(const QString& str, int idx)
{
    return str.length() > ++idx? str[idx].toLower().unicode(): 0;
}

QString transcyr(const QString& str)
{
    if (str.isNull())
    {
        return str;
    }

    QString ret;

    for (int i = 0; i < str.length(); ++i)
    {
        switch (str[i].unicode())
        {
        case 'A':
            ret.append(L'А');
            break;
        case 'B':
            ret.append(L'Б');
            break;
        case 'V':
            ret.append(L'В');
            break;
        case 'G':
            ret.append(L'Г');
            break;
        case 'D':
            ret.append(L'Д');
            break;
        case 'E':
            ret.append(i==0?L'Э':L'Е'); // В начале слова русская 'е' это 'ye'
            break;
        case 'Z':
            ret.append(next(str, i) == 'h'? ++i, L'Ж': L'З');
            break;
        case 'I':
            ret.append(L'И');
            break;
        case 'K':
            ret.append(next(str, i) == 'h'? ++i, L'Х': L'К');
            break;
        case 'L':
            ret.append(L'Л');
            break;
        case 'M':
            ret.append(L'М');
            break;
        case 'N':
            ret.append(L'Н');
            break;
        case 'O':
            ret.append(L'О');
            break;
        case 'P':
            ret.append(L'П');
            break;
        case 'R':
            ret.append(L'Р');
            break;
        case 'S':
            if (next(str, i) == 'h')
            {
                ++i;
                if (next(str, i) == 'c' && next(str, i + 1) == 'h')
                {
                    i+=2;
                    ret.append(L'Щ');
                }
                else
                {
                    ret.append(L'Ш');
                }
            }
            else
            {
                ret.append(L'С');
            }
            break;
        case 'T':
            ret.append(next(str, i) == 's'? ++i, L'Ц': L'Т');
            break;
        case 'U':
            ret.append(L'У');
            break;
        case 'F':
            ret.append(L'Ф');
            break;
        case 'X':
            ret.append(L'К').append(i < str.length() - 1 && str[i+1].isLower()? L'с': L'С');
            break;
        case 'C':
            ret.append(next(str, i) == 'h'? ++i, L'Ч': L'К'); // Английская 'c' без 'h' не должна встрачаться.
            break;
        case 'Y':
            switch (next(str, i)) {
            case 'e':
                ++i, ret.append(L'Е');
                break;
            case 'o':
                ++i, ret.append(L'Ё');
                break;
            case 'u':
                ++i, ret.append(L'Ю');
                break;
            case 'i':
                ret.append(L'Ь'); // Только для мягкого знака перед 'и', как Ilyin.
                break;
            case 'a':
                ++i, ret.append(L'Я');
                break;
            case '\x0':
            case ' ':
            case '.':
                ret.append(L'И').append(L'Й'); // Фамилии на ый заканчиваются редко
                break;
            default:
                ret.append(L'Ы');
                break;
            }
            break;

        case 'a':
            ret.append(L'а');
            break;
        case 'b':
            ret.append(L'б');
            break;
        case 'v':
            ret.append(L'в');
            break;
        case 'g':
            ret.append(L'г');
            break;
        case 'd':
            ret.append(L'д');
            break;
        case 'e':
            ret.append(i==0?L'э':L'е'); // В начале слова русская 'е' это 'ye'
            break;
        case 'z':
            ret.append(next(str, i) == 'h'? ++i, L'ж': L'з');
            break;
        case 'i':
            ret.append(L'и');
            break;
        case 'k':
            ret.append(next(str, i) == 'h'? ++i, L'х': L'к');
            break;
        case 'l':
            ret.append(L'л');
            break;
        case 'm':
            ret.append(L'м');
            break;
        case 'n':
            ret.append(L'н');
            break;
        case 'o':
            ret.append(L'о');
            break;
        case 'p':
            ret.append(L'п');
            break;
        case 'r':
            ret.append(L'р');
            break;
        case 's':
            if (next(str, i) == 'h')
            {
                ++i;
                if (next(str, i) == 'c' && next(str, i + 1) == 'h')
                {
                    i+=2;
                    ret.append(L'щ');
                }
                else
                {
                    ret.append(L'ш');
                }
            }
            else
            {
                ret.append(L'с');
            }
            break;
        case 't':
            ret.append(next(str, i) == 's'? ++i, L'ц': L'т');
            break;
        case 'u':
            ret.append(L'у');
            break;
        case 'f':
            ret.append(L'ф');
            break;
        case 'x':
            ret.append(L'к').append(L'с');
            break;
        case 'с':
            ret.append(next(str, i) == 'h'? ++i, L'ч': L'к'); // Английская 'c' без 'h' не должна встрачаться.
            break;
        case 'y':
            switch (next(str, i)) {
            case 'e':
                ++i, ret.append(L'е');
                break;
            case 'o':
                ++i, ret.append(L'ё');
                break;
            case 'u':
                ++i, ret.append(L'ю');
                break;
            case 'i':
                ret.append(L'ь'); // Только для мягкого знака перед 'и', как Ilyin.
                break;
            case 'a':
                ++i, ret.append(L'я');
                break;
            case '\x0':
            case ' ':
            case '.':
                ret.append(L'и').append(L'й'); // Фамилии на ый заканчиваются редко
                break;
            default:
                ret.append(L'ы');
                break;
            }
            break;

        default:
            ret.append(str[i]);
            break;
        }
    }

    return ret;
}

Worklist::Worklist(QWidget *parent) :
    QWidget(parent),
    timeColumn(-1),
    dateColumn(-1),
    activeConnection(nullptr)
{
    setWindowTitle(tr("Worklist"));

    QSettings settings;
    auto translateCyrillic = settings.value("translate-cyrillic", true).toBool();
    auto cols = settings.value("worklist-columns").toStringList();
    if (cols.size() == 0)
    {
        // Defaults are id, name, bithday, sex, procedure description, date, time
        cols << "0010,0020" << "0010,0010" << "0010,0030" << "0010,0040" << "0040,0007" << "0040,0002" << "0040,0003";
    }

    table = new QTableWidget(0, cols.size());
    connect(table, SIGNAL(itemDoubleClicked(QTableWidgetItem*)), this, SLOT(onCellDoubleClicked(QTableWidgetItem*)));

    for (auto i = 0; i < cols.size(); ++i)
    {
        DcmTag tag;
        auto text = DcmTag::findTagFromName(cols[i].toUtf8(), tag).good()? QString::fromUtf8(tag.getTagName()): cols[i];
        auto item = new QTableWidgetItem(text);
        item->setData(Qt::UserRole, (tag.getGroup() << 16) | tag.getElement());
        table->setHorizontalHeaderItem(i, item);

        if (tag == DCM_ScheduledProcedureStepStartDate)
        {
            dateColumn = i;
        }
        else if (tag == DCM_ScheduledProcedureStepStartTime)
        {
            timeColumn = i;
        }
        else if (translateCyrillic && (tag == DCM_PatientName || tag == DCM_ScheduledPerformingPhysicianName))
        {
            translateColumns.append(i);
        }
    }
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);

    auto layoutMain = new QVBoxLayout();
    layoutMain->addWidget(createToolBar());
    layoutMain->addWidget(table);
    setLayout(layoutMain);

    setMinimumSize(640, 480);

    restoreGeometry(settings.value("worklist-geometry").toByteArray());
    setWindowState((Qt::WindowState)settings.value("worklist-state").toInt());
    table->horizontalHeader()->restoreState(settings.value("worklist-columns-width").toByteArray());

    // Start loading of worklist right after the window is shown for the first time
    //
    QTimer::singleShot(0, this, SLOT(onLoadClick()));
    setAttribute(Qt::WA_DeleteOnClose, false);
}

QToolBar* Worklist::createToolBar()
{
    QToolBar* bar = new QToolBar(tr("Worklist"));
    bar->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);

    actionLoad   = bar->addAction(QIcon(":/buttons/refresh"), tr("&Refresh"), this, SLOT(onLoadClick()));
    actionDetail = bar->addAction(QIcon(":/buttons/details"), tr("&Details"), this, SLOT(onShowDetailsClick()));
    actionDetail->setEnabled(false);
    actionStartStudy = bar->addAction(QIcon(":/buttons/start"), tr("Start &study"), this, SLOT(onStartStudyClick()));
    actionStartStudy->setEnabled(false);

    return bar;
}

void Worklist::onAddRow(DcmDataset* dset)
{
    QDateTime date;
    int row = table->rowCount();
    table->setRowCount(row + 1);

    for (int col = 0; col < table->columnCount(); ++col)
    {
        const char *str = nullptr;
        auto tag = table->horizontalHeaderItem(col)->data(Qt::UserRole).toInt();
        DcmTagKey tagKey(tag >> 16, tag & 0xFFFF);
        OFCondition cond = dset->findAndGetString(tagKey, str, true);
        auto text = QString::fromUtf8(str? str: cond.text());

        if (str && translateColumns.contains(col))
        {
            text = transcyr(text);
        }

        auto item = new QTableWidgetItem(text);
        table->setItem(row, col, item);
        if (col == dateColumn)
        {
            date.setDate(QDate::fromString(text, "yyyyMMdd"));
        }
        else if (col == timeColumn)
        {
            date.setTime(QTime::fromString(text, "HHmm"));
        }
    }

    table->item(row, 0)->setData(Qt::UserRole, QVariant::fromValue(*(DcmDataset*)dset->clone()));

    if (date < QDateTime::currentDateTime() && date > maxDate)
    {
       maxDate = date;
       table->selectRow(row);
    }

    qApp->processEvents();
}

void Worklist::closeEvent(QCloseEvent *evt)
{
    // Force drop connection to the server if the worklist still loading
    //
    if (activeConnection)
    {
        activeConnection->abort();
        activeConnection = nullptr;
    }
    QSettings settings;
    settings.setValue("worklist-geometry", saveGeometry());
    settings.setValue("worklist-state", (int)windowState() & ~Qt::WindowMinimized);
    settings.setValue("worklist-columns-width", table->horizontalHeader()->saveState());
    QWidget::closeEvent(evt);
}

void Worklist::onLoadClick()
{
    QWaitCursor wait(this);
    actionLoad->setEnabled(false);
    actionDetail->setEnabled(false);

    // Clear all data
    //
    table->setUpdatesEnabled(false);
    table->setSortingEnabled(false);
    maxDate.setMSecsSinceEpoch(0);
    table->setRowCount(0);

    DcmClient assoc(UID_FINDModalityWorklistInformationModel);

    activeConnection = &assoc;
    connect(&assoc, SIGNAL(addRow(DcmDataset*)), this, SLOT(onAddRow(DcmDataset*)));
    bool ok = assoc.findSCU();
    activeConnection = nullptr;

    if (!ok)
    {
        qCritical() << assoc.lastError();
        QMessageBox::critical(this, windowTitle(), assoc.lastError(), QMessageBox::Ok);
    }

    if (timeColumn >= 0)
    {
        table->sortItems(timeColumn);
        if (dateColumn >= 0)
        {
            table->sortItems(dateColumn);
        }
    }

    actionDetail->setEnabled(table->rowCount() > 0);
    actionStartStudy->setEnabled(table->rowCount() > 0);

    table->setSortingEnabled(true);
    table->scrollToItem(table->currentItem());
    table->setFocus();
    table->setUpdatesEnabled(true);

    actionLoad->setEnabled(true);
}

void Worklist::onShowDetailsClick()
{
    QWaitCursor wait(this);
    int row = table->currentRow();
    auto ds = table->item(row, 0)->data(Qt::UserRole).value<DcmDataset>();
    DetailsDialog dlg(&ds, this);
    dlg.exec();
}

void Worklist::onCellDoubleClicked(QTableWidgetItem* item)
{
    table->setCurrentItem(item);
    //onShowDetailsClick();
    onStartStudyClick();
}

void Worklist::onStartStudyClick()
{
    int row = table->currentRow();
    if (row >= 0)
    {
        auto ds = table->item(row, 0)->data(Qt::UserRole).value<DcmDataset>();
        startStudy(&ds);
    }
}
