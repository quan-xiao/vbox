/* $Id: UISettingsDialog.cpp 86233 2020-09-23 12:10:51Z vboxsync $ */
/** @file
 * VBox Qt GUI - UISettingsDialog class implementation.
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

/* Qt includes: */
#include <QCloseEvent>
#include <QGridLayout>
#include <QLabel>
#include <QProgressBar>
#include <QPushButton>
#include <QStackedWidget>
#include <QTimer>

/* GUI includes: */
#include "QIDialogButtonBox.h"
#include "QIWidgetValidator.h"
#include "UICommon.h"
#include "UIConverter.h"
#include "UIIconPool.h"
#include "UIMessageCenter.h"
#include "UIModalWindowManager.h"
#include "UIPopupCenter.h"
#include "UISettingsDialog.h"
#include "UISettingsPage.h"
#include "UISettingsSelector.h"
#include "UISettingsSerializer.h"
#include "QIToolBar.h"
#include "UIWarningPane.h"
#ifdef VBOX_WS_MAC
# include "VBoxUtils.h"
#endif

#ifdef VBOX_WS_MAC
# define VBOX_GUI_WITH_TOOLBAR_SETTINGS
#endif


UISettingsDialog::UISettingsDialog(QWidget *pParent)
    : QIWithRetranslateUI<QIMainDialog>(pParent)
    , m_pSelector(0)
    , m_pStack(0)
    , m_enmConfigurationAccessLevel(ConfigurationAccessLevel_Null)
    , m_pSerializeProcess(0)
    , m_fSerializationIsInProgress(false)
    , m_fSerializationClean(true)
    , m_pStatusBar(0)
    , m_pProcessBar(0)
    , m_pWarningPane(0)
    , m_fValid(true)
    , m_fSilent(true)
    , m_pWhatsThisTimer(new QTimer(this))
    , m_pLabelTitle(0)
    , m_pButtonBox(0)
    , m_pWidgetStackHandler(0)
{
    /* Prepare: */
    prepare();
}

UISettingsDialog::~UISettingsDialog()
{
    /* Delete serializer if exists: */
    if (serializeProcess())
    {
        delete m_pSerializeProcess;
        m_pSerializeProcess = 0;
    }

    /* Recall popup-pane if any: */
    popupCenter().recall(m_pStack, "SettingsDialogWarning");

    /* Delete selector early! */
    delete m_pSelector;
}

void UISettingsDialog::execute()
{
    /* Load data: */
    loadOwnData();

    /* Execute dialog: */
    exec();
}

void UISettingsDialog::accept()
{
    /* Save data: */
    saveOwnData();

    /* If serialization was clean: */
    if (m_fSerializationClean)
    {
        /* Call to base-class: */
        QIWithRetranslateUI<QIMainDialog>::accept();
    }
}

void UISettingsDialog::reject()
{
    if (!isSerializationInProgress())
        QIWithRetranslateUI<QIMainDialog>::reject();
}

void UISettingsDialog::sltCategoryChanged(int cId)
{
    const int iIndex = m_pages.value(cId);

#ifdef VBOX_WS_MAC
    /* If index is within the stored size list bounds: */
    if (iIndex < m_sizeList.count())
    {
        /* Get current/stored size: */
        const QSize cs = size();
        const QSize ss = m_sizeList.at(iIndex);

        /* Switch to the new page first if we are shrinking: */
        if (cs.height() > ss.height())
            m_pStack->setCurrentIndex(iIndex);

        /* Do the animation: */
        ::darwinWindowAnimateResize(this, QRect (x(), y(), ss.width(), ss.height()));

        /* Switch to the new page last if we are zooming: */
        if (cs.height() <= ss.height())
            m_pStack->setCurrentIndex(iIndex);

        /* Unlock all page policies but lock the current one: */
        for (int i = 0; i < m_pStack->count(); ++i)
            m_pStack->widget(i)->setSizePolicy(QSizePolicy::Minimum, i == iIndex ? QSizePolicy::Minimum : QSizePolicy::Ignored);

        /* And make sure layouts are freshly calculated: */
        foreach (QLayout *pLayout, findChildren<QLayout*>())
        {
            pLayout->update();
            pLayout->activate();
        }
    }
#else /* !VBOX_WS_MAC */
    m_pStack->setCurrentIndex(iIndex);
#endif /* !VBOX_WS_MAC */

#ifdef VBOX_GUI_WITH_TOOLBAR_SETTINGS
    setWindowTitle(title());
#else
    m_pLabelTitle->setText(m_pSelector->itemText(cId));
#endif
}

