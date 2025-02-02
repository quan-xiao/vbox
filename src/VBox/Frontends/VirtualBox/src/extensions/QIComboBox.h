/* $Id: QIComboBox.h 85135 2020-07-09 06:59:35Z vboxsync $ */
/** @file
 * VBox Qt GUI - Qt extensions: QIComboBox class declaration.
 */

/*
 * Copyright (C) 2016-2020 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#ifndef FEQT_INCLUDED_SRC_extensions_QIComboBox_h
#define FEQT_INCLUDED_SRC_extensions_QIComboBox_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

/* Qt includes: */
#include <QComboBox>

/* GUI includes: */
#include "UILibraryDefs.h"

/** QWidget subclass extending standard functionality of QComboBox. */
class SHARED_LIBRARY_STUFF QIComboBox : public QWidget
{
    Q_OBJECT;

    /** Enumerates sub-element indexes for basic case. */
    enum { SubElement_Selector, SubElement_Max };
    /** Enumerates sub-element indexes for editable case. */
    enum { SubElementEditable_Editor, SubElementEditable_Selector, SubElementEditable_Max };

signals:

    /** Notifies listeners about user chooses an item with @a iIndex in the combo-box. */
    void activated(int iIndex);
    /** Notifies listeners about user chooses an item with @a strText in the combo-box. */
    void activated(const QString &strText);

    /** Notifies listeners about current item changed to item with @a iIndex. */
    void currentIndexChanged(int iIndex);
    /** Notifies listeners about current item changed to item with @a strText. */
    void currentIndexChanged(const QString &strText);

    /** Notifies listeners about current combo-box text is changed to @a strText. */
    void currentTextChanged(const QString &strText);
    /** Notifies listeners about current combo-box editable text is changed to @a strText. */
    void editTextChanged(const QString &strText);

    /** Notifies listeners about user highlighted an item with @a iIndex in the popup list-view. */
    void highlighted(int iIndex);
    /** Notifies listeners about user highlighted an item with @a strText in the popup list-view. */
    void highlighted(const QString &strText);

public:

    /** Constructs combo-box passing @a pParent to the base-class. */
    QIComboBox(QWidget *pParent = 0);

    /** Returns sub-element count. */
    int subElementCount() const;
    /** Returns sub-element with passed @a iIndex. */
    QWidget *subElement(int iIndex) const;

    /** Returns the embedded line-editor reference. */
    QLineEdit *lineEdit() const;
    /** Returns the embedded list-view reference. */
    QAbstractItemView *view() const;

    /** Returns the size of the icons shown in the combo-box. */
    QSize iconSize() const;
    /** Returns the combo-box insert policy. */
    QComboBox::InsertPolicy insertPolicy() const;
    /** Returns whether the combo-box is editable. */
    bool isEditable() const;

    /** Returns the number of items in the combo-box. */
    int count() const;
    /** Returns the index of the current item in the combo-box. */
    int currentIndex() const;
    /** Returns the text of the current item in the combo-box. */
    QString currentText() const;
    /** Returns the data of the current item in the combo-box. */
    QVariant currentData(int iRole = Qt::UserRole) const;

    /** Adds the @a items into the combo-box. */
    void addItems(const QStringList &items) const;
    /** Adds the @a strText and userData (stored in the Qt::UserRole) into the combo-box. */
    void addItem(const QString &strText, const QVariant &userData = QVariant()) const;
    /** Inserts the @a strText and userData (stored in the Qt::UserRole) into the combo-box at the given @a iIndex. */
    void insertItem(int iIndex, const QString &strText, const QVariant &userData = QVariant()) const;
    /** Removes the item from the combo-box at the given @a iIndex. */
    void removeItem(int iIndex) const;

    /** Returns the data for the item with the given @a iIndex and specified @a iRole. */
    QVariant itemData(int iIndex, int iRole = Qt::UserRole) const;
    /** Returns the icon for the item with the given @a iIndex. */
    QIcon itemIcon(int iIndex) const;
    /** Returns the text for the item with the given @a iIndex. */
    QString itemText(int iIndex) const;

    /** Returns the index of the item containing the given @a data for the given @a iRole; otherwise returns -1.
      * @param  flags  Specifies how the items in the combobox are searched. */
    int findData(const QVariant &data, int iRole = Qt::UserRole,
                 Qt::MatchFlags flags = static_cast<Qt::MatchFlags>(Qt::MatchExactly | Qt::MatchCaseSensitive)) const;
    /** Returns the index of the item containing the given @a strText; otherwise returns -1.
      * @param  flags  Specifies how the items in the combobox are searched. */
    int findText(const QString &strText, Qt::MatchFlags flags = static_cast<Qt::MatchFlags>(Qt::MatchExactly | Qt::MatchCaseSensitive)) const;

    /** Returns size adjust policy. */
    QComboBox::SizeAdjustPolicy sizeAdjustPolicy() const;
    /** Defines size adjust @a enmPolicy. */
    void setSizeAdjustPolicy(QComboBox::SizeAdjustPolicy enmPolicy);
    /** Paints the file selectors background to a reddish color when @p fError is true. */
    void mark(bool fError);

public slots:

    /** Clears the combobox, removing all items. */
    void clear();

    /** Defines the @a size of the icons shown in the combo-box. */
    void setIconSize(const QSize &size) const;
    /** Defines the combo-box insert @a policy. */
    void setInsertPolicy(QComboBox::InsertPolicy policy) const;
    /** Defines whether the combo-box is @a fEditable. */
    void setEditable(bool fEditable) const;

    /** Defines the @a iIndex of the current item in the combo-box. */
    void setCurrentIndex(int iIndex) const;

    /** Defines the @a data for the item with the given @a iIndex and specified @a iRole. */
    void setItemData(int iIndex, const QVariant &value, int iRole = Qt::UserRole) const;
    /** Defines the @a icon for the item with the given @a iIndex. */
    void setItemIcon(int iIndex, const QIcon &icon) const;
    /** Defines the @a strText for the item with the given @a iIndex. */
    void setItemText(int iIndex, const QString &strText) const;

protected:

    /** Returns the embedded combo-box reference. */
    QComboBox *comboBox() const;

private:

    /** Prepares all. */
    void prepare();

    /** Holds the original combo-box instance. */
    QComboBox *m_pComboBox;

    /** The original background base color. Used when marking/unmarking the combo box. */
    QColor m_originalBaseColor;
};

#endif /* !FEQT_INCLUDED_SRC_extensions_QIComboBox_h */
