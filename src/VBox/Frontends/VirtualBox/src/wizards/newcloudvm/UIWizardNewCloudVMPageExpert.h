/* $Id: UIWizardNewCloudVMPageExpert.h 86346 2020-09-30 13:02:39Z vboxsync $ */
/** @file
 * VBox Qt GUI - UIWizardNewCloudVMPageExpert class declaration.
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

#ifndef FEQT_INCLUDED_SRC_wizards_newcloudvm_UIWizardNewCloudVMPageExpert_h
#define FEQT_INCLUDED_SRC_wizards_newcloudvm_UIWizardNewCloudVMPageExpert_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

/* GUI includes: */
#include "UIWizardNewCloudVMPageBasic1.h"
#include "UIWizardNewCloudVMPageBasic2.h"

/* Forward declarations: */
class QGroupBox;

/** UIWizardPage extension for UIWizardNewCloudVMPage1 and UIWizardNewCloudVMPage2. */
class UIWizardNewCloudVMPageExpert : public UIWizardPage,
                                     public UIWizardNewCloudVMPage1,
                                     public UIWizardNewCloudVMPage2
{
    Q_OBJECT;
    Q_PROPERTY(QString location READ location);
    Q_PROPERTY(QString profileName READ profileName);

public:

    /** Constructs expert page. */
    UIWizardNewCloudVMPageExpert(bool fFullWizard);

protected:

    /** Allows access wizard from base part. */
    virtual UIWizard *wizardImp() const /* override */ { return UIWizardPage::wizard(); }

    /** Handles translation event. */
    virtual void retranslateUi() /* override */;

    /** Performs page initialization. */
    virtual void initializePage() /* override */;

    /** Returns whether page is complete. */
    virtual bool isComplete() const /* override */;

    /** Performs page validation. */
    virtual bool validatePage() /* override */;

private slots:

    /** Handles change in location combo-box. */
    void sltHandleLocationChange();

    /** Handles change in profile combo-box. */
    void sltHandleProfileComboChange();
    /** Handles profile tool-button click. */
    void sltHandleProfileButtonClick();

    /** Handles change in source tab-bar. */
    void sltHandleSourceChange();

    /** Handles change in instance list. */
    void sltHandleInstanceListChange();

    /** Initializes short wizard form. */
    void sltInitShortWizardForm();

private:

    /** Holds the location container instance. */
    QGroupBox *m_pCntLocation;
    /** Holds the source container instance. */
    QGroupBox *m_pCntSource;
    /** Holds the settings container instance. */
    QGroupBox *m_pSettingsCnt;
};

#endif /* !FEQT_INCLUDED_SRC_wizards_newcloudvm_UIWizardNewCloudVMPageExpert_h */