void UISettingsDialog::sltMarkLoaded()
{
    /* Delete serializer if exists: */
    if (serializeProcess())
    {
        delete m_pSerializeProcess;
        m_pSerializeProcess = 0;
    }

    /* Mark serialization finished: */
    m_fSerializationIsInProgress = false;
}

void UISettingsDialog::sltMarkSaved()
{
    /* Delete serializer if exists: */
    if (serializeProcess())
    {
        delete m_pSerializeProcess;
        m_pSerializeProcess = 0;
    }

    /* Mark serialization finished: */
    m_fSerializationIsInProgress = false;
}

void UISettingsDialog::sltHandleProcessStarted()
{
    m_pProcessBar->setValue(0);
    m_pStatusBar->setCurrentWidget(m_pProcessBar);
}

void UISettingsDialog::sltHandleProcessProgressChange(int iValue)
{
    m_pProcessBar->setValue(iValue);
    if (m_pProcessBar->value() == m_pProcessBar->maximum())
    {
        if (!m_fValid || !m_fSilent)
            m_pStatusBar->setCurrentWidget(m_pWarningPane);
        else
            m_pStatusBar->setCurrentIndex(0);
    }
}

bool UISettingsDialog::eventFilter(QObject *pObject, QEvent *pEvent)
{
    /* Ignore objects which are NOT widgets: */
    if (!pObject->isWidgetType())
        return QIMainDialog::eventFilter(pObject, pEvent);

    /* Ignore widgets which window is NOT settings window: */
    QWidget *pWidget = static_cast<QWidget*>(pObject);
    if (pWidget->window() != this)
        return QIMainDialog::eventFilter(pObject, pEvent);

    /* Process different event-types: */
    switch (pEvent->type())
    {
        /* Process enter/leave events to remember whats-this candidates: */
        case QEvent::Enter:
        case QEvent::Leave:
        {
            if (pEvent->type() == QEvent::Enter)
                m_pWhatsThisCandidate = pWidget;
            else
                m_pWhatsThisCandidate = 0;

            m_pWhatsThisTimer->start(100);
            break;
        }
        /* Process focus-in event to update whats-this pane: */
        case QEvent::FocusIn:
        {
            sltUpdateWhatsThis(true /* got focus? */);
            break;
        }
        default:
            break;
    }

    /* Base-class processing: */
    return QIMainDialog::eventFilter(pObject, pEvent);
}

void UISettingsDialog::retranslateUi()
{
    setWhatsThis(tr("<i>Select a settings category from the list on the left-hand side and move the mouse over a settings "
                    "item to get more information.</i>"));
    m_pLabelTitle->setText(QString());

    /* Translate warning stuff: */
    m_strWarningHint = tr("Invalid settings detected");
    if (!m_fValid || !m_fSilent)
        m_pWarningPane->setWarningLabel(m_strWarningHint);

#ifndef VBOX_GUI_WITH_TOOLBAR_SETTINGS
    /* Retranslate current page headline: */
    m_pLabelTitle->setText(m_pSelector->itemText(m_pSelector->currentId()));
#endif

    /* Retranslate all validators: */
    foreach (UIPageValidator *pValidator, findChildren<UIPageValidator*>())
        if (!pValidator->lastMessage().isEmpty())
            revalidate(pValidator);
    revalidate();
}

