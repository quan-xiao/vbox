/* $Id: UIDetailsSet.cpp 84623 2020-05-31 17:15:24Z vboxsync $ */
/** @file
 * VBox Qt GUI - UIDetailsSet class implementation.
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

/* Qt includes: */
#include <QPainter>
#include <QStyle>
#include <QStyleOptionGraphicsItem>

/* GUI includes: */
#include "UICommon.h"
#include "UIDetailsElements.h"
#include "UIDetailsModel.h"
#include "UIDetailsSet.h"
#include "UIMedium.h"
#include "UIVirtualBoxEventHandler.h"
#include "UIVirtualMachineItemCloud.h"
#include "UIVirtualMachineItemLocal.h"

/* COM includes: */
#include "CUSBController.h"
#include "CUSBDeviceFilters.h"


UIDetailsSet::UIDetailsSet(UIDetailsItem *pParent)
    : UIDetailsItem(pParent)
    , m_pMachineItem(0)
    , m_fFullSet(true)
    , m_fIsLocal(true)
    , m_fHasDetails(false)
    , m_configurationAccessLevel(ConfigurationAccessLevel_Null)
    , m_pBuildStep(0)
{
    /* Add set to the parent group: */
    parentItem()->addItem(this);

    /* Prepare set: */
    prepareSet();

    /* Prepare connections: */
    prepareConnections();
}

UIDetailsSet::~UIDetailsSet()
{
    /* Cleanup items: */
    clearItems();

    /* Remove set from the parent group: */
    parentItem()->removeItem(this);
}

void UIDetailsSet::clearSet()
{
    /* Clear passed arguments: */
    m_pMachineItem = 0;
    m_comMachine = CMachine();
    m_comCloudMachine = CCloudMachine();
}

void UIDetailsSet::buildSet(UIVirtualMachineItem *pMachineItem, bool fFullSet, const QMap<DetailsElementType, bool> &settings)
{
    /* Remember passed arguments: */
    m_pMachineItem = pMachineItem;
    m_fIsLocal = m_pMachineItem->itemType() == UIVirtualMachineItemType_Local;
    m_fHasDetails = m_pMachineItem->hasDetails();
    m_fFullSet = fFullSet;
    m_settings = settings;

    /* Prepare a list of types to build: */
    QList<DetailsElementType> types;

    /* Make sure we have details: */
    if (m_fHasDetails)
    {
        /* Special handling wrt item type: */
        switch (m_pMachineItem->itemType())
        {
            case UIVirtualMachineItemType_Local:
            {
                /* Get local machine: */
                m_comMachine = m_pMachineItem->toLocal()->machine();

                /* Compose a list of types to build: */
                if (m_fFullSet)
                    types << DetailsElementType_General << DetailsElementType_System << DetailsElementType_Preview
                          << DetailsElementType_Display << DetailsElementType_Storage << DetailsElementType_Audio
                          << DetailsElementType_Network << DetailsElementType_Serial << DetailsElementType_USB
                          << DetailsElementType_SF << DetailsElementType_UI << DetailsElementType_Description;
                else
                    types << DetailsElementType_General << DetailsElementType_System << DetailsElementType_Preview;

                /* Take into account USB controller restrictions: */
                const CUSBDeviceFilters &filters = m_comMachine.GetUSBDeviceFilters();
                if (filters.isNull() || !m_comMachine.GetUSBProxyAvailable())
                    m_settings.remove(DetailsElementType_USB);

                break;
            }
            case UIVirtualMachineItemType_CloudReal:
            {
                /* Get cloud machine: */
                m_comCloudMachine = m_pMachineItem->toCloud()->machine();

                /* Compose a list of types to build: */
                types << DetailsElementType_General;

                break;
            }
            default:
                break;
        }
    }

    /* Cleanup if new types differs from old: */
    if (m_types != types)
    {
        clearItems();
        m_elements.clear();
        updateGeometry();
    }

    /* Remember new types: */
    m_types = types;

    /* Build or emit fake signal: */
    if (m_fHasDetails)
        rebuildSet();
    else
        emit sigBuildDone();
}

