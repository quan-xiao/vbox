/* $Id: UIChooserItemGroup.h 86767 2020-10-30 11:34:06Z vboxsync $ */
/** @file
 * VBox Qt GUI - UIChooserItemGroup class declaration.
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

#ifndef FEQT_INCLUDED_SRC_manager_chooser_UIChooserItemGroup_h
#define FEQT_INCLUDED_SRC_manager_chooser_UIChooserItemGroup_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

/* GUI includes: */
#include "UIChooserItem.h"

/* Forward declarations: */
class QGraphicsLinearLayout;
class QLineEdit;
class UIChooserNodeGroup;
class UIEditorGroupRename;
class UIGraphicsButton;
class UIGraphicsRotatorButton;
class UIGraphicsScrollArea;


/** UIChooserItem extension implementing group item. */
class UIChooserItemGroup : public UIChooserItem
{
    Q_OBJECT;
    Q_PROPERTY(int additionalHeight READ additionalHeight WRITE setAdditionalHeight);

signals:

    /** @name Item stuff.
      * @{ */
        /** Notifies listeners about toggle start. */
        void sigToggleStarted();
        /** Notifies listeners about toggle finish. */
        void sigToggleFinished();
    /** @} */

    /** @name Layout stuff.
      * @{ */
        /** Notifies listeners about @a iMinimumWidthHint change. */
        void sigMinimumWidthHintChanged(int iMinimumWidthHint);
    /** @} */

public:

    /** RTTI required for qgraphicsitem_cast. */
    enum { Type = UIChooserNodeType_Group };

    /** Build item for certain @a pNode, adding it directly to the @a pScene. */
    UIChooserItemGroup(QGraphicsScene *pScene, UIChooserNodeGroup *pNode);
    /** Build item for certain @a pNode, passing @a pParent to the base-class. */
    UIChooserItemGroup(UIChooserItem *pParent, UIChooserNodeGroup *pNode);
    /** Destructs group item. */
    virtual ~UIChooserItemGroup() /* override */;

    /** @name Item stuff.
      * @{ */
        /** Returns group node reference. */
        UIChooserNodeGroup *nodeToGroupType() const;
        /** Returns item machine id. */
        QUuid id() const;
        /** Returns group node type. */
        UIChooserNodeGroupType groupType() const;

        /** Returns whether group is closed. */
        bool isClosed() const;
        /** Closes group in @a fAnimated way if requested. */
        void close(bool fAnimated = true);

        /** Returns whether group is opened. */
        bool isOpened() const;
        /** Opens group in @a fAnimated way if requested. */
        void open(bool fAnimated = true);
    /** @} */

    /** @name Children stuff.
      * @{ */
        /** Updates positions of favorite items. */
        void updateFavorites();
    /** @} */

    /** @name Navigation stuff.
      * @{ */
        /** Returns scrolling location value in pixels. */
        int scrollingValue() const;
        /** Defines scrolling location @a iValue in pixels. */
        void setScrollingValue(int iValue);
        /** Performs scrolling by @a iDelta pixels. */
        void scrollBy(int iDelta);

        /** Makes sure passed @a pItem is visible within the current root item.
          * @note Please keep in mind that any group item can be a root, but there
          * is just one model root item at the same time, accessible via model's
          * root() getter, and this API can be called for current root item only,
          * because this is root item who performs actual scrolling, while
          * @a pItem itself can be on any level of embedding. */
        void makeSureItemIsVisible(UIChooserItem *pItem) /* override */;

        /** Class-name used for drag&drop mime-data format. */
        static QString className();
    /** @} */

protected:

    /** @name Event-handling stuff.
      * @{ */
        /** Handles translation event. */
        virtual void retranslateUi() /* override */;

        /** Handles show @a pEvent. */
        virtual void showEvent(QShowEvent *pEvent) /* override */;

        /** Handles resize @a pEvent. */
        virtual void resizeEvent(QGraphicsSceneResizeEvent *pEvent) /* override */;

        /** Handles hover enter @a event. */
        virtual void hoverMoveEvent(QGraphicsSceneHoverEvent *pEvent) /* override */;
        /** Handles hover leave @a event. */
        virtual void hoverLeaveEvent(QGraphicsSceneHoverEvent *pEvent) /* override */;

        /** Performs painting using passed @a pPainter, @a pOptions and optionally specified @a pWidget. */
        virtual void paint(QPainter *pPainter, const QStyleOptionGraphicsItem *pOptions, QWidget *pWidget = 0) /* override */;
    /** @} */

    /** @name Item stuff.
      * @{ */
        /** Returns RTTI item type. */
        virtual int type() const /* override */ { return Type; }

        /** Starts item editing. */
        virtual void startEditing() /* override */;

        /** Updates item. */
        virtual void updateItem() /* override */;
        /** Updates item tool-tip. */
        virtual void updateToolTip() /* override */;