void UISettingsDialog::polishEvent(QShowEvent *pEvent)
{
    /* Check what's the minimum selector size: */
    const int iMinWidth = m_pSelector->minWidth();

#ifdef VBOX_WS_MAC

    /* Remove all title bar buttons (Buggy Qt): */
    ::darwinSetHidesAllTitleButtons(this);

    /* Unlock all page policies initially: */
    for (int i = 0; i < m_pStack->count(); ++i)
        m_pStack->widget(i)->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Ignored);

    /* Activate every single page to get the optimal size: */
    for (int i = m_pStack->count() - 1; i >= 0; --i)
    {
        /* Activate current page: */
        m_pStack->setCurrentIndex(i);

        /* Lock current page policy temporary: */
        m_pStack->widget(i)->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
        /* And make sure layouts are freshly calculated: */
        foreach (QLayout *pLayout, findChildren<QLayout*>())
        {
            pLayout->update();
            pLayout->activate();
        }

        /* Acquire minimum size-hint: */
        QSize s = minimumSizeHint();
        // WORKAROUND:
        // Take into account the height of native tool-bar title.
        // It will be applied only after widget is really shown.
        // The height is 11pix * 2 (possible HiDPI support).
        s.setHeight(s.height() + 11 * 2);
        /* Also make sure that width is no less than tool-bar: */
        if (iMinWidth > s.width())
            s.setWidth(iMinWidth);
        /* And remember the size finally: */
        m_sizeList.insert(0, s);

        /* Unlock the policy for current page again: */
        m_pStack->widget(i)->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Ignored);
    }

    sltCategoryChanged(m_pSelector->currentId());

#else /* VBOX_WS_MAC */

    /* Resize to the minimum possible size: */
    QSize s = minimumSize();
    if (iMinWidth > s.width())
        s.setWidth(iMinWidth);
    resize(s);

#endif /* VBOX_WS_MAC */

    /* Call to base-class: */
    QIWithRetranslateUI<QIMainDialog>::polishEvent(pEvent);
}

void UISettingsDialog::loadData(QVariant &data)
{
    /* Mark serialization started: */
    m_fSerializationIsInProgress = true;

    /* Create settings loader: */
    m_pSerializeProcess = new UISettingsSerializer(this, UISettingsSerializer::Load,
                                                   data, m_pSelector->settingPages());
    AssertPtrReturnVoid(m_pSerializeProcess);
    {
        /* Configure settings loader: */
        connect(m_pSerializeProcess, &UISettingsSerializer::sigNotifyAboutProcessStarted, this, &UISettingsDialog::sltHandleProcessStarted);
        connect(m_pSerializeProcess, &UISettingsSerializer::sigNotifyAboutProcessProgressChanged, this, &UISettingsDialog::sltHandleProcessProgressChange);
        connect(m_pSerializeProcess, &UISettingsSerializer::sigNotifyAboutProcessFinished, this, &UISettingsDialog::sltMarkLoaded);

        /* Raise current page priority: */
        m_pSerializeProcess->raisePriorityOfPage(m_pSelector->currentId());

        /* Start settings loader: */
        m_pSerializeProcess->start();

        /* Upload data finally: */
        data = m_pSerializeProcess->data();
    }
}

void UISettingsDialog::saveData(QVariant &data)
{
    /* Mark serialization started: */
    m_fSerializationIsInProgress = true;

    /* Create the 'settings saver': */
    QPointer<UISettingsSerializerProgress> pDlgSerializeProgress =
        new UISettingsSerializerProgress(this, UISettingsSerializer::Save,
                                         data, m_pSelector->settingPages());
    AssertPtrReturnVoid(static_cast<UISettingsSerializerProgress*>(pDlgSerializeProgress));
    {
        /* Make the 'settings saver' temporary parent for all sub-dialogs: */
        windowManager().registerNewParent(pDlgSerializeProgress, windowManager().realParentWindow(this));

        /* Execute the 'settings saver': */
        pDlgSerializeProgress->exec();

        /* Any modal dialog can be destroyed in own event-loop
         * as a part of application termination procedure..
         * We have to check if the dialog still valid. */
        if (pDlgSerializeProgress)
        {
            /* Remember whether the serialization was clean: */
            m_fSerializationClean = pDlgSerializeProgress->isClean();

            /* Upload 'settings saver' data: */
            data = pDlgSerializeProgress->data();

            /* Delete the 'settings saver': */
            delete pDlgSerializeProgress;
        }
    }
}

