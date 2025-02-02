/* $Id: UIToolsModel.cpp 86668 2020-10-21 16:15:11Z vboxsync $ */
/** @file
 * VBox Qt GUI - UIToolsModel class implementation.
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
#include <QGraphicsScene>
#include <QGraphicsSceneContextMenuEvent>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsView>
#include <QPropertyAnimation>
#include <QScrollBar>
#include <QRegExp>
#include <QTimer>

/* GUI includes: */
#include "QIMessageBox.h"
#include "UICommon.h"
#include "UIActionPoolManager.h"
#include "UIIconPool.h"
#include "UITools.h"
#include "UIToolsHandlerMouse.h"
#include "UIToolsHandlerKeyboard.h"
#include "UIToolsModel.h"
#include "UIExtraDataDefs.h"
#include "UIExtraDataManager.h"
#include "UIMessageCenter.h"
#include "UIModalWindowManager.h"
#include "UIVirtualBoxManagerWidget.h"
#include "UIVirtualBoxEventHandler.h"
#include "UIWizardNewVM.h"

/* COM includes: */
#include "CExtPack.h"
#include "CExtPackManager.h"

/* Qt includes: */
#include <QParallelAnimationGroup>

/* Type defs: */
typedef QSet<QString> UIStringSet;


UIToolsModel::UIToolsModel(UITools *pParent)
    : QIWithRetranslateUI3<QObject>(pParent)
    , m_pTools(pParent)
    , m_pScene(0)
    , m_pMouseHandler(0)
    , m_pKeyboardHandler(0)
    , m_enmCurrentClass(UIToolClass_Global)
{
    /* Prepare: */
    prepare();
}

UIToolsModel::~UIToolsModel()
{
    /* Cleanup: */
    cleanup();
}

void UIToolsModel::init()
{
    /* Load last selected item: */
    loadLastSelectedItems();

    /* Update linked values: */
    updateLayout();
    updateNavigation();
    sltItemMinimumWidthHintChanged();
    sltItemMinimumHeightHintChanged();
}

void UIToolsModel::deinit()
{
    /* Save last selected item: */
    saveLastSelectedItems();
}

UITools *UIToolsModel::tools() const
{
    return m_pTools;
}

UIActionPool *UIToolsModel::actionPool() const
{
    return tools()->actionPool();
}

QGraphicsScene *UIToolsModel::scene() const
{
    return m_pScene;
}

QPaintDevice *UIToolsModel::paintDevice() const
{
    if (scene() && !scene()->views().isEmpty())
        return scene()->views().first();
    return 0;
}

QGraphicsItem *UIToolsModel::itemAt(const QPointF &position, const QTransform &deviceTransform /* = QTransform() */) const
{
    return scene()->itemAt(position, deviceTransform);
}

void UIToolsModel::setToolsClass(UIToolClass enmClass)
{
    /* Update linked values: */
    if (m_enmCurrentClass != enmClass)
    {
        m_enmCurrentClass = enmClass;
        updateLayout();
        updateNavigation();
        sltItemMinimumHeightHintChanged();
    }
}

UIToolClass UIToolsModel::toolsClass() const
{
    return m_enmCurrentClass;
}

void UIToolsModel::setToolsType(UIToolType enmType)
{
    /* Update linked values: */
    if (currentItem()->itemType() != enmType)
    {
        foreach (UIToolsItem *pItem, items())
            if (pItem->itemType() == enmType)
            {
                setCurrentItem(pItem);
                break;
            }
    }
}

UIToolType UIToolsModel::toolsType() const
{
    return currentItem()->itemType();
}

UIToolType UIToolsModel::lastSelectedToolGlobal() const
{
    return m_pLastItemGlobal->itemType();
}

UIToolType UIToolsModel::lastSelectedToolMachine() const
{
    return m_pLastItemMachine->itemType();
}

void UIToolsModel::setToolsEnabled(UIToolClass enmClass, bool fEnabled)
{
    /* Update linked values: */
    m_statesToolsEnabled[enmClass] = fEnabled;
    foreach (UIToolsItem *pItem, items())
        if (pItem->itemClass() == enmClass)
            pItem->setEnabled(m_statesToolsEnabled.value(enmClass));
}

