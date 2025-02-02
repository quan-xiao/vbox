/* $Id: UIMediumSizeEditor.h 82968 2020-02-04 10:35:17Z vboxsync $ */
/** @file
 * VBox Qt GUI - UIMediumSizeEditor class declaration.
 */

/*
 * Copyright (C) 2006-2020 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#ifndef FEQT_INCLUDED_SRC_widgets_UIMediumSizeEditor_h
#define FEQT_INCLUDED_SRC_widgets_UIMediumSizeEditor_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

/* Qt includes: */
#include <QRegularExpression>
#include <QWidget>

/* GUI includes: */
#include "QIWithRetranslateUI.h"
#include "UIDefs.h"
#include "UILibraryDefs.h"

/* Forward declarations: */
class QLabel;
class QSlider;
class QString;
class QWidget;
class QILineEdit;

/** Medium size editor widget. */
class SHARED_LIBRARY_STUFF UIMediumSizeEditor : public QIWithRetranslateUI<QWidget>
{
    Q_OBJECT;

signals:

    /** Notifies listeners about medium size changed. */
    void sigSizeChanged(qulonglong uSize);

public:

    /** Constructs medium size editor passing @a pParent to the base-class. */
    UIMediumSizeEditor(QWidget *pParent = 0);

    /** Returns the medium size. */
    qulonglong mediumSize() const { return m_uSize; }
    /** Sets the initial medium size as the widget is created. */
    void setMediumSize(qulonglong uSize);

protected:

    /** Handles translation event. */
    virtual void retranslateUi() /* override */;

private slots:

    /** Handles size slider change. */
    void sltSizeSliderChanged(int iValue);
    /** Handles size editor text edit finished signal. */
    void sltSizeEditorTextChanged();

private:

    /** Prepares all. */
    void prepare();

    /** Calculates slider scale according to passed @a uMaximumMediumSize. */
    static int calculateSliderScale(qulonglong uMaximumMediumSize);
    /** Returns log2 for passed @a uValue. */
    static int log2i(qulonglong uValue);
    /** Converts passed bytes @a uValue to slides scaled value using @a iSliderScale. */
    static int sizeMBToSlider(qulonglong uValue, int iSliderScale);
    /** Converts passed slider @a uValue to bytes unscaled value using @a iSliderScale. */
    static qulonglong sliderToSizeMB(int uValue, int iSliderScale);
    /** Updates slider/editor tool-tips. */
    void updateSizeToolTips(qulonglong uSize);
    /** Checks if the uSize is divisible by m_uSectorSize */
    qulonglong checkSectorSizeAlignment(qulonglong uSize);
    QString ensureSizeSuffix(const QString &strSizeString);

    /* Holds the block size. We force m_uSize to be multiple of this number. */
    static const qulonglong m_uSectorSize;
    /** Holds the minimum medium size. */
    const qulonglong  m_uSizeMin;
    /** Holds the maximum medium size. */
    const qulonglong  m_uSizeMax;
    /** Holds the slider scale. */
    const int         m_iSliderScale;
    /** Holds the current medium size. */
    qulonglong        m_uSize;
    SizeSuffix        m_enmSizeSuffix;

    /** Holds the size slider. */
    QSlider    *m_pSlider;
    /** Holds the minimum size label. */
    QLabel     *m_pLabelMinSize;
    /** Holds the maximum size label. */
    QLabel     *m_pLabelMaxSize;
    /** Holds the size editor. */
    QILineEdit *m_pEditor;

    /* A regular expression used to remove any character from a QString which is neither a digit nor decimal separator. */
    QRegularExpression m_regExNonDigitOrSeparator;
};

#endif /* !FEQT_INCLUDED_SRC_widgets_UIMediumSizeEditor_h */
