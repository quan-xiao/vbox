/* $Id: UIVMLogPage.cpp 82968 2020-02-04 10:35:17Z vboxsync $ */
/** @file
 * VBox Qt GUI - UIVMLogViewer class implementation.
 */

/*
 * Copyright (C) 2010-2020 Oracle Corporation
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
#include <QDateTime>
#include <QDir>
#include <QVBoxLayout>
#if defined(RT_OS_SOLARIS)
# include <QFontDatabase>
#endif
#include <QPainter>
#include <QScrollBar>
#include <QTextBlock>

/* GUI includes: */
#include "UIVMLogPage.h"
#include "UIVMLogViewerTextEdit.h"


UIVMLogPage::UIVMLogPage(QWidget *pParent /* = 0 */, int tabIndex /*= -1 */)
    : QIWithRetranslateUI<QWidget>(pParent)
    , m_pMainLayout(0)
    , m_pTextEdit(0)
    , m_tabIndex(tabIndex)
    , m_iSelectedBookmarkIndex(-1)
    , m_bFiltered(false)
    , m_iFilteredLineCount(-1)
    , m_iUnfilteredLineCount(-1)
{
    prepare();
}

UIVMLogPage::~UIVMLogPage()
{
    cleanup();
}

int UIVMLogPage::defaultLogPageWidth() const
{
    if (!m_pTextEdit)
        return 0;

    /* Compute a width for 132 characters plus scrollbar and frame width: */
    int iDefaultWidth = m_pTextEdit->fontMetrics().width(QChar('x')) * 132 +
                        m_pTextEdit->verticalScrollBar()->width() +
                        m_pTextEdit->frameWidth() * 2;

    return iDefaultWidth;
}


void UIVMLogPage::prepare()
{
    prepareWidgets();
    retranslateUi();
}

void UIVMLogPage::prepareWidgets()
{
    m_pMainLayout = new QHBoxLayout();
    setLayout(m_pMainLayout);
    m_pMainLayout->setSpacing(0);
    m_pMainLayout->setContentsMargins(0, 0, 0, 0);

    m_pTextEdit = new UIVMLogViewerTextEdit(this);
    m_pMainLayout->addWidget(m_pTextEdit);

    connect(m_pTextEdit, &UIVMLogViewerTextEdit::sigAddBookmark,
            this, &UIVMLogPage::sltAddBookmark);
    connect(m_pTextEdit, &UIVMLogViewerTextEdit::sigDeleteBookmark,
            this, &UIVMLogPage::sltDeleteBookmark);
}

QPlainTextEdit *UIVMLogPage::textEdit()
{
    return m_pTextEdit;
}

QTextDocument* UIVMLogPage::document()
{
    if (!m_pTextEdit)
        return 0;
    return m_pTextEdit->document();
}

void UIVMLogPage::setTabIndex(int index)
{
    m_tabIndex = index;
}

int UIVMLogPage::tabIndex()  const
{
    return m_tabIndex;
}

void UIVMLogPage::retranslateUi()
{
}

void UIVMLogPage::cleanup()
{
}

void UIVMLogPage::setLogString(const QString &strLog)
{
    m_strLog = strLog;
}

const QString& UIVMLogPage::logString() const
{
    return m_strLog;
}

void UIVMLogPage::setLogFileName(const QString &strLogFileName)
{
    m_strLogFileName = strLogFileName;
}

const QString& UIVMLogPage::logFileName() const
{
    return m_strLogFileName;
}

void UIVMLogPage::setTextEditText(const QString &strText)
{
    if (!m_pTextEdit)
        return;

    m_pTextEdit->setPlainText(strText);
    /* Move the cursor position to end: */
    QTextCursor cursor = m_pTextEdit->textCursor();
    cursor.movePosition(QTextCursor::End, QTextCursor::MoveAnchor);
    m_pTextEdit->setTextCursor(cursor);
    update();
}

void UIVMLogPage::setTextEditTextAsHtml(const QString &strText)
{
    if (!m_pTextEdit)
        return;
    m_pTextEdit->appendHtml(strText);
    update();
}

void UIVMLogPage::markForError()
{
    if (!m_pTextEdit)
        return;
    m_pTextEdit->setWrapLines(true);
}

void UIVMLogPage::setScrollBarMarkingsVector(const QVector<float> &vector)
{
    if (!m_pTextEdit)
        return;
    m_pTextEdit->setScrollBarMarkingsVector(vector);
    update();
}

void UIVMLogPage::clearScrollBarMarkingsVector()
{
    if (!m_pTextEdit)
        return;
    m_pTextEdit->clearScrollBarMarkingsVector();
    update();
}

