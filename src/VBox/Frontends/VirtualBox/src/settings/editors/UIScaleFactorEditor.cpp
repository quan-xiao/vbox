/* $Id: UIScaleFactorEditor.cpp 86085 2020-09-10 13:57:52Z vboxsync $ */
/** @file
 * VBox Qt GUI - UIScaleFactorEditor class implementation.
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

/* Qt includes: */
#include <QComboBox>
#include <QGridLayout>
#include <QLabel>
#include <QSpacerItem>
#include <QSpinBox>
#include <QWidget>

/* GUI includes: */
#include "QIAdvancedSlider.h"
#include "UIDesktopWidgetWatchdog.h"
#include "UIScaleFactorEditor.h"

/* External includes: */
#include <math.h>


UIScaleFactorEditor::UIScaleFactorEditor(QWidget *pParent)
    : QIWithRetranslateUI<QWidget>(pParent)
    , m_pMainLayout(0)
    , m_pMonitorComboBox(0)
    , m_pScaleSlider(0)
    , m_pScaleSpinBox(0)
    , m_pMinScaleLabel(0)
    , m_pMaxScaleLabel(0)
    , m_dDefaultScaleFactor(1.0)
{
    /* Prepare: */
    prepare();
    /* Append a default scale factor to the list as an global scale factor: */
    m_scaleFactors.append(1.0);
}

void UIScaleFactorEditor::setMonitorCount(int iMonitorCount)
{
    if (!m_pMonitorComboBox)
        return;
    /* We always have 0th for global scale factor (in combo box and scale factor list): */
    int iEndMonitorCount = iMonitorCount + 1;
    int iCurrentMonitorCount = m_pMonitorComboBox->count();
    if (iEndMonitorCount == iCurrentMonitorCount)
        return;
    m_pMonitorComboBox->setEnabled(iMonitorCount > 1);
    m_pMonitorComboBox->blockSignals(true);
    int iCurrentMonitorIndex = m_pMonitorComboBox->currentIndex();
    if (iCurrentMonitorCount < iEndMonitorCount)
    {
        for (int i = iCurrentMonitorCount; i < iEndMonitorCount; ++i)
            m_pMonitorComboBox->insertItem(i, tr("Monitor %1").arg(i));
    }
    else
    {
        for (int i = iCurrentMonitorCount - 1; i >= iEndMonitorCount; --i)
            m_pMonitorComboBox->removeItem(i);
    }
    m_pMonitorComboBox->setEnabled(iMonitorCount > 1);
    /* If we have a single monitor select the "All Monitors" item in the combo
       but make sure we retain the scale factor of the 0th monitor: */
    if (iMonitorCount <= 1)
    {
        if (m_scaleFactors.size() >= 2)
            m_scaleFactors[0] = m_scaleFactors[1];
        m_pMonitorComboBox->setCurrentIndex(0);
    }
    m_pMonitorComboBox->blockSignals(false);
    /* Update the slider and spinbox values if the combobox index has changed: */
    if (iCurrentMonitorIndex != m_pMonitorComboBox->currentIndex())
        updateValuesAfterMonitorChange();
}

void UIScaleFactorEditor::setScaleFactors(const QList<double> &scaleFactors)
{
    m_scaleFactors.clear();
    /* If we have a single value from the extra data and we treat if as a default scale factor: */
    if (scaleFactors.size() == 1)
    {
        m_dDefaultScaleFactor = scaleFactors.at(0);
        m_scaleFactors.append(m_dDefaultScaleFactor);
        setIsGlobalScaleFactor(true);
        return;
    }

    /* Insert 0th element as the global scalar value: */
    m_scaleFactors.append(m_dDefaultScaleFactor);
    m_scaleFactors.append(scaleFactors);
    setIsGlobalScaleFactor(false);
}

