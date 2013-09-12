dicom {
    QT        += network
    unix:LIBS += -ldcmnet -lwrap -ldcmdata -loflog -lofstd -lssl -lz
    DEFINES   += WITH_DICOM

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
