/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          June 2001
 RCS:           $Id: uiconvpos.cc,v 1.21 2007-08-13 13:33:07 cvsjaap Exp $
________________________________________________________________________

-*/

#include "uiconvpos.h"
#include "pixmap.h"
#include "survinfo.h"
#include "uibutton.h"
#include "uidialog.h"
#include "uigeninput.h"


uiConvertPos::uiConvertPos( uiParent* p, SurveyInfo* si )
	: uiDialog(p, uiDialog::Setup("Position conversion",
		   "Coordinates vs Inline/X-line","0.3.7"))
	, survinfo(si)
{
    uiGroup* fldgrp = new uiGroup( this );
    inlfld = new uiGenInput( fldgrp, "In-line", 
			     IntInpSpec().setName("Inl-field") );
    inlfld->setElemSzPol( uiObject::Small );
    crlfld = new uiGenInput( fldgrp, "Cross-line", 
	    		     IntInpSpec().setName("Crl-field") );
    crlfld->setElemSzPol( uiObject::Small );
    xfld = new uiGenInput( fldgrp, "X-coordinate",
			   DoubleInpSpec().setName("X-field") );
    xfld->setElemSzPol( uiObject::Small );
    yfld = new uiGenInput( fldgrp, "Y-coordinate", 
	    		   DoubleInpSpec().setName("Y-field") );
    yfld->setElemSzPol( uiObject::Small );
    crlfld->attach( alignedBelow, inlfld );
    xfld->attach( rightTo, inlfld );
    yfld->attach( alignedBelow, xfld );
    yfld->attach( rightTo, crlfld );

    uiGroup* butgrp = new uiGroup( this );
    const ioPixmap right( "forward.xpm" );
    const ioPixmap left( "back.xpm" );
    uiToolButton* dobinidbut = new uiToolButton( butgrp, "Left", left );
    dobinidbut->activated.notify( mCB(this,uiConvertPos,getBinID) );
    uiToolButton* docoordbut = new uiToolButton( butgrp, "Right", right );
    docoordbut->activated.notify( mCB(this,uiConvertPos,getCoord) );
    docoordbut->attach( rightOf, dobinidbut, 0 );

    fldgrp->attach( centeredBelow, butgrp );
    setCtrlStyle( LeaveOnly );
}


void uiConvertPos::getCoord()
{
    BinID binid( inlfld->getIntValue(), crlfld->getIntValue() );
    if ( mIsUdf(binid.inl) || mIsUdf(binid.crl) )
    {
	xfld->setText( "" ); yfld->setText( "" );
	return;
    }

    Coord coord( survinfo->transform( binid ) );
    xfld->setValue( coord.x );
    yfld->setValue( coord.y );
    inlfld->setValue( binid.inl );
    crlfld->setValue( binid.crl );
}


void uiConvertPos::getBinID()
{
    Coord coord( xfld->getdValue(), yfld->getdValue() );
    BinID binid( survinfo->transform( coord ) );
    inlfld->setValue( binid.inl );
    crlfld->setValue( binid.crl );
    if ( mIsZero(coord.x,mDefEps) ) xfld->setValue( 0 );
    if ( mIsZero(coord.y,mDefEps) ) yfld->setValue( 0 );
}
