/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          July 2003
 RCS:           $Id: uimultisurfaceread.cc,v 1.9 2007-05-07 10:20:45 cvsjaap Exp $
________________________________________________________________________

-*/

#include "uimultisurfaceread.h"

#include "uibinidsubsel.h"
#include "uilistbox.h"
#include "ioobj.h"
#include "ctxtioobj.h"
#include "uiioobjsel.h"
#include "ioman.h"
#include "iodirentry.h"
#include "emmanager.h"
#include "emsurfaceiodata.h"
#include "emsurfacetr.h"
#include "uimsg.h"


uiMultiSurfaceRead::uiMultiSurfaceRead( uiParent* p, const BufferString& typ )
    : uiIOSurface(p,true,typ)
    , singleSurfaceSelected(this)
{
    IOM().to( ctio.ctxt.getSelKey() );
    entrylist_ = new IODirEntryList( IOM().dirPtr(), ctio.ctxt );
    entrylist_->sort();

    BufferString lbl( "Select " ); lbl += typ; lbl += "(s)";
    surfacefld_ = new uiLabeledListBox( this, lbl, true,
				       uiLabeledListBox::AboveMid );
    for ( int idx=0; idx<entrylist_->size(); idx++ )
	surfacefld_->box()->addItem( (*entrylist_)[idx]->name() );
    surfacefld_->box()->setSelected( 0 );
    surfacefld_->box()->selectionChanged.notify(
	    				mCB(this,uiMultiSurfaceRead,selCB) );
    surfacefld_->box()->doubleClicked.notify(
	    				mCB(this,uiMultiSurfaceRead,dClck) );

    mkSectionFld( true );
    sectionfld->attach( rightTo, surfacefld_ );

    mkRangeFld();
    rgfld->attach( leftAlignedBelow, surfacefld_ );

    if ( typ == EMHorizon2DTranslatorGroup::keyword )
    {
	sectionfld->display( false, true );
	rgfld->display( false, true );
    }
}


uiMultiSurfaceRead::~uiMultiSurfaceRead()
{
    delete entrylist_;
}


void uiMultiSurfaceRead::dClck( CallBacker* )
{
    singleSurfaceSelected.trigger();
}


void uiMultiSurfaceRead::selCB( CallBacker* )
{
    const int nrsel = surfacefld_->box()->nrSelected();
    if ( nrsel > 1 )
    {
	EM::SurfaceIOData sd;
	HorSampling hs( false );
	for ( int idx=0; idx<surfacefld_->box()->size(); idx++ )
	{
	    if ( !surfacefld_->box()->isSelected(idx) ) continue;
	    const MultiID& mid = (*entrylist_)[idx]->ioobj->key();
	    const char* res = EM::EMM().getSurfaceData( mid, sd );
	    if ( res ) continue;
	    if ( hs.isEmpty() )
		hs = sd.rg;
	    else
	    {
		hs.include( sd.rg.start );
		hs.include( sd.rg.stop );
	    }
	}

	fillRangeFld( hs );
	sectionfld->box()->empty();
	return;
    }

    for ( int idx=0; idx<surfacefld_->box()->size(); idx++ )
    {
	if ( !surfacefld_->box()->isSelected(idx) ) continue;
	fillFields( (*entrylist_)[idx]->ioobj->key() );
	break;
    }
}


void uiMultiSurfaceRead::getSurfaceIds( TypeSet<MultiID>& mids ) const
{
    mids.erase();
    for ( int idx=0; idx<surfacefld_->box()->size(); idx++ )
    {
	if ( surfacefld_->box()->isSelected(idx) )
	    mids += (*entrylist_)[idx]->ioobj->key();
    }
}


void uiMultiSurfaceRead::getSurfaceSelection( 
					EM::SurfaceIODataSelection& sel ) const
{
    uiIOSurface::getSelection( sel );
}
