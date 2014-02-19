dicom {
    QT        += network
    DEFINES   += WITH_DICOM

    unix:LIBS += -ldcmnet -lwrap -ldcmdata -loflog -lofstd -lssl -lz
    # libmediainfo.pc adds UNICODE, but dcmtk isn't compatible with wchar,
    # so we can't use pkgconfig for this library
    LIBS += -lmediainfo -lzen

    SOURCES   += \
        dicom/dcmclient.cpp \
        dicom/dcmconverter.cpp \
        dicom/detailsdialog.cpp \
        dicom/dicomdevicesettings.cpp \
        dicom/dicommppsmwlsettings.cpp \
        dicom/dicomserverdetails.cpp \
        dicom/dicomserversettings.cpp \
        dicom/dicomstoragesettings.cpp \
        dicom/transcyrillic.cpp \
        dicom/worklistcolumnsettings.cpp \
        dicom/worklist.cpp \
        dicom/worklistquerysettings.cpp

    HEADERS += \
        dicom/dcmclient.h \
        dicom/dcmconverter.h \
        dicom/detailsdialog.h \
        dicom/dicomdevicesettings.h \
        dicom/dicommppsmwlsettings.h \
        dicom/dicomserverdetails.h \
        dicom/dicomserversettings.h \
        dicom/dicomstoragesettings.h \
        dicom/transcyrillic.h \
        dicom/worklistcolumnsettings.h \
        dicom/worklist.h \
        dicom/worklistquerysettings.h
}