bool UIToolsModel::areToolsEnabled(UIToolClass enmClass) const
{
    return m_statesToolsEnabled.value(enmClass);
}

void UIToolsModel::setRestrictedToolTypes(const QList<UIToolType> &types)
{
    /* Update linked values: */
    if (m_restrictedToolTypes != types)
    {
        m_restrictedToolTypes = types;
        updateLayout();
        updateNavigation();
        sltItemMinimumWidthHintChanged();
        sltItemMinimumHeightHintChanged();
    }
}

QList<UIToolType> UIToolsModel::restrictedToolTypes() const
{
    return m_restrictedToolTypes;
}

void UIToolsModel::closeParent()
{
    m_pTools->close();
}

void UIToolsModel::setCurrentItem(UIToolsItem *pItem)
{
    /* Is there something changed? */
    if (m_pCurrentItem == pItem)
        return;

    /* Remember old current-item: */
    UIToolsItem *pOldCurrentItem = m_pCurrentItem;

    /* If there is item: */
    if (pItem)
    {
        /* Set this item to current if navigation list contains it: */
        if (navigationList().contains(pItem))
            m_pCurrentItem = pItem;
        /* Update last item in any case: */
        switch (pItem->itemClass())
        {
            case UIToolClass_Global:  m_pLastItemGlobal  = pItem; break;
            case UIToolClass_Machine: m_pLastItemMachine = pItem; break;
            default: break;
        }
    }
    /* Otherwise reset current item: */
    else
        m_pCurrentItem = 0;

    /* Update old item (if any): */
    if (pOldCurrentItem)
        pOldCurrentItem->update();
    /* Update new item (if any): */
    if (m_pCurrentItem)
        m_pCurrentItem->update();

    /* Notify about selection change: */
    emit sigSelectionChanged();

    /* Move focus to current-item: */
    setFocusItem(currentItem());
}

UIToolsItem *UIToolsModel::currentItem() const
{
    return m_pCurrentItem;
}

void UIToolsModel::setFocusItem(UIToolsItem *pItem)
{
    /* Always make sure real focus unset: */
    scene()->setFocusItem(0);

    /* Is there something changed? */
    if (m_pFocusItem == pItem)
        return;

    /* Remember old focus-item: */
    UIToolsItem *pOldFocusItem = m_pFocusItem;

    /* If there is item: */
    if (pItem)
    {
        /* Set this item to focus if navigation list contains it: */
        if (navigationList().contains(pItem))
            m_pFocusItem = pItem;
        /* Otherwise it's error: */
        else
            AssertMsgFailed(("Passed item is not in navigation list!"));
    }
    /* Otherwise reset focus item: */
    else
        m_pFocusItem = 0;

    /* Disconnect old focus-item (if any): */
    if (pOldFocusItem)
        disconnect(pOldFocusItem, &UIToolsItem::destroyed, this, &UIToolsModel::sltFocusItemDestroyed);
    /* Connect new focus-item (if any): */
    if (m_pFocusItem)
        connect(m_pFocusItem.data(), &UIToolsItem::destroyed, this, &UIToolsModel::sltFocusItemDestroyed);

    /* Notify about focus change: */
    emit sigFocusChanged();
}

UIToolsItem *UIToolsModel::focusItem() const
{
    return m_pFocusItem;
}

const QList<UIToolsItem*> &UIToolsModel::navigationList() const
{
    return m_navigationList;
}

void UIToolsModel::removeFromNavigationList(UIToolsItem *pItem)
{
    AssertMsg(pItem, ("Passed item is invalid!"));
    m_navigationList.removeAll(pItem);
}

void UIToolsModel::updateNavigation()
{
    /* Clear list initially: */
    m_navigationList.clear();

    /* Enumerate the children: */
    foreach (UIToolsItem *pItem, items())
        if (pItem->isVisible())
            m_navigationList << pItem;

    /* Choose last selected item of current class: */
    UIToolsItem *pLastSelectedItem = m_enmCurrentClass == UIToolClass_Global
                                   ? m_pLastItemGlobal : m_pLastItemMachine;
    if (navigationList().contains(pLastSelectedItem))
        setCurrentItem(pLastSelectedItem);
}

