/* $Id: UIWizardPage.h 82968 2020-02-04 10:35:17Z vboxsync $ */
/** @file
 * VBox Qt GUI - UIWizardPage class declaration.
 */

/*
 * Copyright (C) 2009-2020 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#ifndef FEQT_INCLUDED_SRC_wizards_UIWizardPage_h
#define FEQT_INCLUDED_SRC_wizards_UIWizardPage_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

/* Qt includes: */
#include <QVariant>
#include <QWizardPage>

/* GUI includes: */
#include "QIWithRetranslateUI.h"
#include "UILibraryDefs.h"

/* Forward declarations: */
class UIWizard;
class UIWizardPage;


/** One of two interfaces for wizard page.
  * This is page-BASE providing access API for basic/expert pages. */
class SHARED_LIBRARY_STUFF UIWizardPageBase
{
public:

    /** Destructs wizard page-base. */
    virtual ~UIWizardPageBase() { /* Makes MSC happy. */ }

protected:

    /** Returns wizard this page-base belongs to. */
    virtual UIWizard *wizardImp() const;

    /** Returns wizard page this page-base belongs to. */
    virtual UIWizardPage *thisImp();

    /** Returns page field with certain @a strFieldName. */
    virtual QVariant fieldImp(const QString &strFieldName) const;
};


/** One of two interfaces for wizard page.
  * This is page-BODY based on QWizardPage with advanced functionality. */
class SHARED_LIBRARY_STUFF UIWizardPage : public QIWithRetranslateUI<QWizardPage>
{
    Q_OBJECT;

public:

    /** Constructs wizard page. */
    UIWizardPage();

    /** Redirects the translation call to actual handler. */
    void retranslate() { retranslateUi(); }

    /** Marks page ready. */
    void markReady();

    /** Defines page @a strTitle. */
    void setTitle(const QString &strTitle);

protected:

    /** Returns wizard this page belongs to. */
    UIWizard *wizard() const;

    /** Starts page processing.  */
    void startProcessing();
    /** Ends page processing.  */
    void endProcessing();

    /** Holds whether page is ready. */
    bool m_fReady;
    /** Holds the page title. */
    QString m_strTitle;
};


#endif /* !FEQT_INCLUDED_SRC_wizards_UIWizardPage_h */
