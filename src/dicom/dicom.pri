dicom {
    QT        += network
    LIBS      += -ldcmnet -ldcmdata -loflog -lofstd -lz -lmagic
    unix:LIBS += -lwrap -lssl
    DEFINES   += WITH_DICOM

    # libmediainfo.pc adds UNICODE, but dcmtk isn't compatible with wchar,
    # so we can't use pkgconfig for this library
    LIBS += -lmediainfo -lzen

    SOURCES   += \
        src/dicom/dcmclient.cpp \
        src/dicom/dcmconverter.cpp \
        src/dicom/detailsdialog.cpp \
        src/dicom/dicomdevicesettings.cpp \
        src/dicom/dicommppsmwlsettings.cpp \
        src/dicom/dicomserverdetails.cpp \
        src/dicom/dicomserversettings.cpp \
        src/dicom/dicomstoragesettings.cpp \
        src/dicom/transcyrillic.cpp \
        src/dicom/worklistcolumnsettings.cpp \
        src/dicom/worklist.cpp \
        src/dicom/worklistquerysettings.cpp

    HEADERS += \
        src/dicom/dcmclient.h \
        src/dicom/dcmconverter.h \
        src/dicom/detailsdialog.h \
        src/dicom/dicomdevicesettings.h \
        src/dicom/dicommppsmwlsettings.h \
        src/dicom/dicomserverdetails.h \
        src/dicom/dicomserversettings.h \
        src/dicom/dicomstoragesettings.h \
        src/dicom/transcyrillic.h \
        src/dicom/worklistcolumnsettings.h \
        src/dicom/worklist.h \
        src/dicom/worklistquerysettings.h
}
