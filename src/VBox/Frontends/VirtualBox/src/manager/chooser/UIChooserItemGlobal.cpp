/* $Id: UIChooserItemGlobal.cpp 84625 2020-06-01 16:44:39Z vboxsync $ */
/** @file
 * VBox Qt GUI - UIChooserItemGlobal class implementation.
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
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsView>
#include <QPainter>
#include <QStyleOptionGraphicsItem>

/* GUI includes: */
#include "UIChooserItemGlobal.h"
#include "UIChooserModel.h"
#include "UIChooserNodeGlobal.h"
#include "UIIconPool.h"
#include "UIVirtualBoxManager.h"

/* Other VBox includes: */
#include "iprt/cpp/utils.h"


UIChooserItemGlobal::UIChooserItemGlobal(UIChooserItem *pParent, UIChooserNodeGlobal *pNode)
    : UIChooserItem(pParent, pNode, 0, 100)
    , m_iDefaultLightnessMin(0)
    , m_iDefaultLightnessMax(0)
    , m_iHoverLightnessMin(0)
    , m_iHoverLightnessMax(0)
    , m_iHighlightLightnessMin(0)
    , m_iHighlightLightnessMax(0)
    , m_iMinimumNameWidth(0)
    , m_iMaximumNameWidth(0)
    , m_iHeightHint(0)
{
    prepare();
}

UIChooserItemGlobal::~UIChooserItemGlobal()
{
    cleanup();
}

UIChooserNodeGlobal *UIChooserItemGlobal::nodeToGlobalType() const
{
    return node() ? node()->toGlobalNode() : 0;
}

bool UIChooserItemGlobal::isToolButtonArea(const QPoint &position, int iMarginMultiplier /* = 1 */) const
{
    const int iFullWidth = geometry().width();
    const int iFullHeight = geometry().height();
    const int iMarginHR = data(GlobalItemData_MarginHR).toInt();
    const int iButtonMargin = data(GlobalItemData_ButtonMargin).toInt();
    const int iToolPixmapX = iFullWidth - iMarginHR - 1
                           - m_toolPixmap.width() / m_toolPixmap.devicePixelRatio();
    const int iToolPixmapY = (iFullHeight - m_toolPixmap.height() / m_toolPixmap.devicePixelRatio()) / 2;
    QRect rect = QRect(iToolPixmapX,
                       iToolPixmapY,
                       m_toolPixmap.width() / m_toolPixmap.devicePixelRatio(),
                       m_toolPixmap.height() / m_toolPixmap.devicePixelRatio());
    rect.adjust(-iMarginMultiplier * iButtonMargin, -iMarginMultiplier * iButtonMargin,
                 iMarginMultiplier * iButtonMargin,  iMarginMultiplier * iButtonMargin);
    return rect.contains(position);
}

bool UIChooserItemGlobal::isPinButtonArea(const QPoint &position, int iMarginMultiplier /* = 1 */) const
{
    const int iFullWidth = geometry().width();
    const int iFullHeight = geometry().height();
    const int iMarginHR = data(GlobalItemData_MarginHR).toInt();
    const int iSpacing = data(GlobalItemData_Spacing).toInt();
    const int iButtonMargin = data(GlobalItemData_ButtonMargin).toInt();
    const int iPinPixmapX = iFullWidth - iMarginHR - 1
                          - m_toolPixmap.width() / m_toolPixmap.devicePixelRatio()
                          - iSpacing
                          - m_pinPixmap.width() / m_pinPixmap.devicePixelRatio();
    const int iPinPixmapY = (iFullHeight - m_pinPixmap.height() / m_pinPixmap.devicePixelRatio()) / 2;
    QRect rect = QRect(iPinPixmapX,
                       iPinPixmapY,
                       m_pinPixmap.width() / m_pinPixmap.devicePixelRatio(),
                       m_pinPixmap.height() / m_pinPixmap.devicePixelRatio());
    rect.adjust(-iMarginMultiplier * iButtonMargin, -iMarginMultiplier * iButtonMargin,
                 iMarginMultiplier * iButtonMargin,  iMarginMultiplier * iButtonMargin);
    return rect.contains(position);
}