void UISettingsDialog::setConfigurationAccessLevel(ConfigurationAccessLevel enmConfigurationAccessLevel)
{
    /* Make sure something changed: */
    if (m_enmConfigurationAccessLevel == enmConfigurationAccessLevel)
        return;

    /* Apply new configuration access level: */
    m_enmConfigurationAccessLevel = enmConfigurationAccessLevel;

    /* And propagate it to settings-page(s): */
    foreach (UISettingsPage *pPage, m_pSelector->settingPages())
        pPage->setConfigurationAccessLevel(configurationAccessLevel());
}

void UISettingsDialog::addItem(const QString &strBigIcon,
                               const QString &strMediumIcon,
                               const QString &strSmallIcon,
                               int cId,
                               const QString &strLink,
                               UISettingsPage *pSettingsPage /* = 0 */,
                               int iParentId /* = -1 */)
{
    /* Add new selector item: */
    if (QWidget *pPage = m_pSelector->addItem(strBigIcon, strMediumIcon, strSmallIcon,
                                              cId, strLink, pSettingsPage, iParentId))
    {
        /* Add stack-widget page if created: */
        m_pages[cId] = m_pStack->addWidget(pPage);
    }
    /* Assign validator if necessary: */
    if (pSettingsPage)
    {
        pSettingsPage->setId(cId);
        assignValidator(pSettingsPage);
    }
}

void UISettingsDialog::revalidate(UIPageValidator *pValidator)
{
    /* Perform page revalidation: */
    UISettingsPage *pSettingsPage = pValidator->page();
    QList<UIValidationMessage> messages;
    bool fIsValid = pSettingsPage->validate(messages);

    /* Remember revalidation result: */
    pValidator->setValid(fIsValid);

    /* Remember warning/error message: */
    if (messages.isEmpty())
        pValidator->setLastMessage(QString());
    else
    {
        /* Prepare title prefix: */
        // Its the only thing preventing us from moving this method to validator.
        const QString strTitlePrefix(m_pSelector->itemTextByPage(pSettingsPage));
        /* Prepare text: */
        QStringList text;
        foreach (const UIValidationMessage &message, messages)
        {
            /* Prepare title: */
            const QString strTitle(message.first.isNull() ? tr("<b>%1</b> page:").arg(strTitlePrefix) :
                                                            tr("<b>%1: %2</b> page:").arg(strTitlePrefix, message.first));
            /* Prepare paragraph: */
            QStringList paragraph(message.second);
            paragraph.prepend(strTitle);
            /* Format text for iterated message: */
            text << paragraph.join("<br>");
        }
        /* Remember text: */
        pValidator->setLastMessage(text.join("<br><br>"));
        LogRelFlow(("Settings Dialog:  Page validation FAILED: {%s}\n",
                    pValidator->lastMessage().toUtf8().constData()));
    }
}

void UISettingsDialog::revalidate()
{
    /* Perform dialog revalidation: */
    m_fValid = true;
    m_fSilent = true;
    m_pWarningPane->setWarningLabel(QString());

    /* Enumerating all the validators we have: */
    QList<UIPageValidator*> validators(findChildren<UIPageValidator*>());
    foreach (UIPageValidator *pValidator, validators)
    {
        /* Is current validator have something to say? */
        if (!pValidator->lastMessage().isEmpty())
        {
            /* What page is it related to? */
            UISettingsPage *pFailedSettingsPage = pValidator->page();
            LogRelFlow(("Settings Dialog:  Dialog validation FAILED: Page *%s*\n",
                        pFailedSettingsPage->internalName().toUtf8().constData()));

            /* Show error first: */
            if (!pValidator->isValid())
                m_fValid = false;
            /* Show warning if message is not an error: */
            else
                m_fSilent = false;

            /* Configure warning-pane label: */
            m_pWarningPane->setWarningLabel(m_strWarningHint);

            /* Stop dialog revalidation on first error/warning: */
            break;
        }
    }

    /* Make sure warning-pane visible if necessary: */
    if ((!m_fValid || !m_fSilent) && m_pStatusBar->currentIndex() == 0)
        m_pStatusBar->setCurrentWidget(m_pWarningPane);
    /* Make sure empty-pane visible otherwise: */
    else if (m_fValid && m_fSilent && m_pStatusBar->currentWidget() == m_pWarningPane)
        m_pStatusBar->setCurrentIndex(0);

    /* Lock/unlock settings-page OK button according global validity status: */
    m_pButtonBox->button(QDialogButtonBox::Ok)->setEnabled(m_fValid);
}