QList<double> UIScaleFactorEditor::scaleFactors() const
{
    QList<double> scaleFactorList;
    if (m_scaleFactors.size() == 0)
        return scaleFactorList;

    /* Decide if the users wants a global (not per monitor) scaling. */
    bool globalScaleFactor = false;

    /* if "All Monitors" item is selected in the combobox: */
    if (m_pMonitorComboBox && m_pMonitorComboBox->currentIndex() == 0)
        globalScaleFactor = true;
    /* Also check if all of the monitor scale factors equal to the global scale factor: */
    if (!globalScaleFactor)
    {
        globalScaleFactor = true;
        for (int i = 1; i < m_scaleFactors.size() && globalScaleFactor; ++i)
            if (m_scaleFactors[0] != m_scaleFactors[i])
                globalScaleFactor = false;
    }

    if (globalScaleFactor)
    {
        scaleFactorList << m_scaleFactors.at(0);
    }
    else
    {
        /* Skip the 0th scale factor: */
        for (int i = 1; i < m_scaleFactors.size(); ++i)
            scaleFactorList.append(m_scaleFactors.at(i));
    }

    return scaleFactorList;
}

void UIScaleFactorEditor::setIsGlobalScaleFactor(bool bFlag)
{
    if (!m_pMonitorComboBox)
        return;
    if (bFlag && m_pMonitorComboBox->count() >= 1)
        m_pMonitorComboBox->setCurrentIndex(0);
    else
        if(m_pMonitorComboBox->count() >= 2)
            m_pMonitorComboBox->setCurrentIndex(1);
    updateValuesAfterMonitorChange();
}

void UIScaleFactorEditor::setDefaultScaleFactor(double dDefaultScaleFactor)
{
    m_dDefaultScaleFactor = dDefaultScaleFactor;
}

void UIScaleFactorEditor::setSpinBoxWidthHint(int iHint)
{
    m_pScaleSpinBox->setMinimumWidth(iHint);
}

void UIScaleFactorEditor::retranslateUi()
{
    if (m_pMonitorComboBox && m_pMonitorComboBox->count() > 0)
    {
        m_pMonitorComboBox->setItemText(0, tr("All Monitors"));
        for (int i = 1; i < m_pMonitorComboBox->count(); ++i)
            m_pMonitorComboBox->setItemText(i, tr("Monitor %1").arg(i));
    }
    setToolTip(tr("Controls the guest screen scale factor."));

    m_pMinScaleLabel->setText(tr("%1%").arg(m_pScaleSlider->minimum()));
    m_pMaxScaleLabel->setText(tr("%1%").arg(m_pScaleSlider->maximum()));
}

void UIScaleFactorEditor::sltScaleSpinBoxValueChanged(int value)
{
    setSliderValue(value);
    if (m_pMonitorComboBox)
        setScaleFactor(m_pMonitorComboBox->currentIndex(), value);
}

void UIScaleFactorEditor::sltScaleSliderValueChanged(int value)
{
    setSpinBoxValue(value);
    if (m_pMonitorComboBox)
        setScaleFactor(m_pMonitorComboBox->currentIndex(), value);
}

void UIScaleFactorEditor::sltMonitorComboIndexChanged(int)
{
    updateValuesAfterMonitorChange();
}

