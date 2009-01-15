#ifndef uiflatviewmainwin_h
#define uiflatviewmainwin_h
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert
 Date:          Feb 2007
 RCS:           $Id: uiflatviewmainwin.h,v 1.9 2009-01-15 09:13:51 cvsnanne Exp $
________________________________________________________________________

-*/

#include "uiflatviewwin.h"
#include "uimainwin.h"


/*!\brief (Non-modal) main window containing one or more uiFlatViewer(s). */

mClass uiFlatViewMainWin : public uiMainWin
			, public uiFlatViewWin
{
public:

    struct Setup
    {
					Setup( const char* wintitl,
					       bool delonclose=true )
					    : wintitle_(wintitl)
					    , nrviewers_(1)
					    , nrstatusfields_(1)
					    , deleteonclose_(delonclose)
					    , menubar_(false)		{}
	mDefSetupMemb(BufferString,wintitle)
	mDefSetupMemb(int,nrviewers)
	mDefSetupMemb(int,nrstatusfields)
	mDefSetupMemb(bool,deleteonclose)
	mDefSetupMemb(bool,menubar)
    };

    			uiFlatViewMainWin(uiParent*,const Setup&);

    virtual void	start()				{ show(); }
    virtual void	setWinTitle( const char* t )	{ setCaption(t); }

    void		addControl(uiFlatViewControl*);
    void		displayInfo(CallBacker*);

    virtual uiMainWin*	dockParent()			{ return this; }
    virtual uiParent*	viewerParent()			{ return this; }

};


#endif