int UIChooserItemGlobal::heightHint() const
{
    return m_iHeightHint;
}

void UIChooserItemGlobal::setHeightHint(int iHint)
{
    /* Remember a new hint: */
    m_iHeightHint = iHint;

    /* Update geometry and the model layout: */
    updateGeometry();
    model()->updateLayout();
}

void UIChooserItemGlobal::retranslateUi()
{
}

void UIChooserItemGlobal::showEvent(QShowEvent *pEvent)
{
    /* Call to base-class: */
    UIChooserItem::showEvent(pEvent);

    /* Update pixmaps: */
    updatePixmaps();
}

void UIChooserItemGlobal::resizeEvent(QGraphicsSceneResizeEvent *pEvent)
{
    /* Call to base-class: */
    UIChooserItem::resizeEvent(pEvent);

    /* What is the new geometry? */
    const QRectF newGeometry = geometry();

    /* Should we update visible name? */
    if (previousGeometry().width() != newGeometry.width())
        updateMaximumNameWidth();

    /* Remember the new geometry: */
    setPreviousGeometry(newGeometry);
}

void UIChooserItemGlobal::mousePressEvent(QGraphicsSceneMouseEvent *pEvent)
{
    /* Call to base-class: */
    UIChooserItem::mousePressEvent(pEvent);
    /* No drag at all: */
    pEvent->ignore();
}

void UIChooserItemGlobal::paint(QPainter *pPainter, const QStyleOptionGraphicsItem *pOptions, QWidget * /* pWidget = 0 */)
{
    /* Acquire rectangle: */
    const QRect rectangle = pOptions->rect;

    /* Paint background: */
    paintBackground(pPainter, rectangle);
    /* Paint frame: */
    paintFrame(pPainter, rectangle);
    /* Paint global info: */
    paintGlobalInfo(pPainter, rectangle);
}

void UIChooserItemGlobal::setFavorite(bool fFavorite)
{
    /* Call to base-class: */
    UIChooserItem::setFavorite(fFavorite);

    /* Update pin-pixmap: */
    updatePinPixmap();
}

void UIChooserItemGlobal::startEditing()
{
    AssertMsgFailed(("Global graphics item do NOT support editing yet!"));
}

void UIChooserItemGlobal::updateItem()
{
    /* Update this global-item: */
    updatePixmaps();
    updateMinimumNameWidth();
    updateVisibleName();
    updateToolTip();
    update();

    /* Update parent group-item: */
    parentItem()->updateToolTip();
    parentItem()->update();
}

void UIChooserItemGlobal::updateToolTip()
{
    // Nothing for now..
}

QList<UIChooserItem*> UIChooserItemGlobal::items(UIChooserNodeType) const
{
    AssertMsgFailedReturn(("Global graphics item do NOT support children!"), QList<UIChooserItem*>());
}

void UIChooserItemGlobal::addItem(UIChooserItem *, bool, int)
{
    AssertMsgFailed(("Global graphics item do NOT support children!"));
}

void UIChooserItemGlobal::removeItem(UIChooserItem *)
{
    AssertMsgFailed(("Global graphics item do NOT support children!"));
}

UIChooserItem *UIChooserItemGlobal::searchForItem(const QString &, int iSearchFlags)
{
    /* Ignore if we are not searching for the global-item: */
    if (!(iSearchFlags & UIChooserItemSearchFlag_Global))
        return 0;

    /* Returning this: */
    return this;
}

UIChooserItem *UIChooserItemGlobal::firstMachineItem()
{
    return 0;
}

void UIChooserItemGlobal::updateLayout()
{
    // Just do nothing ..
}

