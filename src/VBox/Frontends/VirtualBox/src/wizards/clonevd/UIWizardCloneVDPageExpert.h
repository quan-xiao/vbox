/* $Id: UIWizardCloneVDPageExpert.h 82968 2020-02-04 10:35:17Z vboxsync $ */
/** @file
 * VBox Qt GUI - UIWizardCloneVDPageExpert class declaration.
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

#ifndef FEQT_INCLUDED_SRC_wizards_clonevd_UIWizardCloneVDPageExpert_h
#define FEQT_INCLUDED_SRC_wizards_clonevd_UIWizardCloneVDPageExpert_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

/* GUI includes: */
#include "UIWizardCloneVDPageBasic1.h"
#include "UIWizardCloneVDPageBasic2.h"
#include "UIWizardCloneVDPageBasic3.h"

/* Forward declarations: */
class QGroupBox;


/** Expert page of the Clone Virtual Disk Image wizard: */
class UIWizardCloneVDPageExpert : public UIWizardPage,
                                  public UIWizardCloneVDPage1,
                                  public UIWizardCloneVDPage2,
                                  public UIWizardCloneVDPage3
{
    Q_OBJECT;
    Q_PROPERTY(CMediumFormat mediumFormat READ mediumFormat WRITE setMediumFormat);
    Q_PROPERTY(qulonglong mediumVariant READ mediumVariant WRITE setMediumVariant);
    Q_PROPERTY(QString mediumPath READ mediumPath);
    Q_PROPERTY(qulonglong mediumSize READ mediumSize);

public:

    /** Constructs basic page.
      * @param  comSourceVirtualDisk  Brings the initial source disk to make copy from.
      * @param  enmDeviceType         Brings the device type to limit format to. */
    UIWizardCloneVDPageExpert(KDeviceType enmDeviceType);

protected:

    /** Allows to access 'wizard()' from base part. */
    UIWizard *wizardImp() const { return wizard(); }
    /** Allows to access 'this' from base part. */
    UIWizardPage* thisImp() { return this; }
    /** Allows to access 'field()' from base part. */
    QVariant fieldImp(const QString &strFieldName) const { return UIWizardPage::field(strFieldName); }

private slots:

    /** Handles medium format change. */
    void sltMediumFormatChanged();

    /** Handles target disk change. */
    void sltSelectLocationButtonClicked();

private:

    /** Handles translation event. */
    virtual void retranslateUi() /* override */;

    /** Prepares the page. */
    virtual void initializePage() /* override */;

    /** Returns whether the page is complete. */
    virtual bool isComplete() const /* override */;

    /** Returns whether the page is valid. */
    virtual bool validatePage() /* override */;

    /** Sets the target disk name and location. */
    void setTargetLocation();

    /** Holds the format container instance. */
    QGroupBox *m_pFormatCnt;
    /** Holds the variant container instance. */
    QGroupBox *m_pVariantCnt;
    /** Holds the target disk container instance. */
    QGroupBox *m_pDestinationCnt;
};

#endif /* !FEQT_INCLUDED_SRC_wizards_clonevd_UIWizardCloneVDPageExpert_h */
