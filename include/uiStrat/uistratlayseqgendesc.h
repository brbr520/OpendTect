#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Oct 2010
________________________________________________________________________

-*/

#include "uistratmod.h"
#include "factory.h"

class PropertyRefSelection;

class uiParent;
class uiObject;
class uiStratLayerModelDisp;
class uiStratLayModEditTools;
namespace Strat { class LayerSequenceGenDesc; class LayerModelSuite; }


/*!\brief Base class for LayerSequenceGenDesc editors - with factory.

  A uiLayerSequenceGenDesc immediately has to produce a layer model displayer,
  which may be itself.

  The subclasses have to keep track whether anything has changed. If so,
  needSave() will return true. You can force the flag with setNeedSave().

 */

mExpClass(uiStrat) uiLayerSequenceGenDesc
{ mODTextTranslationClass(uiLayerSequenceGenDesc);
public:

    typedef Strat::LayerSequenceGenDesc	LayerSequenceGenDesc;

			uiLayerSequenceGenDesc(LayerSequenceGenDesc&);
    virtual		~uiLayerSequenceGenDesc()	{}
    mDefineFactory2ParamInClass(uiLayerSequenceGenDesc,uiParent*,
			        LayerSequenceGenDesc&,factory);

    virtual bool	separateDisplay()		{ return true; }

    virtual void	descHasChanged()		= 0;
    virtual uiObject*	outerObj()			= 0;
    virtual uiStratLayerModelDisp* getLayModDisp(uiStratLayModEditTools&,
			    Strat::LayerModelSuite&,int opt=0) = 0;

    virtual void	prepareDesc()			{}
    virtual void	setEditDesc()			{}
    virtual void	setFromEditDesc()		{}
    LayerSequenceGenDesc& desc()			{ return desc_; }
	const LayerSequenceGenDesc& currentDesc() const;
    bool		needSave() const		{ return needsave_; }
    void		setNeedSave( bool yn )		{ needsave_ = yn; }
    virtual void	setDescID(const DBKey&)		{}
    virtual bool	selProps();

protected:

    LayerSequenceGenDesc& desc_;
    bool		needsave_;
    bool		isValidSelection(const PropertyRefSelection&) const;
    virtual const uiParent* getUiParent() const		= 0;

private:
	virtual const LayerSequenceGenDesc* editedDesc() const
										{ return nullptr; }

};



#define mDefuiLayerSequenceGenDescFns(clss,typstr) \
public: \
    static const char*	typeStr()			{ return typstr; } \
    virtual const char*	factoryKeyword() const		{ return typeStr(); } \
    virtual const uiParent* getUiParent() const		{ return parent(); } \
    static uiLayerSequenceGenDesc* create( uiParent* p, \
		       LayerSequenceGenDesc& gd ) { return new clss(p,gd); } \
    static void		initClass() { factory().addCreator(create,typeStr()); }
