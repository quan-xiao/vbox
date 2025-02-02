/* $Id: UIGraphicsToolBar.cpp 82968 2020-02-04 10:35:17Z vboxsync $ */
/** @file
 * VBox Qt GUI - UIGraphicsToolBar class definition.
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

/* GUI includes: */
#include "UIGraphicsToolBar.h"
#include "UIGraphicsButton.h"


UIGraphicsToolBar::UIGraphicsToolBar(QIGraphicsWidget *pParent, int iRows, int iColumns)
    : QIGraphicsWidget(pParent)
    , m_iMargin(3)
    , m_iRows(iRows)
    , m_iColumns(iColumns)
{
}

int UIGraphicsToolBar::toolBarMargin() const
{
    return m_iMargin;
}

void UIGraphicsToolBar::setToolBarMargin(int iMargin)
{
    m_iMargin = iMargin;
}

void UIGraphicsToolBar::insertItem(UIGraphicsButton *pButton, int iRow, int iColumn)
{
    UIGraphicsToolBarIndex key = qMakePair(iRow, iColumn);
    m_buttons.insert(key, pButton);
}

void UIGraphicsToolBar::updateLayout()
{
    /* For all the rows: */
    for (int iRow = 0; iRow < m_iRows; ++iRow)
    {
        /* For all the columns: */
        for (int iColumn = 0; iColumn < m_iColumns; ++iColumn)
        {
            /* Generate key: */
            UIGraphicsToolBarIndex key = qMakePair(iRow, iColumn);
            /* Check if key present: */
            if (m_buttons.contains(key))
            {
                /* Get corresponding button: */
                UIGraphicsButton *pButton = m_buttons.value(key);
                QSize minimumSize = pButton->minimumSizeHint().toSize();
                pButton->setPos(toolBarMargin() + iColumn * minimumSize.width(),
                                toolBarMargin() + iRow * minimumSize.height());
            }
        }
    }
}

QSizeF UIGraphicsToolBar::sizeHint(Qt::SizeHint which, const QSizeF &constraint /* = QSizeF() */) const
{
    /* If Qt::MinimumSize hint requested: */
    if (which == Qt::MinimumSize)
    {
        /* Prepare variables: */
        int iProposedWidth = 2 * toolBarMargin();
        int iProposedHeight = 2 * toolBarMargin();
        /* Search for any button: */
        UIGraphicsButton *pButton = 0;
        for (int iRow = 0; !pButton && iRow < m_iRows; ++iRow)
        {
            /* For all the columns: */
            for (int iColumn = 0; !pButton && iColumn < m_iColumns; ++iColumn)
            {
                /* Generate key: */
                UIGraphicsToolBarIndex key = qMakePair(iRow, iColumn);
                /* Check if key present: */
                if (m_buttons.contains(key))
                {
                    /* Get corresponding button: */
                    pButton = m_buttons.value(key);
                }
            }
        }
        /* If any button found: */
        if (pButton)
        {
            /* Get button minimum-size: */
            QSize minimumSize = pButton->minimumSizeHint().toSize();
            iProposedWidth += m_iColumns * minimumSize.width();
            iProposedHeight += m_iRows * minimumSize.height();
        }
        /* Return result: */
        return QSizeF(iProposedWidth, iProposedHeight);
    }
    /* Else call to base-class: */
    return QIGraphicsWidget::sizeHint(which, constraint);
}