void UISettingsDialog::sltHandleValidityChange(UIPageValidator *pValidator)
{
    /* Determine which settings-page had called for revalidation: */
    if (UISettingsPage *pSettingsPage = pValidator->page())
    {
        /* Determine settings-page name: */
        const QString strPageName(pSettingsPage->internalName());

        LogRelFlow(("Settings Dialog: %s Page: Revalidation in progress..\n",
                    strPageName.toUtf8().constData()));

        /* Perform page revalidation: */
        revalidate(pValidator);
        /* Perform inter-page recorrelation: */
        recorrelate(pSettingsPage);
        /* Perform dialog revalidation: */
        revalidate();

        LogRelFlow(("Settings Dialog: %s Page: Revalidation complete.\n",
                    strPageName.toUtf8().constData()));
    }
}

void UISettingsDialog::sltHandleWarningPaneHovered(UIPageValidator *pValidator)
{
    LogRelFlow(("Settings Dialog: Warning-icon hovered: %s.\n", pValidator->internalName().toUtf8().constData()));

    /* Show corresponding popup: */
    if (!m_fValid || !m_fSilent)
    {
        popupCenter().popup(m_pStack, "SettingsDialogWarning",
                            pValidator->lastMessage());
    }
}

void UISettingsDialog::sltHandleWarningPaneUnhovered(UIPageValidator *pValidator)
{
    LogRelFlow(("Settings Dialog: Warning-icon unhovered: %s.\n", pValidator->internalName().toUtf8().constData()));

    /* Recall corresponding popup: */
    popupCenter().recall(m_pStack, "SettingsDialogWarning");
}

void UISettingsDialog::sltUpdateWhatsThis(bool fGotFocus)
{
    QString strWhatsThisText;
    QWidget *pWhatsThisWidget = 0;

    /* If focus had NOT changed: */
    if (!fGotFocus)
    {
        /* We will use the recommended candidate: */
        if (m_pWhatsThisCandidate && m_pWhatsThisCandidate != this)
            pWhatsThisWidget = m_pWhatsThisCandidate;
    }
    /* If focus had changed: */
    else
    {
        /* We will use the focused widget instead: */
        pWhatsThisWidget = QApplication::focusWidget();
    }

    /* If the given widget lacks the whats-this text, look at its parent: */
    while (pWhatsThisWidget && pWhatsThisWidget != this)
    {
        strWhatsThisText = pWhatsThisWidget->whatsThis();
        if (!strWhatsThisText.isEmpty())
            break;
        pWhatsThisWidget = pWhatsThisWidget->parentWidget();
    }

    if (pWhatsThisWidget && !strWhatsThisText.isEmpty())
        pWhatsThisWidget->setToolTip(QString("<qt>%1</qt>").arg(strWhatsThisText));
}