int UIChooserItemGlobal::minimumWidthHint() const
{
    /* Prepare variables: */
    const int iMarginHL = data(GlobalItemData_MarginHL).toInt();
    const int iMarginHR = data(GlobalItemData_MarginHR).toInt();
    const int iSpacing = data(GlobalItemData_Spacing).toInt();

    /* Calculating proposed width: */
    int iProposedWidth = 0;

    /* Two margins: */
    iProposedWidth += iMarginHL + iMarginHR;
    /* And global-item content width: */
    iProposedWidth += (m_pixmapSize.width() +
                       iSpacing +
                       m_iMinimumNameWidth +
                       iSpacing +
                       m_toolPixmapSize.width() +
                       iSpacing +
                       m_pinPixmapSize.width());

    /* Return result: */
    return iProposedWidth;
}

int UIChooserItemGlobal::minimumHeightHint() const
{
    /* Prepare variables: */
    const int iMarginV = data(GlobalItemData_MarginV).toInt();

    /* Calculating proposed height: */
    int iProposedHeight = 0;

    /* Global-item content height: */
    int iContentHeight = qMax(m_pixmapSize.height(), m_visibleNameSize.height());
    iContentHeight = qMax(iContentHeight, m_toolPixmapSize.height());
    iContentHeight = qMax(iContentHeight, m_pinPixmapSize.height());

    /* If we have height hint: */
    if (m_iHeightHint)
    {
        /* Take the largest value between height hint and content height: */
        iProposedHeight += qMax(m_iHeightHint, iContentHeight);
    }
    /* Otherwise: */
    else
    {
        /* Two margins: */
        iProposedHeight += 2 * iMarginV;
        /* And content height: */
        iProposedHeight += iContentHeight;
    }

    /* Return result: */
    return iProposedHeight;
}

QSizeF UIChooserItemGlobal::sizeHint(Qt::SizeHint which, const QSizeF &constraint /* = QSizeF() */) const
{
    /* If Qt::MinimumSize requested: */
    if (which == Qt::MinimumSize)
        return QSizeF(minimumWidthHint(), minimumHeightHint());
    /* Else call to base-class: */
    return UIChooserItem::sizeHint(which, constraint);
}

QPixmap UIChooserItemGlobal::toPixmap()
{
    AssertFailedReturn(QPixmap());
}

bool UIChooserItemGlobal::isDropAllowed(QGraphicsSceneDragDropEvent *, UIChooserItemDragToken) const
{
    /* No drops at all: */
    return false;
}

void UIChooserItemGlobal::processDrop(QGraphicsSceneDragDropEvent *, UIChooserItem *, UIChooserItemDragToken)
{
    /* Nothing to process: */
}

void UIChooserItemGlobal::resetDragToken()
{
    /* Nothing to process: */
}

QMimeData *UIChooserItemGlobal::createMimeData()
{
    /* Nothing to return: */
    return 0;
}

void UIChooserItemGlobal::sltHandleWindowRemapped()
{
    updatePixmaps();
}

void UIChooserItemGlobal::prepare()
{
    /* Colors: */
#ifdef VBOX_WS_MAC
    m_iHighlightLightnessMin = 105;
    m_iHighlightLightnessMax = 115;
    m_iHoverLightnessMin = 115;
    m_iHoverLightnessMax = 125;
    m_iDefaultLightnessMin = 125;
    m_iDefaultLightnessMax = 130;
#else /* VBOX_WS_MAC */
    m_iHighlightLightnessMin = 130;
    m_iHighlightLightnessMax = 160;
    m_iHoverLightnessMin = 160;
    m_iHoverLightnessMax = 190;
    m_iDefaultLightnessMin = 160;
    m_iDefaultLightnessMax = 190;
#endif /* !VBOX_WS_MAC */

    /* Fonts: */
    m_nameFont = font();
    m_nameFont.setWeight(QFont::Bold);

    /* Sizes: */
    m_iMinimumNameWidth = 0;
    m_iMaximumNameWidth = 0;

    /* Add item to the parent: */
    AssertPtrReturnVoid(parentItem());
    parentItem()->addItem(this, isFavorite(), position());

    /* Configure connections: */
    connect(gpManager, &UIVirtualBoxManager::sigWindowRemapped,
            this, &UIChooserItemGlobal::sltHandleWindowRemapped);

    /* Init: */
    updatePixmaps();

    /* Apply language settings: */
    retranslateUi();
}

