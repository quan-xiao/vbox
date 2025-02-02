/* $Id: UIWizardNewVDPageBasic3.h 82968 2020-02-04 10:35:17Z vboxsync $ */
/** @file
 * VBox Qt GUI - UIWizardNewVDPageBasic3 class declaration.
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

#ifndef FEQT_INCLUDED_SRC_wizards_newvd_UIWizardNewVDPageBasic3_h
#define FEQT_INCLUDED_SRC_wizards_newvd_UIWizardNewVDPageBasic3_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

/* GUI includes: */
#include "UIWizardPage.h"

/* Forward declarations: */
class CMediumFormat;
class QLineEdit;
class QIToolButton;
class QIRichTextLabel;
class UIMediumSizeEditor;


/* 3rd page of the New Virtual Hard Drive wizard (base part): */
class SHARED_LIBRARY_STUFF UIWizardNewVDPage3 : public UIWizardPageBase
{
protected:

    /* Constructor: */
    UIWizardNewVDPage3(const QString &strDefaultName, const QString &strDefaultPath);

    /* Handlers: */
    void onSelectLocationButtonClicked();

    /* Location-editors stuff: */
    static QString toFileName(const QString &strName, const QString &strExtension);
    /* Returns the full image file path except the extension. */
    static QString absoluteFilePath(const QString &strFileName, const QString &strPath);
    /* Returns the full image file path including the extension. */
    static QString absoluteFilePath(const QString &strFileName, const QString &strPath, const QString &strExtension);
    static QString defaultExtension(const CMediumFormat &mediumFormatRef);

    /* Checks if the medium file is bigger than what is allowed in FAT file systems. */
    bool checkFATSizeLimitation() const;

    /* Stuff for 'mediumPath' field: */
    QString mediumPath() const;

    /* Stuff for 'mediumSize' field: */
    qulonglong mediumSize() const;
    void setMediumSize(qulonglong uMediumSize);

    /* Variables: */
    QString m_strDefaultName;
    QString m_strDefaultPath;
    QString m_strDefaultExtension;
    qulonglong m_uMediumSizeMin;
    qulonglong m_uMediumSizeMax;

    /* Widgets: */
    QLineEdit *m_pLocationEditor;
    QIToolButton *m_pLocationOpenButton;
    UIMediumSizeEditor *m_pEditorSize;
};


/* 3rd page of the New Virtual Hard Drive wizard (basic extension): */
class SHARED_LIBRARY_STUFF UIWizardNewVDPageBasic3 : public UIWizardPage, public UIWizardNewVDPage3
{
    Q_OBJECT;
    Q_PROPERTY(QString mediumPath READ mediumPath);
    Q_PROPERTY(qulonglong mediumSize READ mediumSize WRITE setMediumSize);

public:

    /* Constructor: */
    UIWizardNewVDPageBasic3(const QString &strDefaultName, const QString &strDefaultPath, qulonglong uDefaultSize);

protected:

    /* Wrapper to access 'this' from base part: */
    UIWizardPage* thisImp() { return this; }
    /* Wrapper to access 'wizard-field' from base part: */
    QVariant fieldImp(const QString &strFieldName) const { return UIWizardPage::field(strFieldName); }

private slots:

    /* Location editors stuff: */
    void sltSelectLocationButtonClicked();

private:

    /* Translation stuff: */
    void retranslateUi();

    /* Prepare stuff: */
    void initializePage();

    /* Validation stuff: */
    bool isComplete() const;
    bool validatePage();

    /* Widgets: */
    QIRichTextLabel *m_pLocationLabel;
    QIRichTextLabel *m_pSizeLabel;
};


#endif /* !FEQT_INCLUDED_SRC_wizards_newvd_UIWizardNewVDPageBasic3_h */