void UIDetailsSet::sltBuildStep(const QUuid &uStepId, int iStepNumber)
{
    /* Cleanup build-step: */
    delete m_pBuildStep;
    m_pBuildStep = 0;

    /* Is step id valid? */
    if (uStepId != m_uSetId)
        return;

    /* Step number feats the bounds: */
    if (iStepNumber >= 0 && iStepNumber < m_types.size())
    {
        /* Load details settings: */
        const DetailsElementType enmElementType = m_types.at(iStepNumber);
        /* Should the element be visible? */
        bool fVisible = m_settings.contains(enmElementType);
        /* Should the element be opened? */
        bool fOpen = fVisible && m_settings[enmElementType];

        /* Check if element is present already: */
        UIDetailsElement *pElement = element(enmElementType);
        if (pElement && fOpen)
            pElement->open(false);
        /* Create element if necessary: */
        bool fJustCreated = false;
        if (!pElement)
        {
            fJustCreated = true;
            pElement = createElement(enmElementType, fOpen);
        }

        /* Show element if necessary: */
        if (fVisible && !pElement->isVisible())
        {
            /* Show the element: */
            pElement->show();
            /* Recursively update size-hint: */
            pElement->updateGeometry();
            /* Update layout: */
            model()->updateLayout();
        }
        /* Hide element if necessary: */
        else if (!fVisible && pElement->isVisible())
        {
            /* Hide the element: */
            pElement->hide();
            /* Recursively update size-hint: */
            updateGeometry();
            /* Update layout: */
            model()->updateLayout();
        }
        /* Update model if necessary: */
        else if (fJustCreated)
            model()->updateLayout();

        /* For visible element: */
        if (pElement->isVisible())
        {
            /* Create next build-step: */
            m_pBuildStep = new UIPrepareStep(this, pElement, uStepId, iStepNumber + 1);

            /* Build element: */
            pElement->updateAppearance();
        }
        /* For invisible element: */
        else
        {
            /* Just build next step: */
            sltBuildStep(uStepId, iStepNumber + 1);
        }
    }
    /* Step number out of bounds: */
    else
    {
        /* Update model: */
        model()->updateLayout();
        /* Repaint all the items: */
        foreach (UIDetailsItem *pItem, items())
            pItem->update();
        /* Notify listener about build done: */
        emit sigBuildDone();
    }
}

void UIDetailsSet::paint(QPainter *pPainter, const QStyleOptionGraphicsItem *pOptions, QWidget *)
{
    /* Paint background: */
    paintBackground(pPainter, pOptions);
}

QString UIDetailsSet::description() const
{
    return tr("Contains the details of virtual machine '%1'").arg(m_pMachineItem->name());
}

void UIDetailsSet::addItem(UIDetailsItem *pItem)
{
    switch (pItem->type())
    {
        case UIDetailsItemType_Element:
        {
            UIDetailsElement *pElement = pItem->toElement();
            DetailsElementType type = pElement->elementType();
            AssertMsg(!m_elements.contains(type), ("Element already added!"));
            m_elements.insert(type, pItem);
            break;
        }
        default:
        {
            AssertMsgFailed(("Invalid item type!"));
            break;
        }
    }
}

void UIDetailsSet::removeItem(UIDetailsItem *pItem)
{
    switch (pItem->type())
    {
        case UIDetailsItemType_Element:
        {
            UIDetailsElement *pElement = pItem->toElement();
            DetailsElementType type = pElement->elementType();
            AssertMsg(m_elements.contains(type), ("Element do not present (type = %d)!", (int)type));
            m_elements.remove(type);
            break;
        }
        default:
        {
            AssertMsgFailed(("Invalid item type!"));
            break;
        }
    }
}

QList<UIDetailsItem*> UIDetailsSet::items(UIDetailsItemType enmType /* = UIDetailsItemType_Element */) const
{
    switch (enmType)
    {
        case UIDetailsItemType_Element: return m_elements.values();
        case UIDetailsItemType_Any: return items(UIDetailsItemType_Element);
        default: AssertMsgFailed(("Invalid item type!")); break;
    }
    return QList<UIDetailsItem*>();
}