QList<UIToolsItem*> UIToolsModel::items() const
{
    return m_items;
}

UIToolsItem *UIToolsModel::item(UIToolType enmType) const
{
    foreach (UIToolsItem *pItem, items())
        if (pItem->itemType() == enmType)
            return pItem;
    return 0;
}

void UIToolsModel::updateLayout()
{
    /* Prepare variables: */
    const int iMargin = data(ToolsModelData_Margin).toInt();
    const int iSpacing = data(ToolsModelData_Spacing).toInt();
    const QSize viewportSize = scene()->views()[0]->viewport()->size();
    const int iViewportWidth = viewportSize.width();
    int iVerticalIndent = iMargin;

    /* Layout the children: */
    foreach (UIToolsItem *pItem, items())
    {
        /* Hide/skip unrelated items: */
        if (   pItem->itemClass() != m_enmCurrentClass
            || m_restrictedToolTypes.contains(pItem->itemType()))
        {
            pItem->hide();
            continue;
        }

        /* Set item position: */
        pItem->setPos(iMargin, iVerticalIndent);
        /* Set root-item size: */
        pItem->resize(iViewportWidth, pItem->minimumHeightHint());
        /* Make sure item is shown: */
        pItem->show();
        /* Advance vertical indent: */
        iVerticalIndent += (pItem->minimumHeightHint() + iSpacing);
    }
}

void UIToolsModel::sltHandleViewResized()
{
    /* Relayout: */
    updateLayout();
}

void UIToolsModel::sltItemMinimumWidthHintChanged()
{
    /* Prepare variables: */
    const int iMargin = data(ToolsModelData_Margin).toInt();

    /* Calculate maximum horizontal width: */
    int iMinimumWidthHint = 0;
    iMinimumWidthHint += 2 * iMargin;
    foreach (UIToolsItem *pItem, items())
        iMinimumWidthHint = qMax(iMinimumWidthHint, pItem->minimumWidthHint());

    /* Notify listeners: */
    emit sigItemMinimumWidthHintChanged(iMinimumWidthHint);
}

void UIToolsModel::sltItemMinimumHeightHintChanged()
{
    /* Prepare variables: */
    const int iMargin = data(ToolsModelData_Margin).toInt();
    const int iSpacing = data(ToolsModelData_Spacing).toInt();

    /* Calculate summary vertical height: */
    int iMinimumHeightHint = 0;
    iMinimumHeightHint += 2 * iMargin;
    foreach (UIToolsItem *pItem, items())
        if (pItem->isVisible())
            iMinimumHeightHint += (pItem->minimumHeightHint() + iSpacing);
    iMinimumHeightHint -= iSpacing;

    /* Notify listeners: */
    emit sigItemMinimumHeightHintChanged(iMinimumHeightHint);
}

bool UIToolsModel::eventFilter(QObject *pWatched, QEvent *pEvent)
{
    /* Process only scene events: */
    if (pWatched != scene())
        return QIWithRetranslateUI3<QObject>::eventFilter(pWatched, pEvent);

    /* Process only item focused by model: */
    if (scene()->focusItem())
        return QIWithRetranslateUI3<QObject>::eventFilter(pWatched, pEvent);

    /* Do not handle disabled items: */
    if (!currentItem()->isEnabled())
        return QIWithRetranslateUI3<QObject>::eventFilter(pWatched, pEvent);

    /* Checking event-type: */
    switch (pEvent->type())
    {
        /* Keyboard handler: */
        case QEvent::KeyPress:
            return m_pKeyboardHandler->handle(static_cast<QKeyEvent*>(pEvent), UIKeyboardEventType_Press);
        case QEvent::KeyRelease:
            return m_pKeyboardHandler->handle(static_cast<QKeyEvent*>(pEvent), UIKeyboardEventType_Release);
        /* Mouse handler: */
        case QEvent::GraphicsSceneMousePress:
            return m_pMouseHandler->handle(static_cast<QGraphicsSceneMouseEvent*>(pEvent), UIMouseEventType_Press);
        case QEvent::GraphicsSceneMouseRelease:
            return m_pMouseHandler->handle(static_cast<QGraphicsSceneMouseEvent*>(pEvent), UIMouseEventType_Release);
        /* Shut up MSC: */
        default: break;
    }

    /* Call to base-class: */
    return QIWithRetranslateUI3<QObject>::eventFilter(pWatched, pEvent);
}