void UIChooserItemGlobal::cleanup()
{
    /* If that item is current: */
    if (model()->currentItem() == this)
    {
        /* Unset current-item: */
        model()->setCurrentItem(0);
    }
    /* If that item is in selection list: */
    if (model()->selectedItems().contains(this))
    {
        /* Remove item from the selection list: */
        model()->removeFromSelectedItems(this);
    }
    /* If that item is in navigation list: */
    if (model()->navigationItems().contains(this))
    {
        /* Remove item from the navigation list: */
        model()->removeFromNavigationItems(this);
    }

    /* Remove item from the parent: */
    AssertPtrReturnVoid(parentItem());
    parentItem()->removeItem(this);
}

QVariant UIChooserItemGlobal::data(int iKey) const
{
    /* Provide other members with required data: */
    switch (iKey)
    {
        /* Layout hints: */
        case GlobalItemData_MarginHL:     return QApplication::style()->pixelMetric(QStyle::PM_SmallIconSize);
        case GlobalItemData_MarginHR:     return QApplication::style()->pixelMetric(QStyle::PM_SmallIconSize) / 4 * 5;
        case GlobalItemData_MarginV:      return QApplication::style()->pixelMetric(QStyle::PM_SmallIconSize) / 4 * 3;
        case GlobalItemData_Spacing:      return QApplication::style()->pixelMetric(QStyle::PM_SmallIconSize) / 2;
        case GlobalItemData_ButtonMargin: return QApplication::style()->pixelMetric(QStyle::PM_SmallIconSize) / 4;

        /* Default: */
        default: break;
    }
    return QVariant();
}

void UIChooserItemGlobal::updatePixmaps()
{
    /* Update pixmap: */
    updatePixmap();
    /* Update tool-pixmap: */
    updateToolPixmap();
    /* Update pin-pixmap: */
    updatePinPixmap();
}

void UIChooserItemGlobal::updatePixmap()
{
    /* Acquire new metric, then compose pixmap-size: */
    const int iMetric = QApplication::style()->pixelMetric(QStyle::PM_LargeIconSize);
    const QSize pixmapSize = QSize(iMetric, iMetric);

    /* Create new icon, then acquire pixmap: */
    const QIcon icon = UIIconPool::iconSet(":/tools_global_32px.png");
    const QPixmap pixmap = icon.pixmap(gpManager->windowHandle(), pixmapSize);

    /* Update linked values: */
    if (m_pixmapSize != pixmapSize)
    {
        m_pixmapSize = pixmapSize;
        updateMaximumNameWidth();
        updateGeometry();
    }
    if (m_pixmap.toImage() != pixmap.toImage())
    {
        m_pixmap = pixmap;
        update();
    }
}

void UIChooserItemGlobal::updateToolPixmap()
{
    /* Determine icon metric: */
    const int iIconMetric = QApplication::style()->pixelMetric(QStyle::PM_LargeIconSize) * .75;
    /* Create new tool-pixmap and tool-pixmap size: */
    const QIcon toolIcon = UIIconPool::iconSet(":/tools_menu_24px.png");
    AssertReturnVoid(!toolIcon.isNull());
    const QSize toolPixmapSize = QSize(iIconMetric, iIconMetric);
    const QPixmap toolPixmap = toolIcon.pixmap(gpManager->windowHandle(), toolPixmapSize);
    /* Update linked values: */
    if (m_toolPixmapSize != toolPixmapSize)
    {
        m_toolPixmapSize = toolPixmapSize;
        updateGeometry();
    }
    if (m_toolPixmap.toImage() != toolPixmap.toImage())
    {
        m_toolPixmap = toolPixmap;
        update();
    }
}