bool UIDetailsSet::hasItems(UIDetailsItemType enmType /* = UIDetailsItemType_Element */) const
{
    switch (enmType)
    {
        case UIDetailsItemType_Element: return !m_elements.isEmpty();
        case UIDetailsItemType_Any: return hasItems(UIDetailsItemType_Element);
        default: AssertMsgFailed(("Invalid item type!")); break;
    }
    return false;
}

void UIDetailsSet::clearItems(UIDetailsItemType enmType /* = UIDetailsItemType_Element */)
{
    switch (enmType)
    {
        case UIDetailsItemType_Element:
        {
            foreach (int iKey, m_elements.keys())
                delete m_elements[iKey];
            AssertMsg(m_elements.isEmpty(), ("Set items cleanup failed!"));
            break;
        }
        case UIDetailsItemType_Any:
        {
            clearItems(UIDetailsItemType_Element);
            break;
        }
        default:
        {
            AssertMsgFailed(("Invalid item type!"));
            break;
        }
    }
}

UIDetailsElement *UIDetailsSet::element(DetailsElementType enmElementType) const
{
    UIDetailsItem *pItem = m_elements.value(enmElementType, 0);
    if (pItem)
        return pItem->toElement();
    return 0;
}

void UIDetailsSet::updateLayout()
{
    /* Prepare variables: */
    const int iMargin = data(SetData_Margin).toInt();
    const int iSpacing = data(SetData_Spacing).toInt();
    const int iMaximumWidth = geometry().width();
    const UIDetailsElement *pPreviewElement = element(DetailsElementType_Preview);
    const int iPreviewWidth = pPreviewElement ? pPreviewElement->minimumWidthHint() : 0;
    const int iPreviewHeight = pPreviewElement ? pPreviewElement->minimumHeightHint() : 0;
    int iVerticalIndent = iMargin;

    /* Calculate Preview group elements: */
    QList<DetailsElementType> inGroup;
    QList<DetailsElementType> outGroup;
    int iAdditionalGroupHeight = 0;
    int iAdditionalPreviewHeight = 0;
    enumerateLayoutItems(inGroup, outGroup, iAdditionalGroupHeight, iAdditionalPreviewHeight);

    /* Layout all the elements: */
    foreach (UIDetailsItem *pItem, items())
    {
        /* Skip hidden: */
        if (!pItem->isVisible())
            continue;

        /* For each particular element: */
        UIDetailsElement *pElement = pItem->toElement();
        const DetailsElementType enmElementType = pElement->elementType();
        switch (enmElementType)
        {
            case DetailsElementType_General:
            case DetailsElementType_System:
            case DetailsElementType_Display:
            case DetailsElementType_Storage:
            case DetailsElementType_Audio:
            case DetailsElementType_Network:
            case DetailsElementType_Serial:
            case DetailsElementType_USB:
            case DetailsElementType_SF:
            case DetailsElementType_UI:
            case DetailsElementType_Description:
            {
                /* Move element: */
                pElement->setPos(0, iVerticalIndent);

                /* Calculate required width: */
                int iWidth = iMaximumWidth;
                if (inGroup.contains(enmElementType))
                    iWidth -= (iSpacing + iPreviewWidth);
                /* Resize element to required width (separately from height): */
                if (pElement->geometry().width() != iWidth)
                    pElement->resize(iWidth, pElement->geometry().height());

                /* Calculate required height: */
                int iHeight = pElement->minimumHeightHint();
                if (   !inGroup.isEmpty()
                    && inGroup.last() == enmElementType)
                {
                    if (!pElement->isAnimationRunning() && !pElement->isClosed())
                        iHeight += iAdditionalGroupHeight;
                    else
                        iVerticalIndent += iAdditionalGroupHeight;
                }
                /* Resize element to required height (separately from width): */
                if (pElement->geometry().height() != iHeight)
                    pElement->resize(pElement->geometry().width(), iHeight);

                /* Layout element content: */
                pItem->updateLayout();
                /* Advance indent: */
                iVerticalIndent += (iHeight + iSpacing);

                break;
            }
            case DetailsElementType_Preview:
            {
                /* Move element: */
                pElement->setPos(iMaximumWidth - iPreviewWidth, iMargin);

                /* Calculate required size: */
                int iWidth = iPreviewWidth;
                int iHeight = iPreviewHeight;
                if (!pElement->isAnimationRunning() && !pElement->isClosed())
                    iHeight += iAdditionalPreviewHeight;
                /* Resize element to required size: */
                pElement->resize(iWidth, iHeight);

                /* Layout element content: */
                pItem->updateLayout();

                break;
            }
            default: AssertFailed(); break; /* Shut up, MSC! */
        }
    }
}

