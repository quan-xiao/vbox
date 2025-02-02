/* $Id: UIWizardExportAppPageExpert.h 86346 2020-09-30 13:02:39Z vboxsync $ */
/** @file
 * VBox Qt GUI - UIWizardExportAppPageExpert class declaration.
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

#ifndef FEQT_INCLUDED_SRC_wizards_exportappliance_UIWizardExportAppPageExpert_h
#define FEQT_INCLUDED_SRC_wizards_exportappliance_UIWizardExportAppPageExpert_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

/* GUI includes: */
#include "UIWizardExportAppPageBasic1.h"
#include "UIWizardExportAppPageBasic2.h"
#include "UIWizardExportAppPageBasic3.h"

/* Forward declarations: */
class QGroupBox;

/** UIWizardPage extension for UIWizardExportAppPage1, UIWizardExportAppPage2 and UIWizardExportAppPage3. */
class UIWizardExportAppPageExpert : public UIWizardPage,
                                    public UIWizardExportAppPage1,
                                    public UIWizardExportAppPage2,
                                    public UIWizardExportAppPage3
{
    Q_OBJECT;
    Q_PROPERTY(QStringList machineNames READ machineNames);
    Q_PROPERTY(QList<QUuid> machineIDs READ machineIDs);
    Q_PROPERTY(QString format READ format WRITE setFormat);
    Q_PROPERTY(bool isFormatCloudOne READ isFormatCloudOne);
    Q_PROPERTY(QString path READ path WRITE setPath);
    Q_PROPERTY(MACAddressExportPolicy macAddressExportPolicy READ macAddressExportPolicy WRITE setMACAddressExportPolicy);
    Q_PROPERTY(bool manifestSelected READ isManifestSelected WRITE setManifestSelected);
    Q_PROPERTY(bool includeISOsSelected READ isIncludeISOsSelected WRITE setIncludeISOsSelected);
    Q_PROPERTY(QString providerShortName READ providerShortName);
    Q_PROPERTY(CAppliance appliance READ appliance);
    Q_PROPERTY(CCloudClient client READ client);
    Q_PROPERTY(CVirtualSystemDescription vsd READ vsd);
    Q_PROPERTY(CVirtualSystemDescriptionForm vsdExportForm READ vsdExportForm);
    Q_PROPERTY(CloudExportMode cloudExportMode READ cloudExportMode);
    Q_PROPERTY(ExportAppliancePointer applianceWidget READ applianceWidget);

public:

    /** Constructs expert page.
      * @param  selectedVMNames  Brings the list of selected VM names. */
    UIWizardExportAppPageExpert(const QStringList &selectedVMNames, bool fExportToOCIByDefault);

protected:

    /** Allows access wizard from base part. */
    UIWizard *wizardImp() const { return UIWizardPage::wizard(); }
    /** Allows access page from base part. */
    UIWizardPage* thisImp() { return this; }
    /** Allows access wizard-field from base part. */
    QVariant fieldImp(const QString &strFieldName) const { return UIWizardPage::field(strFieldName); }

    /** Handles translation event. */
    virtual void retranslateUi() /* override */;

    /** Performs page initialization. */
    virtual void initializePage() /* override */;
    /** Performs page uninitialization. */
    virtual void cleanupPage() /* override */;

    /** Returns whether page is complete. */
    virtual bool isComplete() const /* override */;

    /** Performs page validation. */
    virtual bool validatePage() /* override */;

private slots:

    /** Handles VM selector index change. */
    void sltVMSelectionChangeHandler();

    /** Handles format combo change. */
    void sltHandleFormatComboChange();

    /** Handles change in file-name selector. */
    void sltHandleFileSelectorChange();

    /** Handles change in MAC address export policy combo-box. */
    void sltHandleMACAddressExportPolicyComboChange();

    /** Handles change in profile combo-box. */
    void sltHandleProfileComboChange();

    /** Handles profile tool-button click. */
    void sltHandleProfileButtonClick();

private:

    /** Holds whether starting page was polished. */
    bool  m_fPolished;

    /** Holds the VM selector container instance. */
    QGroupBox *m_pSelectorCnt;
    /** Holds the appliance widget container reference. */
    QGroupBox *m_pApplianceCnt;
    /** Holds the settings widget container reference. */
    QGroupBox *m_pSettingsCnt;
};

#endif /* !FEQT_INCLUDED_SRC_wizards_exportappliance_UIWizardExportAppPageExpert_h */