void UISettingsDialog::prepare()
{
    prepareWidgets();

    /* Configure title: */
    if (m_pLabelTitle)
    {
        /* Page-title font is bold and larger but derived from the system font: */
        QFont pageTitleFont = font();
        pageTitleFont.setBold(true);
        pageTitleFont.setPointSize(pageTitleFont.pointSize() + 2);
        m_pLabelTitle->setFont(pageTitleFont);
    }

    /* Prepare selector: */
    QGridLayout *pMainLayout = static_cast<QGridLayout*>(centralWidget()->layout());
    if (pMainLayout)
    {
#ifdef VBOX_GUI_WITH_TOOLBAR_SETTINGS

        /* No page-title with tool-bar: */
        m_pLabelTitle->hide();

        /* Create modern tool-bar selector: */
        m_pSelector = new UISettingsSelectorToolBar(this);
        if (m_pSelector)
        {
            /* Configure tool-bar: */
            static_cast<QIToolBar*>(m_pSelector->widget())->enableMacToolbar();

            /* Add tool-bar into page: */
            addToolBar(qobject_cast<QToolBar*>(m_pSelector->widget()));
        }

        /* No title in this mode, we change the title of the window: */
        pMainLayout->setColumnMinimumWidth(0, 0);
        pMainLayout->setHorizontalSpacing(0);

#else /* !VBOX_GUI_WITH_TOOLBAR_SETTINGS */

        /* Create classical tree-view selector: */
        m_pSelector = new UISettingsSelectorTreeView(this);
        if (m_pSelector)
        {
            /* Add into layout: */
            pMainLayout->addWidget(m_pSelector->widget(), 0, 0, 2, 1);

            /* Set focus: */
            m_pSelector->widget()->setFocus();
        }

#endif /* !VBOX_GUI_WITH_TOOLBAR_SETTINGS */

        connect(m_pSelector, &UISettingsSelectorTreeView::sigCategoryChanged, this, &UISettingsDialog::sltCategoryChanged);
    }

    /* Prepare stack-handler: */
    if (m_pWidgetStackHandler)
    {
        /* Create page-stack layout: */
        QVBoxLayout *pStackLayout = new QVBoxLayout(m_pWidgetStackHandler);
        if (pStackLayout)
        {
            /* Confugre page-stack layout: */
            pStackLayout->setContentsMargins(0, 0, 0, 0);

            /* Create page-stack: */
            m_pStack = new QStackedWidget;
            if (m_pStack)
            {
                /* Configure page-stack: */
                popupCenter().setPopupStackOrientation(m_pStack, UIPopupStackOrientation_Bottom);

                /* Add into layout: */
                pStackLayout->addWidget(m_pStack);
            }
        }
    }

    /* Prepare button-box: */
    if (m_pButtonBox)
    {
        m_pButtonBox->button(QDialogButtonBox::Ok)->setDefault(true);
        connect(m_pButtonBox, &QIDialogButtonBox::helpRequested,
                &msgCenter(), &UIMessageCenter::sltShowHelpHelpDialog);

        /* Create status-bar: */
        m_pStatusBar = new QStackedWidget;
        if (m_pStatusBar)
        {
            /* Add empty widget: */
            m_pStatusBar->addWidget(new QWidget);

            /* Create process-bar: */
            m_pProcessBar = new QProgressBar;
            if (m_pProcessBar)
            {
                /* Configure process-bar: */
                m_pProcessBar->setMinimum(0);
                m_pProcessBar->setMaximum(100);

                /* Add into status-bar: */
                m_pStatusBar->addWidget(m_pProcessBar);
            }

            /* Create warning-pane: */
            m_pWarningPane = new UIWarningPane;
            if (m_pWarningPane)
            {
                /* Configure warning-pane: */
                connect(m_pWarningPane, &UIWarningPane::sigHoverEnter,
                        this, &UISettingsDialog::sltHandleWarningPaneHovered);
                connect(m_pWarningPane, &UIWarningPane::sigHoverLeave,
                        this, &UISettingsDialog::sltHandleWarningPaneUnhovered);

                /* Add into status-bar: */
                m_pStatusBar->addWidget(m_pWarningPane);
            }

            /* Add status-bar to button-box: */
            m_pButtonBox->addExtraWidget(m_pStatusBar);
        }
    }

    /* Setup what's this stuff: */
    qApp->installEventFilter(this);
    m_pWhatsThisTimer->setSingleShot(true);
    connect(m_pWhatsThisTimer, &QTimer::timeout,
            this, &UISettingsDialog::sltUpdateWhatsThisNoFocus);

    /* Apply language settings: */
    retranslateUi();
}