void UIVMLogPage::documentUndo()
{
    if (!m_pTextEdit)
        return;
    if (m_pTextEdit->document())
        m_pTextEdit->document()->undo();
}


void UIVMLogPage::deleteBookmark(int index)
{
    if (m_bookmarkVector.size() <= index)
         return;
    m_bookmarkVector.remove(index, 1);
    updateTextEditBookmarkLineSet();
}

void UIVMLogPage::deleteBookmark(LogBookmark bookmark)
{
    int index = -1;
    for (int i = 0; i < m_bookmarkVector.size(); ++i)
    {
        if (m_bookmarkVector.at(i).first == bookmark.first)
        {
            index = i;
            break;
        }
    }
    if (index != -1)
        deleteBookmark(index);
}


void UIVMLogPage::deleteAllBookmarks()
{
    m_bookmarkVector.clear();
    updateTextEditBookmarkLineSet();
}

void UIVMLogPage::scrollToBookmark(int bookmarkIndex)
{
    if (!m_pTextEdit)
        return;
    if (bookmarkIndex >= m_bookmarkVector.size())
        return;

    int lineNumber = m_bookmarkVector.at(bookmarkIndex).first;
    m_pTextEdit->scrollToLine(lineNumber);
}

const QVector<LogBookmark>& UIVMLogPage::bookmarkVector() const
{
    return m_bookmarkVector;
}

void UIVMLogPage::setBookmarkVector(const QVector<LogBookmark>& bookmarks)
{
    m_bookmarkVector = bookmarks;
    updateTextEditBookmarkLineSet();
}

void UIVMLogPage::sltAddBookmark(LogBookmark bookmark)
{
    m_bookmarkVector.push_back(bookmark);
    updateTextEditBookmarkLineSet();
    emit sigBookmarksUpdated();
}

void UIVMLogPage::sltDeleteBookmark(LogBookmark bookmark)
{
    deleteBookmark(bookmark);
    updateTextEditBookmarkLineSet();
    emit sigBookmarksUpdated();
}

void UIVMLogPage::updateTextEditBookmarkLineSet()
{
    if (!m_pTextEdit)
        return;
    QSet<int> bookmarkLinesSet;
    for (int i = 0; i < m_bookmarkVector.size(); ++i)
    {
        bookmarkLinesSet.insert(m_bookmarkVector.at(i).first);
    }
    m_pTextEdit->setBookmarkLineSet(bookmarkLinesSet);
}

bool UIVMLogPage::isFiltered() const
{
    return m_bFiltered;
}

void UIVMLogPage::setFiltered(bool filtered)
{
    if (m_bFiltered == filtered)
        return;
    m_bFiltered = filtered;
    if (m_pTextEdit)
    {
        m_pTextEdit->setShownTextIsFiltered(m_bFiltered);
        m_pTextEdit->update();
    }
    emit sigLogPageFilteredChanged(m_bFiltered);
}

void UIVMLogPage::setShowLineNumbers(bool bShowLineNumbers)
{
    if (!m_pTextEdit)
        return;
    m_pTextEdit->setShowLineNumbers(bShowLineNumbers);
}

void UIVMLogPage::setWrapLines(bool bWrapLines)
{
    if (!m_pTextEdit)
        return;
    m_pTextEdit->setWrapLines(bWrapLines);
}

void UIVMLogPage::setFilterParameters(const QSet<QString> &filterTermSet, int filterOperationType,
                                      int iFilteredLineCount, int iUnfilteredLineCount)
{
    m_filterTermSet = filterTermSet;
    m_filterOperationType = filterOperationType;
    m_iFilteredLineCount = iFilteredLineCount;
    m_iUnfilteredLineCount = iUnfilteredLineCount;
}

int  UIVMLogPage::filteredLineCount() const
{
    return m_iFilteredLineCount;
}

int  UIVMLogPage::unfilteredLineCount() const
{
    return m_iUnfilteredLineCount;
}

bool UIVMLogPage::shouldFilterBeApplied(const QSet<QString> &filterTermSet, int filterOperationType) const
{
    /* If filter terms set is different reapply the filter. */
    if (filterTermSet != m_filterTermSet)
        return true;

    /* If filter operation type set is different reapply the filter. */
    if (filterOperationType != m_filterOperationType)
        return true;
    return false;
}

QFont UIVMLogPage::currentFont() const
{
    if (!m_pTextEdit)
        return QFont();
    return m_pTextEdit->font();
}

void UIVMLogPage::setCurrentFont(QFont font)
{
    if (m_pTextEdit)
        m_pTextEdit->setCurrentFont(font);
}