int UIDetailsSet::minimumWidthHint() const
{
    /* Zero if has no details: */
    if (!hasDetails())
        return 0;

    /* Prepare variables: */
    const int iSpacing = data(SetData_Spacing).toInt();
    int iMinimumWidthHint = 0;

    /* Take into account all the elements: */
    foreach (UIDetailsItem *pItem, items())
    {
        /* Skip hidden: */
        if (!pItem->isVisible())
            continue;

        /* For each particular element: */
        UIDetailsElement *pElement = pItem->toElement();
        switch (pElement->elementType())
        {
            case DetailsElementType_General:
            case DetailsElementType_System:
            case DetailsElementType_Display:
            case DetailsElementType_Storage:
            case DetailsElementType_Audio:
            case DetailsElementType_Network:
            case DetailsElementType_Serial:
            case DetailsElementType_USB:
            case DetailsElementType_SF:
            case DetailsElementType_UI:
            case DetailsElementType_Description:
            {
                iMinimumWidthHint = qMax(iMinimumWidthHint, pItem->minimumWidthHint());
                break;
            }
            case DetailsElementType_Preview:
            {
                UIDetailsItem *pGeneralItem = element(DetailsElementType_General);
                UIDetailsItem *pSystemItem = element(DetailsElementType_System);
                int iGeneralElementWidth = pGeneralItem ? pGeneralItem->minimumWidthHint() : 0;
                int iSystemElementWidth = pSystemItem ? pSystemItem->minimumWidthHint() : 0;
                int iFirstColumnWidth = qMax(iGeneralElementWidth, iSystemElementWidth);
                iMinimumWidthHint = qMax(iMinimumWidthHint, iFirstColumnWidth + iSpacing + pItem->minimumWidthHint());
                break;
            }
            default: AssertFailed(); break; /* Shut up, MSC! */
        }
    }

    /* Return result: */
    return iMinimumWidthHint;
}

int UIDetailsSet::minimumHeightHint() const
{
    /* Zero if has no details: */
    if (!hasDetails())
        return 0;

    /* Prepare variables: */
    const int iMargin = data(SetData_Margin).toInt();
    const int iSpacing = data(SetData_Spacing).toInt();

    /* Calculate Preview group elements: */
    QList<DetailsElementType> inGroup;
    QList<DetailsElementType> outGroup;
    int iAdditionalGroupHeight = 0;
    int iAdditionalPreviewHeight = 0;
    enumerateLayoutItems(inGroup, outGroup, iAdditionalGroupHeight, iAdditionalPreviewHeight);

    /* Take into account all the elements: */
    int iMinimumHeightHintInGroup = 0;
    int iMinimumHeightHintOutGroup = 0;
    int iMinimumHeightHintPreview = 0;
    foreach (UIDetailsItem *pItem, items())
    {
        /* Skip hidden: */
        if (!pItem->isVisible())
            continue;

        /* For each particular element: */
        UIDetailsElement *pElement = pItem->toElement();
        const DetailsElementType enmElementType = pElement->elementType();
        switch (enmElementType)
        {
            case DetailsElementType_General:
            case DetailsElementType_System:
            case DetailsElementType_Display:
            case DetailsElementType_Storage:
            case DetailsElementType_Audio:
            case DetailsElementType_Network:
            case DetailsElementType_Serial:
            case DetailsElementType_USB:
            case DetailsElementType_SF:
            case DetailsElementType_UI:
            case DetailsElementType_Description:
            {
                if (inGroup.contains(enmElementType))
                {
                    iMinimumHeightHintInGroup += (pItem->minimumHeightHint() + iSpacing);
                    if (inGroup.last() == enmElementType)
                        iMinimumHeightHintInGroup += iAdditionalGroupHeight;
                }
                else if (outGroup.contains(enmElementType))
                    iMinimumHeightHintOutGroup += (pItem->minimumHeightHint() + iSpacing);
                break;
            }
            case DetailsElementType_Preview:
            {
                iMinimumHeightHintPreview = pItem->minimumHeightHint() + iAdditionalPreviewHeight;
                break;
            }
            default: AssertFailed(); break; /* Shut up, MSC! */
        }
    }

    /* Minus last spacing: */
    iMinimumHeightHintInGroup -= iSpacing;
    iMinimumHeightHintOutGroup -= iSpacing;

    /* Calculate minimum height hint: */
    int iMinimumHeightHint = qMax(iMinimumHeightHintInGroup, iMinimumHeightHintPreview);

    /* Spacing if necessary: */
    if (!inGroup.isEmpty() && !outGroup.isEmpty())
        iMinimumHeightHint += iSpacing;

    /* Spacing if necessary: */
    if (!outGroup.isEmpty())
        iMinimumHeightHint += iMinimumHeightHintOutGroup;

    /* And two margins finally: */
    iMinimumHeightHint += 2 * iMargin;

    /* Return result: */
    return iMinimumHeightHint;
}