void UIToolsModel::retranslateUi()
{
    foreach (UIToolsItem *pItem, m_items)
    {
        switch (pItem->itemType())
        {
            case UIToolType_Welcome:     pItem->reconfigure(tr("Welcome")); break;
            case UIToolType_Media:       pItem->reconfigure(tr("Media")); break;
            case UIToolType_Network:     pItem->reconfigure(tr("Network")); break;
            case UIToolType_Cloud:       pItem->reconfigure(tr("Cloud")); break;
            case UIToolType_Resources:   pItem->reconfigure(tr("Resources")); break;
            case UIToolType_Details:     pItem->reconfigure(tr("Details")); break;
            case UIToolType_Snapshots:   pItem->reconfigure(tr("Snapshots")); break;
            case UIToolType_Logs:        pItem->reconfigure(tr("Logs")); break;
            case UIToolType_Performance: pItem->reconfigure(tr("Performance")); break;
            default: break;
        }
    }
}

void UIToolsModel::sltFocusItemDestroyed()
{
    AssertMsgFailed(("Focus item destroyed!"));
}

void UIToolsModel::prepare()
{
    /* Prepare scene: */
    prepareScene();
    /* Prepare items: */
    prepareItems();
    /* Prepare handlers: */
    prepareHandlers();
    /* Prepare connections: */
    prepareConnections();
    /* Apply language settings: */
    retranslateUi();
}

void UIToolsModel::prepareScene()
{
    m_pScene = new QGraphicsScene(this);
    if (m_pScene)
        m_pScene->installEventFilter(this);
}

void UIToolsModel::prepareItems()
{
    /* Enable both classes of tools initially: */
    m_statesToolsEnabled[UIToolClass_Global] = true;
    m_statesToolsEnabled[UIToolClass_Machine] = true;

    /* Welcome: */
    m_items << new UIToolsItem(scene(), UIToolClass_Global, UIToolType_Welcome, QString(),
                               UIIconPool::iconSet(":/welcome_screen_24px.png", ":/welcome_screen_24px.png"));

    /* Media: */
    m_items << new UIToolsItem(scene(), UIToolClass_Global, UIToolType_Media, QString(),
                               UIIconPool::iconSet(":/media_manager_24px.png", ":/media_manager_disabled_24px.png"));

    /* Network: */
    m_items << new UIToolsItem(scene(), UIToolClass_Global, UIToolType_Network, QString(),
                               UIIconPool::iconSet(":/host_iface_manager_24px.png", ":/host_iface_manager_disabled_24px.png"));

    /* Cloud: */
    m_items << new UIToolsItem(scene(), UIToolClass_Global, UIToolType_Cloud, QString(),
                               UIIconPool::iconSet(":/cloud_profile_manager_24px.png", ":/cloud_profile_manager_disabled_24px.png"));

    /* Resources: */
    m_items << new UIToolsItem(scene(), UIToolClass_Global, UIToolType_Resources, QString(),
                               UIIconPool::iconSet(":/resources_monitor_24px.png", ":/resources_monitor_disabled_24px.png"));

    /* Details: */
    m_items << new UIToolsItem(scene(), UIToolClass_Machine, UIToolType_Details, QString(),
                               UIIconPool::iconSet(":/machine_details_manager_24px.png", ":/machine_details_manager_disabled_24px.png"));

    /* Snapshots: */
    m_items << new UIToolsItem(scene(), UIToolClass_Machine, UIToolType_Snapshots, QString(),
                               UIIconPool::iconSet(":/snapshot_manager_24px.png", ":/snapshot_manager_disabled_24px.png"));

    /* Logs: */
    m_items << new UIToolsItem(scene(), UIToolClass_Machine, UIToolType_Logs, QString(),
                               UIIconPool::iconSet(":/vm_show_logs_24px.png", ":/vm_show_logs_disabled_24px.png"));

    /* Performance: */
    m_items << new UIToolsItem(scene(), UIToolClass_Machine, UIToolType_Performance, QString(),
                               UIIconPool::iconSet(":/performance_monitor_24px.png", ":/performance_monitor_disabled_24px.png"));
}