void UIChooserItemGlobal::updatePinPixmap()
{
    /* Determine icon metric: */
    const int iIconMetric = QApplication::style()->pixelMetric(QStyle::PM_LargeIconSize) * .75;
    /* Create new tool-pixmap and tool-pixmap size: */
    const QIcon pinIcon = UIIconPool::iconSet(isFavorite() ? ":/favorite_pressed_24px.png" : ":/favorite_24px.png");
    AssertReturnVoid(!pinIcon.isNull());
    const QSize pinPixmapSize = QSize(iIconMetric, iIconMetric);
    const QPixmap pinPixmap = pinIcon.pixmap(gpManager->windowHandle(), pinPixmapSize);
    /* Update linked values: */
    if (m_pinPixmapSize != pinPixmapSize)
    {
        m_pinPixmapSize = pinPixmapSize;
        updateGeometry();
    }
    if (m_pinPixmap.toImage() != pinPixmap.toImage())
    {
        m_pinPixmap = pinPixmap;
        update();
    }
}

void UIChooserItemGlobal::updateMinimumNameWidth()
{
    /* Calculate new minimum name width: */
    QPaintDevice *pPaintDevice = model()->paintDevice();
    const QFontMetrics fm(m_nameFont, pPaintDevice);
    const int iMinimumNameWidth = fm.width(compressText(m_nameFont, pPaintDevice, name(), textWidth(m_nameFont, pPaintDevice, 15)));

    /* Is there something changed? */
    if (m_iMinimumNameWidth == iMinimumNameWidth)
        return;

    /* Update linked values: */
    m_iMinimumNameWidth = iMinimumNameWidth;
    updateGeometry();
}

void UIChooserItemGlobal::updateMaximumNameWidth()
{
    /* Prepare variables: */
    const int iMarginHL = data(GlobalItemData_MarginHL).toInt();
    const int iMarginHR = data(GlobalItemData_MarginHR).toInt();
    const int iSpacing = data(GlobalItemData_Spacing).toInt();

    /* Calculate new maximum name width: */
    int iMaximumNameWidth = (int)geometry().width();
    iMaximumNameWidth -= iMarginHL; /* left margin */
    iMaximumNameWidth -= m_pixmapSize.width(); /* pixmap width */
    iMaximumNameWidth -= iSpacing; /* spacing between pixmap and name */
    iMaximumNameWidth -= iMarginHR; /* right margin */

    /* Is there something changed? */
    if (m_iMaximumNameWidth == iMaximumNameWidth)
        return;

    /* Update linked values: */
    m_iMaximumNameWidth = iMaximumNameWidth;
    updateVisibleName();
}

void UIChooserItemGlobal::updateVisibleName()
{
    /* Prepare variables: */
    QPaintDevice *pPaintDevice = model()->paintDevice();

    /* Calculate new visible name and name-size: */
    const QString strVisibleName = compressText(m_nameFont, pPaintDevice, name(), m_iMaximumNameWidth);
    const QSize visibleNameSize = textSize(m_nameFont, pPaintDevice, strVisibleName);

    /* Update linked values: */
    if (m_visibleNameSize != visibleNameSize)
    {
        m_visibleNameSize = visibleNameSize;
        updateGeometry();
    }
    if (m_strVisibleName != strVisibleName)
    {
        m_strVisibleName = strVisibleName;
        update();
    }
}

