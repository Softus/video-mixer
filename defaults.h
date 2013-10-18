#ifndef BERYLLIUM_DEFAULTS_H
#define BERYLLIUM_DEFAULTS_H

#define DEFAULT_DISPLAY_SINK          "autovideosink"
#define DEFAULT_FOLDER_TEMPLATE       "/%yyyy%-%MM%/%dd%/%name%/"
#define DEFAULT_OUTPUT_PATH           "/video"
#define DEFAULT_RTP_PAYLOADER         "rtpmp2tpay"
#define DEFAULT_RTP_SINK              "udpsink"
#define DEFAULT_VIDEOBITRATE          4000
#define DEFAULT_VIDEO_ENCODER         "ffenc_mpeg2video"
#define DEFAULT_MANDATORY_FIELD_COLOR "red"
#define DEFAULT_MANDATORY_FIELDS      {"PatientID", "Name"}
#define DEFAULT_IMAGE_TEMPLATE        "image-%study%-%nn%"
#define DEFAULT_CLIP_TEMPLATE         "clip-%study%-%nn%"
#define DEFAULT_VIDEO_TEMPLATE        "video-%study%"
#define DEFAULT_IMAGE_ENCODER         "jpegenc"
#define DEFAULT_IMAGE_SINK            "multifilesink"
#define DEFAULT_VIDEO_SINK            "filesink"
#define DEFAULT_VIDEO_MUXER           "mpegpsmux"

#ifdef WITH_DICOM
#define DEFAULT_EXPORT_TO_DICOM     true
#define DEFAULT_TRANSLATE_CYRILLIC  true
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
