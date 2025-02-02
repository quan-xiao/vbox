/* $Id: QIRichTextLabel.h 86557 2020-10-13 13:58:00Z vboxsync $ */
/** @file
 * VBox Qt GUI - Qt extensions: QIRichTextLabel class declaration.
 */

/*
 * Copyright (C) 2012-2020 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#ifndef FEQT_INCLUDED_SRC_extensions_QIRichTextLabel_h
#define FEQT_INCLUDED_SRC_extensions_QIRichTextLabel_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

/* Qt includes: */
#include <QTextBrowser>

/* GUI includes: */
#include "UILibraryDefs.h"

/** QLabel analog to reflect rich-text,
 ** based on private QTextBrowser functionality. */
class SHARED_LIBRARY_STUFF QIRichTextLabel : public QWidget
{
    Q_OBJECT;
    Q_PROPERTY(QString text READ text WRITE setText);

public:

    /** Constructs rich text-label passing @a pParent to the base-class. */
    QIRichTextLabel(QWidget *pParent = 0);

    /** Returns text. */
    QString text() const;

    /** Registers @a image under a passed @a strName. */
    void registerImage(const QImage &image, const QString &strName);

    /** Returns word wrapping policy. */
    QTextOption::WrapMode wordWrapMode() const;
    /** Defines word wrapping @a policy. */
    void setWordWrapMode(QTextOption::WrapMode policy);

    /** Installs event filter for a passed @ pFilterObj. */
    void installEventFilter(QObject *pFilterObj);

public slots:

    /** Defines @a iMinimumTextWidth. */
    void setMinimumTextWidth(int iMinimumTextWidth);

    /** Defines @a strText. */
    void setText(const QString &strText);

private:

    /** Holds the text-browser instance. */
    QTextBrowser *m_pTextBrowser;

    /** Holds the minimum text-width. */
    int m_iMinimumTextWidth;
};

#endif /* !FEQT_INCLUDED_SRC_extensions_QIRichTextLabel_h */
