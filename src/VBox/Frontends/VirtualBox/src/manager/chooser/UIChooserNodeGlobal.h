/* $Id: UIChooserNodeGlobal.h 84625 2020-06-01 16:44:39Z vboxsync $ */
/** @file
 * VBox Qt GUI - UIChooserNodeGlobal class declaration.
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

#ifndef FEQT_INCLUDED_SRC_manager_chooser_UIChooserNodeGlobal_h
#define FEQT_INCLUDED_SRC_manager_chooser_UIChooserNodeGlobal_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

/* GUI includes: */
#include "UIChooserNode.h"


/** UIChooserNode subclass used as interface for invisible tree-view global nodes. */
class UIChooserNodeGlobal : public UIChooserNode
{
    Q_OBJECT;

public:

    /** Constructs chooser node passing @a pParent to the base-class.
      * @param  iPosition  Brings the initial node position.
      * @param  fFavorite  Brings whether the node is favorite.
      * @param  strTip     Brings the dummy tip. */
    UIChooserNodeGlobal(UIChooserNode *pParent,
                        int iPosition,
                        bool fFavorite,
                        const QString &strTip);
    /** Constructs chooser node passing @a pParent to the base-class.
      * @param  iPosition  Brings the initial node position.
      * @param  pCopyFrom  Brings the node to copy data from. */
    UIChooserNodeGlobal(UIChooserNode *pParent,
                        int iPosition,
                        UIChooserNodeGlobal *pCopyFrom);
    /** Destructs chooser node. */
    virtual ~UIChooserNodeGlobal() /* override */;

    /** Returns RTTI node type. */
    virtual UIChooserNodeType type() const /* override */ { return UIChooserNodeType_Global; }

    /** Returns node name. */
    virtual QString name() const /* override */;
    /** Returns full node name. */
    virtual QString fullName() const /* override */;
    /** Returns item description. */
    virtual QString description() const /* override */;
    /** Returns item definition.
      * @param  fFull  Brings whether full definition is required
      *                which is used while saving group definitions,
      *                otherwise short definition will be returned,
      *                which is used while saving last chosen node. */
    virtual QString definition(bool fFull = false) const /* override */;

    /** Returns whether there are children of certain @a enmType. */
    virtual bool hasNodes(UIChooserNodeType enmType = UIChooserNodeType_Any) const /* override */;
    /** Returns a list of nodes of certain @a enmType. */
    virtual QList<UIChooserNode*> nodes(UIChooserNodeType enmType = UIChooserNodeType_Any) const /* override */;

    /** Adds passed @a pNode to specified @a iPosition. */
    virtual void addNode(UIChooserNode *pNode, int iPosition) /* override */;
    /** Removes passed @a pNode. */
    virtual void removeNode(UIChooserNode *pNode) /* override */;

    /** Removes all children with specified @a uId recursively. */
    virtual void removeAllNodes(const QUuid &uId) /* override */;
    /** Updates all children with specified @a uId recursively. */
    virtual void updateAllNodes(const QUuid &uId) /* override */;

    /** Returns position of specified node inside this one. */
    virtual int positionOf(UIChooserNode *pNode) /* override */;

    /** Updates the @a matchedItems wrt. @strSearchTerm and @a iSearchFlags. */
    virtual void searchForNodes(const QString &strSearchTerm, int iSearchFlags, QList<UIChooserNode*> &matchedItems) /* override */;

    /** Performs sorting of children nodes. */
    virtual void sortNodes() /* override */;

protected:

    /** Handles translation event. */
    virtual void retranslateUi() /* override */;

private:

    /** Holds the node name. */
    QString  m_strName;
};


#endif /* !FEQT_INCLUDED_SRC_manager_chooser_UIChooserNodeGlobal_h */