void UIDetailsSet::sltMachineStateChange(const QUuid &uId)
{
    /* For local VMs only: */
    if (!m_fIsLocal)
        return;

    /* Make sure VM is set: */
    if (m_comMachine.isNull())
        return;

    /* Is this our VM changed? */
    if (m_comMachine.GetId() != uId)
        return;

    /* Update appearance: */
    rebuildSet();
}

void UIDetailsSet::sltMachineAttributesChange(const QUuid &uId)
{
    /* For local VMs only: */
    if (!m_fIsLocal)
        return;

    /* Make sure VM is set: */
    if (m_comMachine.isNull())
        return;

    /* Is this our VM changed? */
    if (m_comMachine.GetId() != uId)
        return;

    /* Update appearance: */
    rebuildSet();
}

void UIDetailsSet::sltMediumEnumerated(const QUuid &uId)
{
    /* For local VMs only: */
    if (!m_fIsLocal)
        return;

    /* Make sure VM is set: */
    if (m_comMachine.isNull())
        return;

    /* Is this our medium changed? */
    const UIMedium guiMedium = uiCommon().medium(uId);
    if (   guiMedium.isNull()
        || !guiMedium.machineIds().contains(m_comMachine.GetId()))
        return;

    /* Update appearance: */
    rebuildSet();
}

void UIDetailsSet::prepareSet()
{
    /* Setup size-policy: */
    setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
}

void UIDetailsSet::prepareConnections()
{
    /* Global-events connections: */
    connect(gVBoxEvents, &UIVirtualBoxEventHandler::sigMachineStateChange, this, &UIDetailsSet::sltMachineStateChange);
    connect(gVBoxEvents, &UIVirtualBoxEventHandler::sigMachineDataChange, this, &UIDetailsSet::sltMachineAttributesChange);
    connect(gVBoxEvents, &UIVirtualBoxEventHandler::sigSessionStateChange, this, &UIDetailsSet::sltMachineAttributesChange);
    connect(gVBoxEvents, &UIVirtualBoxEventHandler::sigSnapshotTake, this, &UIDetailsSet::sltMachineAttributesChange);
    connect(gVBoxEvents, &UIVirtualBoxEventHandler::sigSnapshotDelete, this, &UIDetailsSet::sltMachineAttributesChange);
    connect(gVBoxEvents, &UIVirtualBoxEventHandler::sigSnapshotChange, this, &UIDetailsSet::sltMachineAttributesChange);
    connect(gVBoxEvents, &UIVirtualBoxEventHandler::sigSnapshotRestore, this, &UIDetailsSet::sltMachineAttributesChange);

    /* Meidum-enumeration connections: */
    connect(&uiCommon(), &UICommon::sigMediumEnumerated, this, &UIDetailsSet::sltMediumEnumerated);
}

QVariant UIDetailsSet::data(int iKey) const
{
    /* Provide other members with required data: */
    switch (iKey)
    {
        /* Layout hints: */
        case SetData_Margin: return 1;
        case SetData_Spacing: return 1;
        /* Default: */
        default: break;
    }
    return QVariant();
}

