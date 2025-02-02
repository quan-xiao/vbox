/* $Id: UIVisoHostBrowser.cpp 82968 2020-02-04 10:35:17Z vboxsync $ */
/** @file
 * VBox Qt GUI - UIVisoHostBrowser class implementation.
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
#include <QFileSystemModel>
#include <QGridLayout>
#include <QHeaderView>
#include <QMimeData>
#include <QTableView>
#include <QTextEdit>
#include <QTreeView>

/* GUI includes: */
#include "UIVisoHostBrowser.h"

/*********************************************************************************************************************************
*   UIVisoHostBrowserModel definition.                                                                                   *
*********************************************************************************************************************************/

class UIVisoHostBrowserModel : public QFileSystemModel
{
    Q_OBJECT;

public:
    virtual QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const /* override */;
    UIVisoHostBrowserModel(QObject *pParent);

    virtual QStringList mimeTypes() const /* override */;
    /** Prepares the mime data  as a list of text consisting of dragged objects full file path. */
    QMimeData *mimeData(const QModelIndexList &indexes) const /* override */;

protected:

private:

};

/*********************************************************************************************************************************
*   UIVisoHostBrowserModel implementation.                                                                                   *
*********************************************************************************************************************************/

UIVisoHostBrowserModel::UIVisoHostBrowserModel(QObject *pParent /* = 0 */)
    :QFileSystemModel(pParent)
{
}

QVariant UIVisoHostBrowserModel::data(const QModelIndex &index, int enmRole /* = Qt::DisplayRole */) const
{
    if (enmRole == Qt::DecorationRole && index.column() == 0)
    {
        QFileInfo info = fileInfo(index);

        if(info.isSymLink() && info.isDir())
            return QIcon(":/file_manager_folder_symlink_16px.png");
        else if(info.isSymLink() && info.isFile())
            return QIcon(":/file_manager_file_symlink_16px.png");
        else if(info.isFile())
            return QIcon(":/file_manager_file_16px.png");
        else if(info.isDir())
            return QIcon(":/file_manager_folder_16px.png");
    }
    return QFileSystemModel::data(index, enmRole);
}

QStringList UIVisoHostBrowserModel::mimeTypes() const
{
    QStringList types;
    types << "application/vnd.text.list";
    return types;
}

QMimeData *UIVisoHostBrowserModel::mimeData(const QModelIndexList &indexes) const
{
    QMimeData *mimeData = new QMimeData();
    QByteArray encodedData;

    QDataStream stream(&encodedData, QIODevice::WriteOnly);

    foreach (const QModelIndex &index, indexes) {
        if (index.isValid() && index.column() == 0)
        {
            QString strPath = fileInfo(index).filePath();
            if (!strPath.contains(".."))
                stream << fileInfo(index).filePath();
        }
    }

    mimeData->setData("application/vnd.text.list", encodedData);
    return mimeData;
}

/*********************************************************************************************************************************
*   UIVisoHostBrowser implementation.                                                                                   *
*********************************************************************************************************************************/

UIVisoHostBrowser::UIVisoHostBrowser(QWidget *pParent /* = 0 */)
    : UIVisoBrowserBase(pParent)
    , m_pTreeModel(0)
    , m_pTableModel(0)
    , m_pTableView(0)
{
    prepareObjects();
    prepareConnections();
}

UIVisoHostBrowser::~UIVisoHostBrowser()
{
}

void UIVisoHostBrowser::retranslateUi()
{
}

void UIVisoHostBrowser::prepareObjects()
{
    UIVisoBrowserBase::prepareObjects();

    m_pTreeModel = new UIVisoHostBrowserModel(this);
    m_pTreeModel->setRootPath(QDir::rootPath());
    m_pTreeModel->setReadOnly(true);
    m_pTreeModel->setFilter(QDir::AllDirs | QDir::NoDotAndDotDot | QDir::Hidden);
    m_pTableModel = new UIVisoHostBrowserModel(this);
    m_pTableModel->setRootPath(QDir::rootPath());
    m_pTableModel->setReadOnly(true);
    m_pTableModel->setFilter(QDir::AllEntries | QDir::NoDot | QDir::Hidden | QDir::System);

    if (m_pTreeView)
    {
        m_pTreeView->setModel(m_pTreeModel);
        m_pTreeView->setRootIndex(m_pTreeModel->index(m_pTreeModel->rootPath()).parent());
        m_pTreeView->setCurrentIndex(m_pTreeModel->index(QDir::homePath()));
        /* Show only the 0th column that is "name': */
        m_pTreeView->hideColumn(1);
        m_pTreeView->hideColumn(2);
        m_pTreeView->hideColumn(3);
    }

    m_pTableView = new QTableView;
    if (m_pTableView)
    {
        m_pTableView->setContextMenuPolicy(Qt::CustomContextMenu);
        m_pMainLayout->addWidget(m_pTableView, 1, 0, 8, 4);
        m_pTableView->setSelectionMode(QAbstractItemView::ContiguousSelection);
        m_pTableView->setShowGrid(false);
        m_pTableView->setSelectionBehavior(QAbstractItemView::SelectRows);
        m_pTableView->setEditTriggers(QAbstractItemView::NoEditTriggers);
        m_pTableView->setAlternatingRowColors(true);
        QHeaderView *pVerticalHeader = m_pTableView->verticalHeader();
        if (pVerticalHeader)
        {
            m_pTableView->verticalHeader()->setVisible(false);
            /* Minimize the row height: */
            m_pTableView->verticalHeader()->setDefaultSectionSize(m_pTableView->verticalHeader()->minimumSectionSize());
        }
        QHeaderView *pHorizontalHeader = m_pTableView->horizontalHeader();
        if (pHorizontalHeader)
        {
            pHorizontalHeader->setHighlightSections(false);
            pHorizontalHeader->setSectionResizeMode(QHeaderView::Stretch);
        }

        m_pTableView->setModel(m_pTableModel);
        setTableRootIndex();
        /* Hide the "type" column: */
        m_pTableView->hideColumn(2);

        m_pTableView->setSelectionMode(QAbstractItemView::ExtendedSelection);
        m_pTableView->setDragDropMode(QAbstractItemView::DragOnly);
    }

    retranslateUi();
}

