#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kristofer Tingdahl
 Date:		May 2006
________________________________________________________________________


-*/

#include "uiodmainmod.h"
#include "uiodattribtreeitem.h"
#include "uiodprobeparenttreeitem.h"
#include "attribsel.h"

class uiTaskRunner;


mExpClass(uiODMain) uiODLine2DParentTreeItem
    : public uiODSceneProbeParentTreeItem
{   mODTextTranslationClass(uiODLine2DParentTreeItem)
    mDefineItemMembers( Line2DParent, SceneProbeParentTreeItem, SceneTreeTop );
    mShowMenu;
    mMenuOnAnyButton;
			~uiODLine2DParentTreeItem();

    bool		init();
    int			selectionKey() const;
    void		createMenuCB(CallBacker*);
    void		handleMenuCB(CallBacker*);

    void		createMenu(MenuHandler*,bool istb);
    Probe*		createNewProbe() const;
    uiODPrManagedTreeItem* addChildItem(const OD::ObjPresentationInfo&);
    const char*		childObjTypeKey() const;
    static const char*  sKeyRightClick();
    static const char*  sKeyUnselected();

protected:
    uiVisPartServer*	visserv_;
    Pos::GeomID		geomtobeadded_;
    mutable Attrib::SelSpec selattr_;
    MenuItem		additm_;
    MenuItem		create2dgridfrom3ditm_;
    MenuItem		extractfrom3ditm_;
    MenuItem		generate3dcubeitm_;
    MenuItem		addattritm_;
    MenuItem		replaceattritm_;
    MenuItem		removeattritm_;
    MenuItem		dispattritm_;
    MenuItem		hideattritm_;
    MenuItem		editcoltabitm_;
    MenuItem		displayallitm_;
    MenuItem		hideallitm_;

    bool		getSelAttrSelSpec(Probe&,Attrib::SelSpec&) const;
    BufferStringSet	getDisplayedAttribNames() const;
    Type		getType(int) const;
};



mExpClass(uiODMain) Line2DTreeItemFactory : public uiODSceneTreeItemFactory
{ mODTextTranslationClass(Line2DTreeItemFactory);
public:
    const char*		name() const { return typeid(*this).name(); }
    uiTreeItem*		create() const
			{ return new uiODLine2DParentTreeItem; }
    uiTreeItem*		createForVis(int visid,uiTreeItem*) const;
};


mExpClass(uiODMain) uiOD2DLineTreeItem : public uiODSceneProbeTreeItem
{ mODTextTranslationClass(uiOD2DLineTreeItem);
public:
			uiOD2DLineTreeItem(Probe&,int displayid=-1);

    void		showLineName(bool);
    void		setZRange(const Interval<float>);
    void		removeAttrib(const char*);

protected:
			~uiOD2DLineTreeItem();
    bool		init();
    const char*		parentType() const;
    void		objChangedCB(CallBacker*);
    void		updateDisplay();

    void		createMenu(MenuHandler*,bool istb);
    void		handleMenuCB(CallBacker*);

private:

    MenuItem		linenmitm_;
    MenuItem		panelitm_;
    MenuItem		polylineitm_;
    MenuItem		positionitm_;
};


mExpClass(uiODMain) uiOD2DLineAttribTreeItem : public uiODAttribTreeItem
{ mODTextTranslationClass(uiOD2DLineAttribTreeItem);
public:
			uiOD2DLineAttribTreeItem(const char* parenttype);
    virtual void	updateDisplay();
    void		clearAttrib();
    static void		initClass();
    static uiODDataTreeItem* create(ProbeLayer&);
};