void UIChooserItemGlobal::paintBackground(QPainter *pPainter, const QRect &rectangle) const
{
    /* Save painter: */
    pPainter->save();

    /* Prepare color: */
    const QPalette pal = palette();

    /* Selected-item background: */
    if (model()->selectedItems().contains(unconst(this)))
    {
        /* Prepare color: */
        const QColor backgroundColor = pal.color(QPalette::Active, QPalette::Highlight);
        /* Draw gradient: */
        QLinearGradient bgGrad(rectangle.topLeft(), rectangle.bottomLeft());
        bgGrad.setColorAt(0, backgroundColor.lighter(m_iHighlightLightnessMax));
        bgGrad.setColorAt(1, backgroundColor.lighter(m_iHighlightLightnessMin));
        pPainter->fillRect(rectangle, bgGrad);

        if (isHovered())
        {
            /* Prepare color: */
            QColor animationColor1 = QColor(Qt::white);
            QColor animationColor2 = QColor(Qt::white);
#ifdef VBOX_WS_MAC
            animationColor1.setAlpha(90);
#else
            animationColor1.setAlpha(30);
#endif
            animationColor2.setAlpha(0);
            /* Draw hovered-item animated gradient: */
            QRect animatedRect = rectangle;
            animatedRect.setWidth(animatedRect.height());
            const int iLength = 2 * animatedRect.width() + rectangle.width();
            const int iShift = - animatedRect.width() + iLength * animatedValue() / 100;
            animatedRect.moveLeft(iShift);
            QLinearGradient bgAnimatedGrad(animatedRect.topLeft(), animatedRect.bottomRight());
            bgAnimatedGrad.setColorAt(0,   animationColor2);
            bgAnimatedGrad.setColorAt(0.1, animationColor2);
            bgAnimatedGrad.setColorAt(0.5, animationColor1);
            bgAnimatedGrad.setColorAt(0.9, animationColor2);
            bgAnimatedGrad.setColorAt(1,   animationColor2);
            pPainter->fillRect(rectangle, bgAnimatedGrad);
        }
    }
    /* Hovered-item background: */
    else if (isHovered())
    {
        /* Prepare color: */
        const QColor backgroundColor = pal.color(QPalette::Active, QPalette::Highlight);
        /* Draw gradient: */
        QLinearGradient bgGrad(rectangle.topLeft(), rectangle.bottomLeft());
        bgGrad.setColorAt(0, backgroundColor.lighter(m_iHoverLightnessMax));
        bgGrad.setColorAt(1, backgroundColor.lighter(m_iHoverLightnessMin));
        pPainter->fillRect(rectangle, bgGrad);

        /* Prepare color: */
        QColor animationColor1 = QColor(Qt::white);
        QColor animationColor2 = QColor(Qt::white);
#ifdef VBOX_WS_MAC
        animationColor1.setAlpha(120);
#else
        animationColor1.setAlpha(50);
#endif
        animationColor2.setAlpha(0);
        /* Draw hovered-item animated gradient: */
        QRect animatedRect = rectangle;
        animatedRect.setWidth(animatedRect.height());
        const int iLength = 2 * animatedRect.width() + rectangle.width();
        const int iShift = - animatedRect.width() + iLength * animatedValue() / 100;
        animatedRect.moveLeft(iShift);
        QLinearGradient bgAnimatedGrad(animatedRect.topLeft(), animatedRect.bottomRight());
        bgAnimatedGrad.setColorAt(0,   animationColor2);
        bgAnimatedGrad.setColorAt(0.1, animationColor2);
        bgAnimatedGrad.setColorAt(0.5, animationColor1);
        bgAnimatedGrad.setColorAt(0.9, animationColor2);
        bgAnimatedGrad.setColorAt(1,   animationColor2);
        pPainter->fillRect(rectangle, bgAnimatedGrad);
    }
    /* Default background: */
    else
    {
#ifdef VBOX_WS_MAC
        /* Prepare color: */
        const QColor backgroundColor = pal.color(QPalette::Active, QPalette::Mid);
        /* Draw gradient: */
        QLinearGradient bgGrad(rectangle.topLeft(), rectangle.bottomLeft());
        bgGrad.setColorAt(0, backgroundColor.lighter(m_iDefaultLightnessMax));
        bgGrad.setColorAt(1, backgroundColor.lighter(m_iDefaultLightnessMin));
        pPainter->fillRect(rectangle, bgGrad);
#else
        /* Prepare color: */
        QColor backgroundColor = pal.color(QPalette::Active, QPalette::Mid).lighter(160);
        /* Draw gradient: */
        pPainter->fillRect(rectangle, backgroundColor);
#endif
    }

    /* Restore painter: */
    pPainter->restore();
}