void UIVisoHostBrowser::prepareConnections()
{
    UIVisoBrowserBase::prepareConnections();
    if (m_pTableView)
    {
        connect(m_pTableView, &QTableView::doubleClicked,
                this, &UIVisoBrowserBase::sltHandleTableViewItemDoubleClick);
        connect(m_pTableView, &QTableView::customContextMenuRequested,
                this, &UIVisoHostBrowser::sltFileTableViewContextMenu);
    }

    if (m_pTableView->selectionModel())
        connect(m_pTableView->selectionModel(), &QItemSelectionModel::selectionChanged,
                this, &UIVisoHostBrowser::sltHandleTableSelectionChanged);
}

void UIVisoHostBrowser::sltHandleTableSelectionChanged(const QItemSelection &selected, const QItemSelection &deselected)
{
    Q_UNUSED(deselected);
    emit sigTableSelectionChanged(selected.isEmpty());
}

void UIVisoHostBrowser::tableViewItemDoubleClick(const QModelIndex &index)
{
    if (!index.isValid())
        return;
    QFileInfo fileInfo = m_pTableModel->fileInfo(index);
    /* QFileInfo::isDir() returns true if QFileInfo is a folder or a symlink to folder: */
    if (!fileInfo.isDir())
        return;
    setTableRootIndex(index);

    m_pTreeView->blockSignals(true);
    setTreeCurrentIndex(index);
    m_pTreeView->blockSignals(false);

    /* Check if we still have something selected after table root index change: */
    if (m_pTableView && m_pTableView->selectionModel())
        emit sigTableSelectionChanged(m_pTableView->selectionModel()->hasSelection());
}

void UIVisoHostBrowser::treeSelectionChanged(const QModelIndex &selectedTreeIndex)
{
    setTableRootIndex(selectedTreeIndex);
}

void UIVisoHostBrowser::showHideHiddenObjects(bool bShow)
{
    if (bShow)
    {
        m_pTreeModel->setFilter(QDir::AllDirs | QDir::NoDotAndDotDot | QDir::Hidden);
        m_pTableModel->setFilter(QDir::AllEntries | QDir::NoDot | QDir::Hidden | QDir::System);
    }
    else
    {
        m_pTreeModel->setFilter(QDir::AllDirs | QDir::NoDotAndDotDot);
        m_pTableModel->setFilter(QDir::AllEntries | QDir::NoDot);
    }
}

QString UIVisoHostBrowser::currentPath() const
{
    if (!m_pTreeView || !m_pTreeModel)
        return QString();
    QModelIndex currentTreeIndex = m_pTreeView->selectionModel()->currentIndex();
    return QDir::fromNativeSeparators(m_pTreeModel->filePath(currentTreeIndex));
}

void UIVisoHostBrowser::setCurrentPath(const QString &strPath)
{
    if (strPath.isEmpty() || !m_pTreeModel)
        return;
    QModelIndex index = m_pTreeModel->index(strPath);
    setTreeCurrentIndex(index);
}

void UIVisoHostBrowser::sltHandleAddAction()
{
    if (!m_pTableView || !m_pTableModel)
        return;
    QItemSelectionModel *pSelectionModel = m_pTableView->selectionModel();
    if (!pSelectionModel)
        return;
    QModelIndexList selectedIndices = pSelectionModel->selectedRows(0);
    QStringList pathList;
    for (int i = 0; i < selectedIndices.size(); ++i)
    {
        QString strPath = m_pTableModel->filePath(selectedIndices[i]);
        if (strPath.contains(".."))
            continue;
        pathList << strPath;
    }
    emit sigAddObjectsToViso(pathList);
}

void UIVisoHostBrowser::setTableRootIndex(QModelIndex index /* = QModelIndex */)
{
    if (!m_pTreeView || !m_pTreeView->selectionModel() || !m_pTableView)
        return;
    QString strCurrentTreePath;
    if (!index.isValid())
    {
        QModelIndex currentTreeIndex = m_pTreeView->selectionModel()->currentIndex();
        strCurrentTreePath = m_pTreeModel->filePath(currentTreeIndex);
    }
    else
        strCurrentTreePath = m_pTreeModel->filePath(index);
    if (!strCurrentTreePath.isEmpty())
        m_pTableView->setRootIndex(m_pTableModel->index(strCurrentTreePath));
    updateLocationSelectorText(strCurrentTreePath);
}

void UIVisoHostBrowser::setTreeCurrentIndex(QModelIndex index /* = QModelIndex() */)
{
    QString strCurrentTablePath;
    if (!index.isValid())
    {
        QModelIndex currentTableIndex = m_pTableView->selectionModel()->currentIndex();
        strCurrentTablePath = m_pTableModel->filePath(currentTableIndex);
    }
    else
        strCurrentTablePath = m_pTableModel->filePath(index);
    QModelIndex treeIndex = m_pTreeModel->index(strCurrentTablePath);
    m_pTreeView->setCurrentIndex(treeIndex);
    m_pTreeView->setExpanded(treeIndex, true);
    m_pTreeView->scrollTo(treeIndex, QAbstractItemView::PositionAtCenter);
}


#include "UIVisoHostBrowser.moc"