void UIScaleFactorEditor::prepare()
{
    m_pMainLayout = new QGridLayout(this);
    if (m_pMainLayout)
    {
        m_pMainLayout->setContentsMargins(0, 0, 0, 0);

        m_pMonitorComboBox = new QComboBox;
        if (m_pMonitorComboBox)
        {
            m_pMonitorComboBox->insertItem(0, "All Monitors");
            connect(m_pMonitorComboBox ,static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
                    this, &UIScaleFactorEditor::sltMonitorComboIndexChanged);
            m_pMainLayout->addWidget(m_pMonitorComboBox, 0, 0);
        }

        QGridLayout *pSliderLayout = new QGridLayout;
        if (pSliderLayout)
        {
            m_pScaleSlider = new QIAdvancedSlider;
            {
                m_pScaleSlider->setPageStep(10);
                m_pScaleSlider->setSingleStep(1);
                m_pScaleSlider->setTickInterval(10);
                m_pScaleSlider->setSnappingEnabled(true);
                connect(m_pScaleSlider, static_cast<void(QIAdvancedSlider::*)(int)>(&QIAdvancedSlider::valueChanged),
                        this, &UIScaleFactorEditor::sltScaleSliderValueChanged);
                pSliderLayout->addWidget(m_pScaleSlider, 0, 0, 1, 3);
            }

            m_pMinScaleLabel = new QLabel;
            if (m_pMinScaleLabel)
                pSliderLayout->addWidget(m_pMinScaleLabel, 1, 0);

            QSpacerItem *pSpacer = new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Minimum);
            if (pSpacer)
                pSliderLayout->addItem(pSpacer, 1, 1);

            m_pMaxScaleLabel = new QLabel;
            if (m_pMaxScaleLabel)
                pSliderLayout->addWidget(m_pMaxScaleLabel, 1, 2);

            m_pMainLayout->addLayout(pSliderLayout, 0, 1, 2, 1);
        }

        m_pScaleSpinBox = new QSpinBox;
        if (m_pScaleSpinBox)
        {
            setFocusProxy(m_pScaleSpinBox);
            m_pScaleSpinBox->setSuffix("%");
            connect(m_pScaleSpinBox ,static_cast<void(QSpinBox::*)(int)>(&QSpinBox::valueChanged),
                    this, &UIScaleFactorEditor::sltScaleSpinBoxValueChanged);
            m_pMainLayout->addWidget(m_pScaleSpinBox, 0, 3);
        }
    }

    prepareScaleFactorMinMaxValues();
    retranslateUi();
}

void UIScaleFactorEditor::prepareScaleFactorMinMaxValues()
{
    const int iHostScreenCount = gpDesktop->screenCount();
    if (iHostScreenCount == 0)
        return;
    double dMaxDevicePixelRatio = gpDesktop->devicePixelRatio(0);
    for (int i = 1; i < iHostScreenCount; ++i)
        if (dMaxDevicePixelRatio < gpDesktop->devicePixelRatio(i))
            dMaxDevicePixelRatio = gpDesktop->devicePixelRatio(i);

    const int iMinimum = 100;
    const int iMaximum = ceil(iMinimum + 100 * dMaxDevicePixelRatio);

    const int iStep = 25;

    m_pScaleSlider->setMinimum(iMinimum);
    m_pScaleSlider->setMaximum(iMaximum);
    m_pScaleSlider->setPageStep(iStep);
    m_pScaleSlider->setSingleStep(1);
    m_pScaleSlider->setTickInterval(iStep);
    m_pScaleSpinBox->setMinimum(iMinimum);
    m_pScaleSpinBox->setMaximum(iMaximum);
}

void UIScaleFactorEditor::setScaleFactor(int iMonitorIndex, int iScaleFactor)
{
    /* Make sure we have the corresponding scale values for all monitors: */
    if (m_pMonitorComboBox->count() > m_scaleFactors.size())
    {
        for (int i = m_scaleFactors.size(); i < m_pMonitorComboBox->count(); ++i)
            m_scaleFactors.append(m_dDefaultScaleFactor);
    }
    m_scaleFactors[iMonitorIndex] = iScaleFactor / 100.0;
}

void UIScaleFactorEditor::setSliderValue(int iValue)
{
    if (m_pScaleSlider && iValue != m_pScaleSlider->value())
    {
        m_pScaleSlider->blockSignals(true);
        m_pScaleSlider->setValue(iValue);
        m_pScaleSlider->blockSignals(false);
    }
}

void UIScaleFactorEditor::setSpinBoxValue(int iValue)
{
    if (m_pScaleSpinBox && iValue != m_pScaleSpinBox->value())
    {
        m_pScaleSpinBox->blockSignals(true);
        m_pScaleSpinBox->setValue(iValue);
        m_pScaleSpinBox->blockSignals(false);
    }
}

void UIScaleFactorEditor::updateValuesAfterMonitorChange()
{
    /* Set the spinbox value for the currently selected monitor: */
    if (m_pMonitorComboBox)
    {
        int currentMonitorIndex = m_pMonitorComboBox->currentIndex();
        while (m_scaleFactors.size() <= currentMonitorIndex)
            m_scaleFactors.append(m_dDefaultScaleFactor);

        setSpinBoxValue(100 * m_scaleFactors.at(currentMonitorIndex));
        setSliderValue(100 * m_scaleFactors.at(currentMonitorIndex));

    }
}