        /** Installs event-filter for @a pSource object. */
        virtual void installEventFilterHelper(QObject *pSource) /* override */;
    /** @} */

    /** @name Children stuff.
      * @{ */
        /** Returns children items of certain @a enmType. */
        virtual QList<UIChooserItem*> items(UIChooserNodeType enmType = UIChooserNodeType_Any) const /* override */;

        /** Adds possible @a fFavorite child @a pItem to certain @a iPosition. */
        virtual void addItem(UIChooserItem *pItem, bool fFavorite, int iPosition) /* override */;
        /** Removes child @a pItem. */
        virtual void removeItem(UIChooserItem *pItem) /* override */;

        /** Searches for a first child item answering to specified @a strSearchTag and @a iSearchFlags. */
        virtual UIChooserItem *searchForItem(const QString &strSearchTag, int iSearchFlags) /* override */;

        /** Searches for a first machine child item. */
        virtual UIChooserItem *firstMachineItem() /* override */;
    /** @} */

    /** @name Layout stuff.
      * @{ */
        /** Updates geometry. */
        virtual void updateGeometry() /* override */;

        /** Updates layout. */
        virtual void updateLayout() /* override */;

        /** Returns minimum width-hint. */
        virtual int minimumWidthHint() const /* override */;
        /** Returns minimum height-hint. */
        virtual int minimumHeightHint() const /* override */;

        /** Returns size-hint.
          * @param  enmWhich    Brings size-hint type.
          * @param  constraint  Brings size constraint. */
        virtual QSizeF sizeHint(Qt::SizeHint enmWhich, const QSizeF &constraint = QSizeF()) const /* override */;
    /** @} */

    /** @name Navigation stuff.
      * @{ */
        /** Returns pixmap item representation. */
        virtual QPixmap toPixmap() /* override */;

        /** Returns whether item drop is allowed.
          * @param  pEvent    Brings information about drop event.
          * @param  enmPlace  Brings the place of drag token to the drop moment. */
        virtual bool isDropAllowed(QGraphicsSceneDragDropEvent *pEvent, UIChooserItemDragToken where) const /* override */;
        /** Processes item drop.
          * @param  pEvent    Brings information about drop event.
          * @param  pFromWho  Brings the item according to which we choose drop position.
          * @param  enmPlace  Brings the place of drag token to the drop moment (according to item mentioned above). */
        virtual void processDrop(QGraphicsSceneDragDropEvent *pEvent, UIChooserItem *pFromWho, UIChooserItemDragToken where) /* override */;
        /** Reset drag token. */
        virtual void resetDragToken() /* override */;

        /** Returns D&D mime data. */
        virtual QMimeData *createMimeData() /* override */;
    /** @} */

private slots:

    /** @name Item stuff.
      * @{ */
        /** Handles top-level window remaps. */
        void sltHandleWindowRemapped();

        /** Handles name editing trigger. */
        void sltNameEditingFinished();

        /** Handles group toggle start. */
        void sltGroupToggleStart();
        /** Handles group toggle finish for group finally @a fToggled. */
        void sltGroupToggleFinish(bool fToggled);
    /** @} */

private:

    /** Data field types. */
    enum GroupItemData
    {
        /* Layout hints: */
        GroupItemData_MarginHL,
        GroupItemData_MarginHR,
        GroupItemData_MarginV,
        GroupItemData_HeaderSpacing,
        GroupItemData_ChildrenSpacing,
        GroupItemData_ParentIndent,
    };

    /** @name Prepare/cleanup cascade.
      * @{ */
        /** Prepares all. */
        void prepare();
        /** Cleanups all. */
        void cleanup();
    /** @} */

    /** @name Item stuff.
      * @{ */
        /** Returns abstractly stored data value for certain @a iKey. */
        QVariant data(int iKey) const;

        /** Returns item's header darkness. */
        int headerDarkness() const { return m_iHeaderDarkness; }

        /** Returns additional height. */
        int additionalHeight() const;
        /** Defines @a iAdditionalHeight. */
        void setAdditionalHeight(int iAdditionalHeight);

        /** Updates animation parameters. */
        void updateAnimationParameters();
        /** Updates toggle-button tool-tip. */
        void updateToggleButtonToolTip();
    /** @} */

    /** @name Children stuff.
      * @{ */
        /** Copies group contents from @a pCopyFrom node recursively. */
        void copyContents(UIChooserNodeGroup *pCopyFrom);

        /** Returns whether group contains machine with @a uId. */
        bool isContainsMachine(const QUuid &uId) const;
        /** Returns whether group contains locked machine. */
        bool isContainsLockedMachine();

        /** Updates user count info. */
        void updateItemCountInfo();
    /** @} */

