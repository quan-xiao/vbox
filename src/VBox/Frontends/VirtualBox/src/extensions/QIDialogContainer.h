/* $Id: QIDialogContainer.h 84022 2020-04-27 19:18:37Z vboxsync $ */
/** @file
 * VBox Qt GUI - Qt extensions: QIDialogContainer class declaration.
 */

/*
 * Copyright (C) 2019-2020 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#ifndef FEQT_INCLUDED_SRC_extensions_QIDialogContainer_h
#define FEQT_INCLUDED_SRC_extensions_QIDialogContainer_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

/* Qt includes: */
#include <QDialog>

/* GUI includes: */
#include "QIWithRetranslateUI.h"
#include "UILibraryDefs.h"

/* Forward declarations: */
class QGridLayout;
class QLabel;
class QProgressBar;
class QWidget;
class QIDialogButtonBox;

/** QDialog sub-class used as executable input container for passed widget.
  * Should be used as popup or modal dialog wrapping functionality of the passed widget. */
class SHARED_LIBRARY_STUFF QIDialogContainer : public QIWithRetranslateUI2<QDialog>
{
    Q_OBJECT;

public:

    /** Constructs QIDialogContainer passing @a pParent & @a enmFlags to the base-class. */
    QIDialogContainer(QWidget *pParent = 0, Qt::WindowFlags enmFlags = Qt::WindowFlags());

    /** Defines containing @a pWidget. */
    void setWidget(QWidget *pWidget);

public slots:

    /** Activates window. */
    void sltActivateWindow() { activateWindow(); }

    /** Sets progress-bar to be @a fHidden. */
    void setProgressBarHidden(bool fHidden);

    /** Sets Ok button to be @a fEnabled. */
    void setOkButtonEnabled(bool fEnabled);

protected:

    /** Handles translation event. */
    virtual void retranslateUi() /* override */;

private:

    /** Prepares all. */
    void prepare();

    /** Holds the layout instance. */
    QGridLayout       *m_pLayout;
    /** Holds the widget reference. */
    QWidget           *m_pWidget;
    /** Holds the progress-bar instance. */
    QLabel            *m_pProgressLabel;
    /** Holds the progress-bar instance. */
    QProgressBar      *m_pProgressBar;
    /** Holds the button-box instance. */
    QIDialogButtonBox *m_pButtonBox;
};

#endif /* !FEQT_INCLUDED_SRC_extensions_QIDialogContainer_h */