void UIChooserItemGlobal::paintFrame(QPainter *pPainter, const QRect &rectangle) const
{
    /* Only selected and/or hovered item should have a frame: */
    if (!model()->selectedItems().contains(unconst(this)) && !isHovered())
        return;

    /* Save painter: */
    pPainter->save();

    /* Prepare color: */
    const QPalette pal = palette();
    QColor strokeColor;

    /* Selected-item frame: */
    if (model()->selectedItems().contains(unconst(this)))
        strokeColor = pal.color(QPalette::Active, QPalette::Highlight).lighter(m_iHighlightLightnessMin - 40);
    /* Hovered-item frame: */
    else if (isHovered())
        strokeColor = pal.color(QPalette::Active, QPalette::Highlight).lighter(m_iHoverLightnessMin - 50);
    /* Default frame: */
    else
        strokeColor = pal.color(QPalette::Active, QPalette::Mid).lighter(m_iDefaultLightnessMin);

    /* Create/assign pen: */
    QPen pen(strokeColor);
    pen.setWidth(0);
    pPainter->setPen(pen);

    /* Draw borders: */
    pPainter->drawLine(rectangle.topLeft(),    rectangle.topRight()    + QPoint(1, 0));
    pPainter->drawLine(rectangle.bottomLeft(), rectangle.bottomRight() + QPoint(1, 0));
    pPainter->drawLine(rectangle.topLeft(),    rectangle.bottomLeft());

    /* Restore painter: */
    pPainter->restore();
}