void UIDetailsSet::rebuildSet()
{
    /* Make sure we have details: */
    if (!m_fHasDetails)
        return;

    /* Recache properties: */
    m_configurationAccessLevel = m_pMachineItem->configurationAccessLevel();

    /* Cleanup build-step: */
    delete m_pBuildStep;
    m_pBuildStep = 0;

    /* Generate new set-id: */
    m_uSetId = QUuid::createUuid();

    /* Request to build first step: */
    emit sigBuildStep(m_uSetId, 0);
}

UIDetailsElement *UIDetailsSet::createElement(DetailsElementType enmElementType, bool fOpen)
{
    /* Element factory: */
    switch (enmElementType)
    {
        case DetailsElementType_General:     return new UIDetailsElementGeneral(this, fOpen);
        case DetailsElementType_System:      return new UIDetailsElementSystem(this, fOpen);
        case DetailsElementType_Preview:     return new UIDetailsElementPreview(this, fOpen);
        case DetailsElementType_Display:     return new UIDetailsElementDisplay(this, fOpen);
        case DetailsElementType_Storage:     return new UIDetailsElementStorage(this, fOpen);
        case DetailsElementType_Audio:       return new UIDetailsElementAudio(this, fOpen);
        case DetailsElementType_Network:     return new UIDetailsElementNetwork(this, fOpen);
        case DetailsElementType_Serial:      return new UIDetailsElementSerial(this, fOpen);
        case DetailsElementType_USB:         return new UIDetailsElementUSB(this, fOpen);
        case DetailsElementType_SF:          return new UIDetailsElementSF(this, fOpen);
        case DetailsElementType_UI:          return new UIDetailsElementUI(this, fOpen);
        case DetailsElementType_Description: return new UIDetailsElementDescription(this, fOpen);
        default:                             AssertFailed(); break; /* Shut up, MSC! */
    }
    return 0;
}

void UIDetailsSet::enumerateLayoutItems(QList<DetailsElementType> &inGroup,
                                        QList<DetailsElementType> &outGroup,
                                        int &iAdditionalGroupHeight,
                                        int &iAdditionalPreviewHeight) const
{
    /* Prepare variables: */
    const int iSpacing = data(SetData_Spacing).toInt();
    const UIDetailsElement *pPreviewElement = element(DetailsElementType_Preview);
    const bool fPreviewVisible = pPreviewElement && pPreviewElement->isVisible();
    const int iPreviewHeight = fPreviewVisible ? pPreviewElement->minimumHeightHint() : 0;
    int iGroupHeight = 0;

    /* Enumerate all the items: */
    foreach (UIDetailsItem *pItem, items())
    {
        /* Skip hidden: */
        if (!pItem->isVisible())
            continue;

        /* Acquire element and its type: */
        const UIDetailsElement *pElement = pItem->toElement();
        const DetailsElementType enmElementType = pElement->elementType();

        /* Skip Preview: */
        if (enmElementType == DetailsElementType_Preview)
            continue;

        /* Acquire element height: */
        const int iElementHeight = pElement->minimumHeightHint();

        /* Advance cumulative height if necessary: */
        if (   fPreviewVisible
            && outGroup.isEmpty()
            && (   iGroupHeight == 0
                || iGroupHeight + iElementHeight / 2 < iPreviewHeight))
        {
            iGroupHeight += iElementHeight;
            iGroupHeight += iSpacing;
            inGroup << enmElementType;
        }
        else
            outGroup << enmElementType;
    }
    /* Minus last spacing: */
    iGroupHeight -= iSpacing;

    /* Calculate additional height: */
    if (iPreviewHeight > iGroupHeight)
        iAdditionalGroupHeight = iPreviewHeight - iGroupHeight;
    else
        iAdditionalPreviewHeight = iGroupHeight - iPreviewHeight;
}

void UIDetailsSet::paintBackground(QPainter *pPainter, const QStyleOptionGraphicsItem *pOptions) const
{
    /* Save painter: */
    pPainter->save();

    /* Prepare variables: */
    const QRect optionRect = pOptions->rect;

    /* Paint default background: */
    const QColor defaultColor = palette().color(QPalette::Active, QPalette::Midlight).darker(110);
    pPainter->fillRect(optionRect, defaultColor);

    /* Restore painter: */
    pPainter->restore();
}
