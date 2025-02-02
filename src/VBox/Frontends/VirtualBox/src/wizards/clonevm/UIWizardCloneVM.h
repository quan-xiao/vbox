/* $Id: UIWizardCloneVM.h 82968 2020-02-04 10:35:17Z vboxsync $ */
/** @file
 * VBox Qt GUI - UIWizardCloneVM class declaration.
 */

/*
 * Copyright (C) 2011-2020 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#ifndef FEQT_INCLUDED_SRC_wizards_clonevm_UIWizardCloneVM_h
#define FEQT_INCLUDED_SRC_wizards_clonevm_UIWizardCloneVM_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

/* GUI includes: */
#include "UIWizard.h"

/* COM includes: */
#include "COMEnums.h"
#include "CMachine.h"
#include "CSnapshot.h"

/* Clone VM wizard: */
class UIWizardCloneVM : public UIWizard
{
    Q_OBJECT;

public:

    /* Page IDs: */
    enum
    {
        Page1,
        Page2,
        Page3
    };

    /* Page IDs: */
    enum
    {
        PageExpert
    };

    /* Constructor: */
    UIWizardCloneVM(QWidget *pParent, const CMachine &machine,
                    const QString &strGroup, CSnapshot snapshot = CSnapshot());

protected:

    /* CLone VM stuff: */
    bool cloneVM();

    /* Who will be able to clone virtual-machine: */
    friend class UIWizardCloneVMPageBasic2;
    friend class UIWizardCloneVMPageBasic3;
    friend class UIWizardCloneVMPageExpert;

private:

    /* Translation stuff: */
    void retranslateUi();

    /* Pages related stuff: */
    void prepare();

    /* Variables: */
    CMachine  m_machine;
    CSnapshot m_snapshot;
    QString   m_strGroup;
};

#endif /* !FEQT_INCLUDED_SRC_wizards_clonevm_UIWizardCloneVM_h */
