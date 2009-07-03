/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Bruno
 Date:		Apr 2009
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uiwelltieeventstretch.cc,v 1.13 2009-07-03 15:13:13 cvsbruno Exp $";

#include "arrayndimpl.h"
#include "uiwelltieeventstretch.h"

#include "welld2tmodel.h"
#include "welltiedata.h"
#include "welltied2tmodelmanager.h"
#include "welltiegeocalculator.h"
#include "welltieunitfactors.h"
#include "welltiedata.h"
#include "welltiepickset.h"
#include "welltiesetup.h"
#include "uiwelltieview.h"

uiWellTieEventStretch::uiWellTieEventStretch( uiParent* p, 
					      WellTieDataHolder* dh, 
			 		      uiWellTieView& v )
        : d2tmgr_(dh->d2TMGR())
  	, pmgr_(*dh->pickmgr())  
	, readyforwork(this)
	, pickadded(this)
	, synthpickset_(*dh->pickmgr()->getSynthPickSet())
	, seispickset_(*dh->pickmgr()->getSeisPickSet())
	, uiWellTieStretch(p,dh,v)
{
    synthpickset_.pickadded.notify(mCB(this,uiWellTieEventStretch,addSyntPick));
    seispickset_.pickadded.notify(mCB(this,uiWellTieEventStretch,addSeisPick));
} 


uiWellTieEventStretch::~uiWellTieEventStretch()
{
    synthpickset_.pickadded.remove(mCB(this,uiWellTieEventStretch,addSyntPick));
    seispickset_.pickadded.remove(mCB(this,uiWellTieEventStretch,addSeisPick));
}


void uiWellTieEventStretch::addSyntPick( CallBacker* )
{
    dataviewer_.drawUserPicks();
    checkReadyForWork();
    pickadded.trigger();
}


void uiWellTieEventStretch::addSeisPick( CallBacker* )
{
    dataviewer_.drawUserPicks();
    checkReadyForWork();
    pickadded.trigger();
}


void uiWellTieEventStretch::checkReadyForWork()
{
    if ( seispickset_.getSize() && synthpickset_.getSize() )
	readyforwork.trigger();
}


void uiWellTieEventStretch::doWork( CallBacker* )
{
    pmgr_.sortByDah( seispickset_ ); 	
    pmgr_.sortByDah( synthpickset_ );
    doStretchWork();	
    timeChanged.trigger();
}


void uiWellTieEventStretch::doStretchWork()
{
    Well::D2TModel d2t = *wd_->d2TModel();
    Array1DImpl<float> d2tarr( d2t.size() );
    Array1DImpl<float>* prvd2tarr = new Array1DImpl<float>( d2t.size() );
    float timeshift = time( seispickset_.getLastDah() ) 
		     -time( synthpickset_.getLastDah() ) ;
    for ( int idx=0; idx<d2t.size(); idx++ )
    {
	prvd2tarr->set( idx, d2t.value(idx) );
	d2tarr.set( idx, d2t.value(idx)+timeshift );
    }

    updatePicksPos( d2tarr, *prvd2tarr, synthpickset_, 0 );
    infborderpos_ = 0;
    supborderpos_ = time( seispickset_.getLastDah() );

    for ( int idx=0; idx<seispickset_.getSize()-1; idx++ )
    {
	if ( idx && idx<seispickset_.getSize()-1 )
	    infborderpos_ = time( seispickset_.getDah(idx-1) );

	startpos_ = time( synthpickset_.getDah(idx) );
	stoppos_  = time( seispickset_.getDah(idx) );

	delete prvd2tarr; prvd2tarr = 0;
	prvd2tarr = new Array1DImpl<float>( d2tarr );

	doStretchData( *prvd2tarr, d2tarr );
	//position of the following picks needs update if one of the pick moved
	updatePicksPos( d2tarr, *prvd2tarr, synthpickset_, idx );
    }
    delete prvd2tarr;
    d2tmgr_->replaceTime( d2tarr );
}


void uiWellTieEventStretch::updatePicksPos( const Array1DImpl<float>& curtime,
					    const Array1DImpl<float>& prevtime,
					    WellTiePickSet& pickset, 
					    int startidx )
{
    for ( int pickidx=startidx; pickidx<pickset.getSize(); pickidx++ )
    {
	float pos = time( pickset.getDah( pickidx ) );
	const int newidx = geocalc_->getIdx( prevtime, pos );
	pos = curtime.get( newidx ); 	
	pickset.setDah( pickidx, dah(pos) );
    }
}