void UIToolsModel::prepareHandlers()
{
    m_pMouseHandler = new UIToolsHandlerMouse(this);
    m_pKeyboardHandler = new UIToolsHandlerKeyboard(this);
}

void UIToolsModel::prepareConnections()
{
    UITools* pTools = qobject_cast<UITools*>(parent());
    AssertPtrReturnVoid(pTools);
    {
        /* Setup parent connections: */
        connect(this, &UIToolsModel::sigSelectionChanged,
                pTools, &UITools::sigSelectionChanged);
        connect(this, &UIToolsModel::sigExpandingStarted,
                pTools, &UITools::sigExpandingStarted);
        connect(this, &UIToolsModel::sigExpandingFinished,
                pTools, &UITools::sigExpandingFinished);
    }
}

void UIToolsModel::loadLastSelectedItems()
{
    /* Load selected items data: */
    const QList<UIToolType> data = gEDataManager->toolsPaneLastItemsChosen();

    /* First of them is current global class item definition: */
    UIToolType enmTypeGlobal = data.value(0);
    if (!UIToolStuff::isTypeOfClass(enmTypeGlobal, UIToolClass_Global))
        enmTypeGlobal = UIToolType_Welcome;
    foreach (UIToolsItem *pItem, items())
        if (pItem->itemType() == enmTypeGlobal)
            m_pLastItemGlobal = pItem;
    if (m_pLastItemGlobal.isNull())
        m_pLastItemGlobal = item(UIToolType_Welcome);

    /* Second of them is current machine class item definition: */
    UIToolType enmTypeMachine = data.value(1);
    if (!UIToolStuff::isTypeOfClass(enmTypeMachine, UIToolClass_Machine))
        enmTypeMachine = UIToolType_Details;
    foreach (UIToolsItem *pItem, items())
        if (pItem->itemType() == enmTypeMachine)
            m_pLastItemMachine = pItem;
    if (m_pLastItemMachine.isNull())
        m_pLastItemMachine = item(UIToolType_Details);
}

void UIToolsModel::saveLastSelectedItems()
{
    /* Prepare selected items data: */
    const QList<UIToolType> set = QList<UIToolType>() << m_pLastItemGlobal->itemType() << m_pLastItemMachine->itemType();

    /* Save selected items data: */
    gEDataManager->setToolsPaneLastItemsChosen(set);
}

void UIToolsModel::cleanupConnections()
{
    /* Disconnect selection-changed signal prematurelly.
     * Keep in mind, we are using static_cast instead of qobject_cast here to be
     * sure connection is disconnected even if parent is self-destroyed. */
    disconnect(this, &UIToolsModel::sigSelectionChanged,
               static_cast<UITools*>(parent()), &UITools::sigSelectionChanged);
}

void UIToolsModel::cleanupHandlers()
{
    delete m_pKeyboardHandler;
    m_pKeyboardHandler = 0;
    delete m_pMouseHandler;
    m_pMouseHandler = 0;
}

void UIToolsModel::cleanupItems()
{
    foreach (UIToolsItem *pItem, m_items)
        delete pItem;
    m_items.clear();
}

void UIToolsModel::cleanupScene()
{
    delete m_pScene;
    m_pScene = 0;
}

void UIToolsModel::cleanup()
{
    /* Cleanup connections: */
    cleanupConnections();
    /* Cleanup handlers: */
    cleanupHandlers();
    /* Cleanup items: */
    cleanupItems();
    /* Cleanup scene: */
    cleanupScene();
}

QVariant UIToolsModel::data(int iKey) const
{
    /* Provide other members with required data: */
    switch (iKey)
    {
        /* Layout hints: */
        case ToolsModelData_Margin:  return 0;
        case ToolsModelData_Spacing: return 1;

        /* Default: */
        default: break;
    }
    return QVariant();
}
