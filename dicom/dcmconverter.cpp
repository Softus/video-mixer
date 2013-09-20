/*
 * Copyright (C) 2013 Irkutsk Diagnostic Center.
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

#include "product.h"
#include "dcmconverter.h"

#include <QDebug>
#include <QFile>
#include <QSettings>

#include <dcmtk/dcmdata/dcdeftag.h>
#include <dcmtk/dcmdata/dcfilefo.h>
#include <dcmtk/dcmdata/dcuid.h>

#include <dcmtk/dcmdata/dcpixel.h>   /* for DcmPixelData */
#include <dcmtk/dcmdata/dcpixseq.h>  /* for DcmPixelSequence */
#include <dcmtk/dcmdata/dcpxitem.h>  /* for DcmPixelItem */

#ifdef DCDEFINE_H
#define OFFIS_DCMTK_VER 0x030601
#else
#define OFFIS_DCMTK_VER 0x030600
#endif

#if defined(UNICODE) || defined (_UNICODE)
#include <MediaInfo/MediaInfo.h>
#else
#define UNICODE
#include <MediaInfo/MediaInfo.h>
#undef UNICODE
#endif

class DcmConverter
{
    MediaInfoLib::MediaInfo mi;
    MediaInfoLib::stream_t  type;

    QString getStr(const MediaInfoLib::String &parameter)
    {
        return QString::fromStdWString(mi.Get(type, 0, parameter));
    }

    Uint16 getUint16(const MediaInfoLib::String &parameter)
    {
        return getStr(parameter).toUShort();
    }

public:
    DcmConverter()
        : type(MediaInfoLib::Stream_General)
    {
    }

    OFCondition readPixelData
        ( DcmDataset* dset
        , uchar*  pixData
        , qint64 length
        , E_TransferSyntax& ts
        )
    {
        OFCondition cond;

        if (!mi.Open(pixData, length))
        {
            return makeOFCondition(0, 2, OF_error, "Failed to open buffer");
        }

        //qDebug() << QString::fromStdWString(mi.Inform());

        if (getUint16(__T("ImageCount")) > 0)
        {
            type = MediaInfoLib::Stream_Image;
        }
        else if (getUint16(__T("VideoCount")) > 0)
        {
            type = MediaInfoLib::Stream_Video;
        }
        else
        {
            return makeOFCondition(0, 3, OF_error, "Unsupported format");
        }

        cond = dset->putAndInsertUint16(DCM_Rows, getUint16(__T("Height")));
        if (cond.bad())
          return cond;

        cond = dset->putAndInsertUint16(DCM_Columns, getUint16(__T("Width")));
        if (cond.bad())
          return cond;

        auto bitDepth = getUint16(__T("BitDepth"));

        cond = dset->putAndInsertUint16(DCM_BitsAllocated, bitDepth);
        if (cond.bad())
          return cond;

        cond = dset->putAndInsertUint16(DCM_BitsStored, bitDepth);
        if (cond.bad())
          return cond;

        cond = dset->putAndInsertUint16(DCM_HighBit, bitDepth - 1);
        if (cond.bad())
          return cond;

        cond = dset->putAndInsertUint16(DCM_PixelRepresentation, 0);
        if (cond.bad())
          return cond;

        if (getStr(__T("ColorSpace")) == "YUV")
        {
            cond = dset->putAndInsertUint16(DCM_SamplesPerPixel, 3);
            if (cond.bad())
              return cond;

            auto subsampling = getStr(__T("ChromaSubsampling"));
            if (subsampling == "4:2:0")
            {
                cond = dset->putAndInsertString(DCM_PhotometricInterpretation, "YBR_FULL");
                if (cond.bad())
                  return cond;
            }
            else if (subsampling == "4:2:2")
            {
                cond = dset->putAndInsertString(DCM_PhotometricInterpretation, "YBR_FULL_422");
                if (cond.bad())
                  return cond;
            }
            else
            {
                return makeOFCondition(0, 4, OF_error, "Unsupperted color format");
            }

            // Should only be written if Samples per Pixel > 1
            //
            cond = dset->putAndInsertUint16(DCM_PlanarConfiguration, 0);
            if (cond.bad())
              return cond;
        }
        else
        {
            cond = dset->putAndInsertUint16(DCM_SamplesPerPixel, 1);
            if (cond.bad())
              return cond;

            cond = dset->putAndInsertString(DCM_PhotometricInterpretation, "MONOCHROME2");
            if (cond.bad())
              return cond;
        }

        auto codec = getStr(__T("Codec"));

        if (type == MediaInfoLib::Stream_Image)
        {
            cond = dset->putAndInsertString(DCM_SOPClassUID, UID_SecondaryCaptureImageStorage);
            if (cond.bad())
              return cond;

            if (getStr(__T("Compression_Mode")) == "Lossy")
            {
                cond = dset->putAndInsertOFStringArray(DCM_LossyImageCompression, "01");
                if (cond.bad())
                  return cond;

                cond = dset->putAndInsertOFStringArray(DCM_LossyImageCompressionMethod, "ISO_10918_1");
                if (cond.bad())
                  return cond;
            }

            if (0 == codec.compare("JPEG", Qt::CaseInsensitive))
            {
#if OFFIS_DCMTK_VER < 0x030601
                ts = EXS_JPEGProcess1TransferSyntax;
#else
                ts = EXS_JPEGProcess1;
#endif
            }
            else
            {
                return makeOFCondition(0, 5, OF_error, "Unsupperted image format");
            }
        }
        else
        {
            cond = dset->putAndInsertString(DCM_SOPClassUID, UID_VideoEndoscopicImageStorage);
            if (cond.bad())
              return cond;

            auto frameRate = getStr(__T("FrameRate")).toDouble();

            cond = dset->putAndInsertString(DCM_CineRate, QString::number((Uint16)(frameRate + 0.5)).toUtf8());
            if (cond.bad())
              return cond;

            cond = dset->putAndInsertString(DCM_FrameTime, QString::number(1000.0 / frameRate).toUtf8());
            if (cond.bad())
              return cond;

            cond = dset->putAndInsertString(DCM_NumberOfFrames, getStr(__T("FrameCount")).toUtf8());
            if (cond.bad())
              return cond;

#if OFFIS_DCMTK_VER >= 0x030601
            cond = dset->putAndInsertTagKey(DCM_FrameIncrementPointer, DCM_FrameTime);
            if (cond.bad())
              return cond;
#endif
            if (QSettings().value("store-video-as-binary", false).toBool())
            {
                ts = EXS_LittleEndianImplicit;
            }
            else
            {
                auto codecProfile = getStr(__T("Codec_Profile"));
                if (0 == codec.compare("MPEG-2V", Qt::CaseInsensitive))
                {
                    if (codecProfile.startsWith("main@", Qt::CaseInsensitive))
                    {
                        ts = EXS_MPEG2MainProfileAtMainLevel;
                    }
                    else
                    {
                        ts = EXS_MPEG2MainProfileAtHighLevel;
                    }
                }
#if OFFIS_DCMTK_VER >= 0x030601
                else if (0 == codec.compare("AVC", Qt::CaseInsensitive))
                {
                    if (codecProfile.startsWith("main@", Qt::CaseInsensitive))
                    {
                        ts = EXS_MPEG4HighProfileLevel4_1;
                    }
                    else
                    {
                        ts = EXS_MPEG4BDcompatibleHighProfileLevel4_1;
                    }
                }
#endif
                else
                {
                    qDebug() << "Unknown 'codec " << codec << "' the file will be stored as binary";
                    ts = EXS_LittleEndianImplicit;
                }
            }
        }
        return EC_Normal;
    }
};