void UIChooserItemGlobal::paintGlobalInfo(QPainter *pPainter, const QRect &rectangle) const
{
    /* Prepare variables: */
    const int iFullWidth = rectangle.width();
    const int iFullHeight = rectangle.height();
    const int iMarginHL = data(GlobalItemData_MarginHL).toInt();
    const int iMarginHR = data(GlobalItemData_MarginHR).toInt();
    const int iSpacing = data(GlobalItemData_Spacing).toInt();
    const int iButtonMargin = data(GlobalItemData_ButtonMargin).toInt();

    /* Selected-item foreground: */
    if (model()->selectedItems().contains(unconst(this)))
    {
        const QPalette pal = palette();
        pPainter->setPen(pal.color(QPalette::HighlightedText));
    }
    /* Hovered item foreground: */
    else if (isHovered())
    {
        /* Prepare color: */
        const QPalette pal = palette();
        const QColor highlight = pal.color(QPalette::Active, QPalette::Highlight);
        const QColor hhl = highlight.lighter(m_iHoverLightnessMax);
        if (hhl.value() - hhl.saturation() > 0)
            pPainter->setPen(pal.color(QPalette::Active, QPalette::Text));
        else
            pPainter->setPen(pal.color(QPalette::Active, QPalette::HighlightedText));
    }

    /* Calculate indents: */
    int iLeftColumnIndent = iMarginHL;

    /* Paint left column: */
    {
        /* Prepare variables: */
        const int iGlobalPixmapX = iLeftColumnIndent;
        const int iGlobalPixmapY = (iFullHeight - m_pixmap.height() / m_pixmap.devicePixelRatio()) / 2;

        /* Paint pixmap: */
        paintPixmap(/* Painter: */
                    pPainter,
                    /* Point to paint in: */
                    QPoint(iGlobalPixmapX, iGlobalPixmapY),
                    /* Pixmap to paint: */
                    m_pixmap);
    }

    /* Calculate indents: */
    const int iMiddleColumnIndent = iLeftColumnIndent +
                                    m_pixmapSize.width() +
                                    iSpacing;

    /* Paint middle column: */
    {
        /* Prepare variables: */
        const int iNameX = iMiddleColumnIndent;
        const int iNameY = (iFullHeight - m_visibleNameSize.height()) / 2;

        /* Paint name: */
        paintText(/* Painter: */
                  pPainter,
                  /* Point to paint in: */
                  QPoint(iNameX, iNameY),
                  /* Font to paint text: */
                  m_nameFont,
                  /* Paint device: */
                  model()->paintDevice(),
                  /* Text to paint: */
                  m_strVisibleName);
    }

    /* Calculate indents: */
    QGraphicsView *pView = model()->scene()->views().first();
    const QPointF sceneCursorPosition = pView->mapToScene(pView->mapFromGlobal(QCursor::pos()));
    const QPoint itemCursorPosition = mapFromScene(sceneCursorPosition).toPoint();
    int iRightColumnIndent = iFullWidth - iMarginHR - 1 - m_toolPixmap.width() / m_toolPixmap.devicePixelRatio();

    /* Paint right column: */
    if (   model()->firstSelectedItem() == this
        || isHovered())
    {
        /* Prepare variables: */
        const int iToolPixmapX = iRightColumnIndent;
        const int iToolPixmapY = (iFullHeight - m_toolPixmap.height() / m_toolPixmap.devicePixelRatio()) / 2;
        QRect toolButtonRectangle = QRect(iToolPixmapX,
                                          iToolPixmapY,
                                          m_toolPixmap.width() / m_toolPixmap.devicePixelRatio(),
                                          m_toolPixmap.height() / m_toolPixmap.devicePixelRatio());
        toolButtonRectangle.adjust(- iButtonMargin, -iButtonMargin, iButtonMargin, iButtonMargin);

        /* Paint tool button: */
        if (   isHovered()
            && isToolButtonArea(itemCursorPosition, 4))
            paintFlatButton(/* Painter: */
                            pPainter,
                            /* Button rectangle: */
                            toolButtonRectangle,
                            /* Cursor position: */
                            itemCursorPosition);

        /* Paint pixmap: */
        paintPixmap(/* Painter: */
                    pPainter,
                    /* Point to paint in: */
                    QPoint(iToolPixmapX, iToolPixmapY),
                    /* Pixmap to paint: */
                    m_toolPixmap);
    }

    /* Calculate indents: */
    iRightColumnIndent = iRightColumnIndent - m_toolPixmap.width() / m_toolPixmap.devicePixelRatio() - iSpacing;

    /* Paint right column: */
    if (   model()->firstSelectedItem() == this
        || isHovered())
    {
        /* Prepare variables: */
        const int iPinPixmapX = iRightColumnIndent;
        const int iPinPixmapY = (iFullHeight - m_pinPixmap.height() / m_pinPixmap.devicePixelRatio()) / 2;
        QRect pinButtonRectangle = QRect(iPinPixmapX,
                                         iPinPixmapY,
                                         m_pinPixmap.width() / m_pinPixmap.devicePixelRatio(),
                                         m_pinPixmap.height() / m_pinPixmap.devicePixelRatio());
        pinButtonRectangle.adjust(- iButtonMargin, -iButtonMargin, iButtonMargin, iButtonMargin);

        /* Paint pin button: */
        if (   isHovered()
            && isPinButtonArea(itemCursorPosition, 4))
            paintFlatButton(/* Painter: */
                            pPainter,
                            /* Button rectangle: */
                            pinButtonRectangle,
                            /* Cursor position: */
                            itemCursorPosition);

        /* Paint pixmap: */
        paintPixmap(/* Painter: */
                    pPainter,
                    /* Point to paint in: */
                    QPoint(iPinPixmapX, iPinPixmapY),
                    /* Pixmap to paint: */
                    m_pinPixmap);
    }
}
