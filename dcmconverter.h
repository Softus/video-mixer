#ifndef DCMCONVERTER_H
#define DCMCONVERTER_H

#include <QString>

#define HAVE_CONFIG_H
#include <dcmtk/config/osconfig.h> /* make sure OS specific configuration is included first */
#include <dcmtk/ofstd/ofcond.h>    /* for class OFCondition */
#include <dcmtk/dcmdata/dcxfer.h>

class DcmDataset;

OFCondition
readAndInsertPixelData
    ( const QString& fileName
    , DcmDataset* dset
    , E_TransferSyntax& outputTS
    );

#endif // DCMCONVERTER_H