    /** @name Layout stuff.
      * @{ */
        /** Returns minimum width-hint depending on whether @a fGroupOpened. */
        int minimumWidthHintForGroup(bool fGroupOpened) const;
        /** Returns minimum height-hint depending on whether @a fGroupOpened. */
        int minimumHeightHintForGroup(bool fGroupOpened) const;
        /** Returns minimum size-hint depending on whether @a fGroupOpened. */
        QSizeF minimumSizeHintForGroup(bool fGroupOpened) const;

        /** Updates visible name. */
        void updateVisibleName();
        /** Updates pixmaps. */
        void updatePixmaps();
        /** Updates minimum header size. */
        void updateMinimumHeaderSize();
        /** Updates layout spacings. */
        void updateLayoutSpacings();
    /** @} */

    /** @name Painting stuff.
      * @{ */
        /** Paints background using specified @a pPainter and certain @a rect. */
        void paintBackground(QPainter *pPainter, const QRect &rect);
        /** Paints frame rectangle using specified @a pPainter and certain @a rect. */
        void paintFrame(QPainter *pPainter, const QRect &rect);
        /** Paints header using specified @a pPainter and certain @a rect. */
        void paintHeader(QPainter *pPainter, const QRect &rect);
    /** @} */

    /** @name Item stuff.
      * @{ */
        /** Holds the graphics scene reference. */
        QGraphicsScene *m_pScene;

        /** Holds the cached visible name. */
        QString  m_strVisibleName;
        /** Holds the cached group children info. */
        QString  m_strInfoGroups;
        /** Holds the cached machine children info. */
        QString  m_strInfoMachines;

        /** Holds aditional height. */
        int  m_iAdditionalHeight;
        /** Holds the header darkness. */
        int  m_iHeaderDarkness;

        /** Holds group children pixmap. */
        QPixmap  m_groupsPixmap;
        /** Holds machine children pixmap. */
        QPixmap  m_machinesPixmap;

        /** Holds the name font. */
        QFont  m_nameFont;
        /** Holds the info font. */
        QFont  m_infoFont;

        /** Holds the group toggle button instance. */
        UIGraphicsRotatorButton *m_pToggleButton;

        /** Holds the group name editor instance. */
        UIEditorGroupRename *m_pNameEditorWidget;
    /** @} */

    /** @name Children stuff.
      * @{ */
        /** Holds the favorite children container instance. */
        QIGraphicsWidget      *m_pContainerFavorite;
        /** Holds the favorite children layout instance. */
        QGraphicsLinearLayout *m_pLayoutFavorite;

        /** Holds the children scroll-area instance. */
        UIGraphicsScrollArea  *m_pScrollArea;
        /** Holds the children container instance. */
        QIGraphicsWidget      *m_pContainer;

        /** Holds the main layout instance. */
        QGraphicsLinearLayout *m_pLayout;
        /** Holds the global layout instance. */
        QGraphicsLinearLayout *m_pLayoutGlobal;
        /** Holds the group layout instance. */
        QGraphicsLinearLayout *m_pLayoutGroup;
        /** Holds the machine layout instance. */
        QGraphicsLinearLayout *m_pLayoutMachine;

        /** Holds the global children list. */
        QList<UIChooserItem*>  m_globalItems;
        /** Holds the group children list. */
        QList<UIChooserItem*>  m_groupItems;
        /** Holds the machine children list. */
        QList<UIChooserItem*>  m_machineItems;
    /** @} */

    /** @name Layout stuff.
      * @{ */
        /** Holds previous minimum width hint. */
        int  m_iPreviousMinimumWidthHint;

        /** Holds cached visible name size. */
        QSize  m_visibleNameSize;
        /** Holds cached group children pixmap size. */
        QSize  m_pixmapSizeGroups;
        /** Holds cached machine children pixmap size. */
        QSize  m_pixmapSizeMachines;
        /** Holds cached group children info size. */
        QSize  m_infoSizeGroups;
        /** Holds cached machine children info size. */
        QSize  m_infoSizeMachines;
        /** Holds cached minimum header size. */
        QSize  m_minimumHeaderSize;
        /** Holds cached toggle button size. */
        QSize  m_toggleButtonSize;
    /** @} */
};


/** QWidget extension to use as group name editor. */
class UIEditorGroupRename : public QWidget
{
    Q_OBJECT;

signals:

    /** Notifies about group editing finished. */
    void sigEditingFinished();

public:

    /** Constructs group editor with initial @a strName. */
    UIEditorGroupRename(const QString &strName);

    /** Returns editor text. */
    QString text() const;
    /** Defines editor @a strText. */
    void setText(const QString &strText);

    /** Defines editor @a font. */
    void setFont(const QFont &font);

private:

    /** Holds the line-edit instance. */
    QLineEdit *m_pLineEdit;
};


#endif /* !FEQT_INCLUDED_SRC_manager_chooser_UIChooserItemGroup_h */