void UISettingsDialog::prepareWidgets()
{
    if (objectName().isEmpty())
        setObjectName(QStringLiteral("UISettingsDialog"));
    resize(550, 450);
    QWidget *pCentralWidget = new QWidget(this);
    pCentralWidget->setObjectName(QStringLiteral("pCentralWidget"));
    QGridLayout *pMainLayout = new QGridLayout(pCentralWidget);
    pMainLayout->setObjectName(QStringLiteral("pMainLayout"));
    m_pLabelTitle = new QLabel(pCentralWidget);
    m_pLabelTitle->setObjectName(QStringLiteral("m_pLabelTitle"));
    QSizePolicy sizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    sizePolicy.setHorizontalStretch(0);
    sizePolicy.setVerticalStretch(0);
    sizePolicy.setHeightForWidth(m_pLabelTitle->sizePolicy().hasHeightForWidth());
    m_pLabelTitle->setSizePolicy(sizePolicy);
    QPalette palette;
    QBrush brush(QColor(255, 255, 255, 255));
    brush.setStyle(Qt::SolidPattern);
    palette.setBrush(QPalette::Active, QPalette::Window, brush);
    palette.setBrush(QPalette::Inactive, QPalette::Window, brush);
    palette.setBrush(QPalette::Disabled, QPalette::Window, brush);
    m_pLabelTitle->setPalette(palette);
    QFont font;
    font.setFamily(QStringLiteral("Sans Serif"));
    font.setPointSize(11);
    font.setBold(true);
    font.setWeight(75);
    m_pLabelTitle->setFont(font);
    m_pLabelTitle->setAutoFillBackground(true);
    m_pLabelTitle->setFrameShape(QFrame::Box);
    m_pLabelTitle->setFrameShadow(QFrame::Sunken);
    m_pLabelTitle->setMargin(7);
    pMainLayout->addWidget(m_pLabelTitle, 0, 1, 1, 1);

    m_pWidgetStackHandler = new QWidget(pCentralWidget);
    m_pWidgetStackHandler->setObjectName(QStringLiteral("m_pWidgetStackHandler"));
    QSizePolicy sizePolicy1(QSizePolicy::Preferred, QSizePolicy::Expanding);
    sizePolicy1.setHorizontalStretch(0);
    sizePolicy1.setVerticalStretch(0);
    sizePolicy1.setHeightForWidth(m_pWidgetStackHandler->sizePolicy().hasHeightForWidth());
    m_pWidgetStackHandler->setSizePolicy(sizePolicy1);
    pMainLayout->addWidget(m_pWidgetStackHandler, 1, 1, 1, 1);

    m_pButtonBox = new QIDialogButtonBox(pCentralWidget);
    m_pButtonBox->setObjectName(QStringLiteral("m_pButtonBox"));
    m_pButtonBox->setStandardButtons(QDialogButtonBox::Cancel|QDialogButtonBox::NoButton|QDialogButtonBox::Ok);
    pMainLayout->addWidget(m_pButtonBox, 2, 0, 1, 2);

    setCentralWidget(pCentralWidget);

    QObject::connect(m_pButtonBox, &QIDialogButtonBox::rejected, this, &UISettingsDialog::reject);
    QObject::connect(m_pButtonBox, &QIDialogButtonBox::accepted, this, &UISettingsDialog::accept);
}

void UISettingsDialog::assignValidator(UISettingsPage *pPage)
{
    /* Assign validator: */
    UIPageValidator *pValidator = new UIPageValidator(this, pPage);
    connect(pValidator, &UIPageValidator::sigValidityChanged, this, &UISettingsDialog::sltHandleValidityChange);
    pPage->setValidator(pValidator);
    m_pWarningPane->registerValidator(pValidator);

    /// @todo Why here?
    /* Configure navigation (tab-order): */
    pPage->setOrderAfter(m_pSelector->widget());
}