static OFCondition
insertEncapsulatedPixelData
    ( DcmDataset* dset
    , uchar *pixData
    , qint64 length
    , E_TransferSyntax outputTS
    )
{
  OFCondition cond;

  // create initial pixel sequence
  DcmPixelSequence* pixelSequence = new DcmPixelSequence(DcmTag(DCM_PixelData, EVR_OB));
  if (pixelSequence == NULL)
    return EC_MemoryExhausted;

  // insert empty offset table into sequence
  DcmPixelItem *offsetTable = new DcmPixelItem(DcmTag(DCM_Item, EVR_OB));
  if (offsetTable == NULL)
  {
    delete pixelSequence; pixelSequence = NULL;
    return EC_MemoryExhausted;
  }
  cond = pixelSequence->insert(offsetTable);
  if (cond.bad())
  {
    delete offsetTable; offsetTable = NULL;
    delete pixelSequence; pixelSequence = NULL;
    return cond;
  }

  // store compressed frame into pixel seqeuence
  DcmOffsetList dummyList;
  cond = pixelSequence->storeCompressedFrame(dummyList, pixData, length, 0);
  if (cond.bad())
  {
    delete pixelSequence; pixelSequence = NULL;
    return cond;
  }

  // insert pixel data attribute incorporating pixel sequence into dataset
  DcmPixelData *pixelData = new DcmPixelData(DCM_PixelData);
  if (pixelData == NULL)
  {
    delete pixelSequence; pixelSequence = NULL;
    return EC_MemoryExhausted;
  }
  /* tell pixel data element that this is the original presentation of the pixel data
   * pixel data and how it compressed
   */
  pixelData->putOriginalRepresentation(outputTS, NULL, pixelSequence);
  cond = dset->insert(pixelData);
  if (cond.bad())
  {
    delete pixelData; pixelData = NULL; // also deletes contained pixel sequence
    return cond;
  }

  return EC_Normal;
}

OFCondition
readAndInsertPixelData
    ( const QString& fileName
    , DcmDataset* dset
    , E_TransferSyntax& outputTS
    )
{
    QFile file(fileName);
    outputTS = EXS_Unknown;

    if (!file.open(QFile::ReadOnly))
    {
        return makeOFCondition(0, 1, OF_error, "Failed to open file");
    }

    qint64 length  = file.size();
    uchar* pixData = file.map(0, length);
    if (!pixData)
    {
        return makeOFCondition(0, 2, OF_error, "Failed to map file");
    }

    DcmConverter cvt;
    OFCondition cond = cvt.readPixelData(dset, pixData, length, outputTS);

    if (cond.good())
    {
        DcmXfer transport(outputTS);
        if (transport.isEncapsulated())
        {
            cond = insertEncapsulatedPixelData(dset, pixData, length, outputTS);
        }
        else
        {
            /* Not encapsulated */
            cond = dset->putAndInsertUint8Array(DCM_PixelData, pixData, length);
        }
    }
    return cond;
}
