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

#ifndef BERYLLIUM_DEFAULTS_H
#define BERYLLIUM_DEFAULTS_H

#define DEFAULT_DISPLAY_SINK          "autovideosink"
#define DEFAULT_FOLDER_TEMPLATE       "/%yyyy%-%MM%/%dd%/%name%-%id%/"
#define DEFAULT_HOTKEY_ABOUT          (int)(Qt::AltModifier | Qt::Key_Question)
#define DEFAULT_HOTKEY_ARCHIVE        (int)(Qt::Key_F2)
#define DEFAULT_HOTKEY_UPLOAD         (int)(Qt::Key_F6)
#define DEFAULT_HOTKEY_DELETE         (int)(Qt::Key_Delete)
#define DEFAULT_HOTKEY_EDIT           (int)(Qt::Key_F4)
#define DEFAULT_HOTKEY_PARENT_FOLDER  (int)(Qt::Key_Backspace)
#define DEFAULT_HOTKEY_RECORD_START   (int)(Qt::AltModifier | Qt::Key_R)
#define DEFAULT_HOTKEY_RECORD_STOP    (int)(Qt::AltModifier | Qt::Key_E)
#define DEFAULT_HOTKEY_REFRESH        (int)(Qt::Key_F5)
#define DEFAULT_HOTKEY_SETTINGS       (int)(Qt::Key_F9)
#define DEFAULT_HOTKEY_SHOW_DETAILS   (int)(Qt::AltModifier | Qt::Key_Return)
#define DEFAULT_HOTKEY_SNAPSHOT       (int)(Qt::AltModifier | Qt::Key_T)
#define DEFAULT_HOTKEY_START          (int)(Qt::AltModifier | Qt::Key_A)
#define DEFAULT_HOTKEY_WORKLIST       (int)(Qt::Key_F3)
#define DEFAULT_HOTKEY_NEXT_MODE      (int)(Qt::ControlModifier | Qt::Key_0)
#define DEFAULT_HOTKEY_LIST_MODE      (int)(Qt::ControlModifier | Qt::Key_1)
#define DEFAULT_HOTKEY_ICON_MODE      (int)(Qt::ControlModifier | Qt::Key_2)
#define DEFAULT_HOTKEY_GALLERY_MODE   (int)(Qt::ControlModifier | Qt::Key_3)
#define DEFAULT_HOTKEY_BROWSE         (int)(Qt::Key_F2)
#define DEFAULT_HOTKEY_PREV           (int)(Qt::Key_Left)
#define DEFAULT_HOTKEY_NEXT           (int)(Qt::Key_Right)
#define DEFAULT_HOTKEY_SEEK_BACK      (int)(Qt::ShiftModifier | Qt::Key_Left)
#define DEFAULT_HOTKEY_SEEK_FWD       (int)(Qt::ShiftModifier | Qt::Key_Right)
#define DEFAULT_HOTKEY_PLAY           (int)(Qt::Key_Space)
#define DEFAULT_HOTKEY_SELECT         (int)(Qt::Key_Return)
#define DEFAULT_OUTPUT_PATH           "/video"
#define DEFAULT_OUTPUT_UNIQUE         false
#define DEFAULT_RTP_PAYLOADER         "rtpmpvpay"
#define DEFAULT_RTP_SINK              "udpsink"
#define DEFAULT_VIDEOBITRATE          4000
#define DEFAULT_NOTIFY_CLIP_LIMIT     true
#define DEFAULT_CLIP_LIMIT            true
#define DEFAULT_MANDATORY_FIELD_COLOR "red"
#define DEFAULT_MANDATORY_FIELDS      (QStringList() << "PatientID" << "Name")
#define DEFAULT_MOTION_DETECTION      true
#define DEFAULT_MOTION_SENSITIVITY    0.75
#define DEFAULT_MOTION_THRESHOLD      0.02
#define DEFAULT_MOTION_MIN_FRAMES     1
#define DEFAULT_MOTION_GAP            5
#define DEFAULT_IMAGE_TEMPLATE        "image-%study%-%nn%"
#define DEFAULT_CLIP_TEMPLATE         "clip-%study%-%nn%"
#define DEFAULT_VIDEO_TEMPLATE        "video-%study%"
#define DEFAULT_VIDEO_OUTPUT_UNIQUE   false
#define DEFAULT_IMAGE_ENCODER         "jpegenc"
#define DEFAULT_IMAGE_SINK            "multifilesink"
#define DEFAULT_VIDEO_ENCODER         "ffenc_mpeg2video"
#define DEFAULT_VIDEO_MUXER           "mpegpsmux"
#define DEFAULT_LIMIT_VIDEO_FPS       false
#define DEFAULT_VIDEO_MAX_FPS         30
#define DEFAULT_SPLIT_VIDEO_FILES     false
#define DEFAULT_VIDEO_MAX_FILE_SIZE   1024
#define DEFAULT_VIDEO_RECORD_LIMIT    10
#define DEFAULT_VIDEO_RECORD_NOTIFY   2

#define DEFAULT_GST_DEBUG_ON          true
#define DEFAULT_GST_DEBUG_LEVEL       GST_LEVEL_WARNING
#define DEFAULT_GST_DEBUG_NO_COLOR    true
#define DEFAULT_GST_DEBUG             QString()
#define DEFAULT_GST_DEBUG_LOG_FILE    QString()
#define DEFAULT_GST_DEBUG_DOT_DIR     QString()

#ifdef WITH_DICOM
#define DEFAULT_DCMTK_DEBUG_ON        true
#define DEFAULT_DCMTK_DEBUG_LEVEL     log4cplus::WARN_LOG_LEVEL
#define DEFAULT_DCMTK_DEBUG_LOG_FILE  QString()
#define DEFAULT_DCMTK_LOG_CONFIG_FILE QString()

#define DEFAULT_EXPORT_CLIPS_TO_DICOM true
#define DEFAULT_EXPORT_VIDEO_TO_DICOM false
#define DEFAULT_MODALITY              "ES"
#define DEFAULT_TRANSLATE_CYRILLIC    true
#define DEFAULT_WORKLIST_COLUMNS      (QStringList() << "0010,0020" << "0010,0010" \
        << "0010,0030" << "0010,0040" << "0040,0007" << "0040,0002" << "0040,0003")
#define DEFAULT_WORKLIST_DATE_RANGE 1
#define DEFAULT_WORKLIST_DAY_DELTA  30
#define DEFAULT_COMPLETE_WITH_MPPS  true
#define DEFAULT_START_WITH_MPPS     true
#define DEFAULT_STORE_VIDEO_AS_BINARY false

#ifdef QT_DEBUG
  #define DEFAULT_TIMEOUT 3 // 3 seconds for test builds
#else
  #define DEFAULT_TIMEOUT 30 // 30 seconds for prodaction builds
#endif
#endif

#endif // BERYLLIUM_DEFAULTS_H
