/*+
________________________________________________________________________

(C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
Author:        Bruno
Date:          Jan 2011
________________________________________________________________________

-*/

static const char* rcsID = "$Id: uiwelllogtools.cc,v 1.7 2011-10-19 14:11:26 cvsbruno Exp $";

#include "uiwelllogtools.h"

#include "color.h"
#include "dataclipper.h"
#include "statgrubbs.h"
#include "smoother1d.h"
#include "welldata.h"
#include "welllog.h"
#include "welllogset.h"
#include "wellmarker.h"
#include "welltransl.h"
#include "wellreader.h"
#include "wellwriter.h"

#include "uibutton.h"
#include "uicombobox.h"
#include "uigeninput.h"
#include "uigeninput.h"
#include "uilabel.h"
#include "uimsg.h"
#include "uimultiwelllogsel.h"
#include "uiseparator.h"
#include "uispinbox.h"
#include "uitaskrunner.h"
#include "uiwelllogdisplay.h"


uiWellLogToolWinMgr::uiWellLogToolWinMgr( uiParent* p )
	: uiDialog( p, Setup( "Well log tools", "Select logs", mTODOHelpID ) )
{
    setCtrlStyle( DoAndStay );
    welllogselfld_ = new uiMultiWellLogSel( this );
    welllogselfld_->update();
}


#define mErrRet(s) { uiMSG().error(s); return false; }
bool uiWellLogToolWinMgr::acceptOK( CallBacker* )
{
    BufferStringSet wellids; welllogselfld_->getSelWellIDs( wellids );
    BufferStringSet wellnms; welllogselfld_->getSelWellNames( wellnms );
    if ( wellids.isEmpty() ) mErrRet( "Please select at least one well" )

    ObjectSet<uiWellLogToolWin::LogData> logdatas; 
    for ( int idx=0; idx<wellids.size(); idx++ )
    {
	const MultiID& wid = wellids[idx]->buf();
	const char* nm = Well::IO::getMainFileName( wid );
	if ( !nm || !*nm ) continue;

	Well::Data wd; Well::Reader wr( nm, wd ); wr.getLogs(); wr.getMarkers(); 	BufferStringSet lognms; welllogselfld_->getSelLogNames( lognms );
	Well::LogSet* wls = new Well::LogSet( wd.logs() );
	uiWellLogToolWin::LogData* ldata = new uiWellLogToolWin::LogData(*wls);
	if ( !ldata->setSelectedLogs( lognms ) ) 
	    { delete ldata; continue; }
	ldata->wellid_ = wid; 
	BufferString topm; BufferString botm; float topd, botd;
	welllogselfld_->getLimitMarkers( topm, botm );
	welllogselfld_->getLimitDists( topd, botd );
	const Well::Marker* topmrk = wd.markers().getByName( topm );
	const Well::Marker* botmrk = wd.markers().getByName( botm );
	ldata->dahrg_.start = topmrk ? topmrk->dah() : wls->dahInterval().start;
	ldata->dahrg_.stop  = botmrk ? botmrk->dah() : wls->dahInterval().stop;
	ldata->dahrg_.start -= topd; 	ldata->dahrg_.stop += botd;
	ldata->wellname_ = wellnms[idx]->buf();

	logdatas += ldata;
    }
    if ( logdatas.isEmpty() )
	mErrRet("Please select at least one valid log for the selected well(s)")

    uiWellLogToolWin* win = new uiWellLogToolWin( this, logdatas );
    win->show();
    win->windowClosed.notify( mCB( this, uiWellLogToolWinMgr, winClosed ) );

    return false;
}


void uiWellLogToolWinMgr::winClosed( CallBacker* cb )
{
    mDynamicCastGet(uiWellLogToolWin*,win,cb)
    if ( !win ) pErrMsg( "can not find window" );
    if ( win->needSave() )
    {
	ObjectSet<uiWellLogToolWin::LogData> lds; win->getLogDatas( lds );
	for ( int idx=0; idx<lds.size(); idx++ )
	{
	    const char* nm = Well::IO::getMainFileName( lds[idx]->wellid_ );
	    if ( !nm || !*nm ) continue;
	    Well::Data wd; lds[idx]->getOutputLogs( wd.logs() );
	    Well::Writer wrr( nm, wd );
	    wrr.putLogs();
	}
	welllogselfld_->update();
    }
}



uiWellLogToolWin::LogData::LogData( const Well::LogSet& ls )
    : logs_(*new Well::LogSet)
{
    for ( int idx=0; idx<ls.size(); idx++ )
	logs_.add( new Well::Log( ls.getLog( idx ) ) );
}


uiWellLogToolWin::LogData::~LogData()
{
    deepErase( outplogs_ );
    delete &logs_;
}


void uiWellLogToolWin::LogData::getOutputLogs( Well::LogSet& ls ) const
{
    for ( int idx=0; idx<logs_.size(); idx++ )
	ls.add( new Well::Log( logs_.getLog( idx ) ) );
}


int uiWellLogToolWin::LogData::setSelectedLogs( BufferStringSet& lognms )
{
    int nrsel = 0;
    for ( int idx=0; idx<lognms.size(); idx++ )
    {
	Well::Log* wl = logs_.getLog( lognms[idx]->buf() );
	if ( wl ) { inplogs_ += wl; nrsel++; }
    }
    return nrsel;
}



uiWellLogToolWin::uiWellLogToolWin( uiParent* p, ObjectSet<LogData>& logs )
    : uiMainWin(p,"Log Tools Window")
    , logdatas_(logs)
    , needsave_(false)
{
    uiGroup* displaygrp = new uiGroup( this, "Well display group" );
    displaygrp->setHSpacing( 0 );
    zdisplayrg_ = logdatas_[0]->dahrg_;
    uiGroup* wellgrp; uiGroup* prevgrp = 0; uiLabel* wellnm; 
    for ( int idx=0; idx<logdatas_.size(); idx++ )
    {
	LogData& logdata = *logdatas_[idx];
	wellgrp  = new uiGroup( displaygrp, "Well group" );
	if ( prevgrp ) wellgrp->attach( rightOf, prevgrp );
	wellgrp->setHSpacing( 0 );
	wellnm = new uiLabel( wellgrp, logdata.wellname_ );
	wellnm->setVSzPol( uiObject::Small );
	for ( int idlog=0; idlog<logdata.inplogs_.size(); idlog++ )
	{
	    uiWellLogDisplay::Setup su; su.sameaxisrange_ = true;
	    uiWellLogDisplay* ld = new uiWellLogDisplay( wellgrp, su );
	    ld->setPrefWidth( 150 ); ld->setPrefHeight( 650 );
	    zdisplayrg_.include( logdata.dahrg_ );
	    if ( idlog ) ld->attach( rightOf, logdisps_[logdisps_.size()-1] );
	    ld->attach( ensureBelow, wellnm );
	    logdisps_ += ld;
	}
	prevgrp = wellgrp;
    }
    zdisplayrg_.sort();

    uiGroup* actiongrp = new uiGroup( this, "Action" );
    actiongrp->attach( hCentered );
    actiongrp->attach( ensureBelow, displaygrp );
    const char* acts[] = { "Smooth", "Remove Spikes", "Clip", 0 };
    uiLabeledComboBox* llc = new uiLabeledComboBox( actiongrp, acts, "Action" );
    actionfld_ = llc->box();
    actionfld_->selectionChanged.notify(mCB(this,uiWellLogToolWin,actionSelCB));

    CallBack cb( mCB( this, uiWellLogToolWin, applyPushedCB ) );
    applybut_ = new uiPushButton( actiongrp, "Apply", cb, true );
    applybut_->attach( rightOf, llc );

    uiLabeledSpinBox* spbgt = new uiLabeledSpinBox( actiongrp, "Window size" );
    spbgt->attach( alignedBelow, llc );
    gatefld_ = spbgt->box();
    gatelbl_ = spbgt->label();

    const char* txt = " Threshold ( Grubbs number )"; 
    thresholdfld_ = new uiLabeledSpinBox( actiongrp, txt );
    thresholdfld_->attach( rightOf, spbgt );
    thresholdfld_->box()->setInterval( 1.0, 20.0, 0.1 );
    thresholdfld_->box()->setValue( 3 );
    thresholdfld_->box()->setNrDecimals( 2 );

    const char* spk[] = {"Undefined values","Interpolated values","Specify",0};
    replacespikefld_ = new uiLabeledComboBox(actiongrp,spk,"Replace spikes by");
    replacespikefld_->box()->selectionChanged.notify( 
	    			mCB(this,uiWellLogToolWin,handleSpikeSelCB) );
    replacespikefld_->attach( alignedBelow, spbgt );

    replacespikevalfld_ = new uiGenInput( actiongrp, 0, FloatInpSpec() );
    replacespikevalfld_->attach( rightOf, replacespikefld_ );
    replacespikevalfld_->setValue( 0 );

    uiSeparator* horSepar = new uiSeparator( this );
    horSepar->attach( stretchedBelow, actiongrp );

    uiPushButton* okbut = new uiPushButton( this, "&Ok",
				mCB(this,uiWellLogToolWin,acceptOK), true );
    okbut->attach( leftBorder, 20 );
    okbut->attach( ensureBelow, horSepar );

    uiLabel* savelbl = new uiLabel( this, "On OK save logs:" );
    savelbl->attach( rightOf, okbut );
    savefld_ = new uiGenInput( this, "with extension" );
    savefld_->setElemSzPol( uiObject::Small );
    savefld_->setText( "_edited" );
    savefld_->attach( rightOf, savelbl );
    savefld_->setStretch( 0, 0 );

    overwritefld_ = new uiCheckBox( this, "Overwrite" );
    overwritefld_->attach( rightOf, savefld_ );
    overwritefld_->activated.notify( mCB(this,uiWellLogToolWin,overWriteCB) );
    overwritefld_->setStretch( 0, 0 );

    uiPushButton* cancelbut = new uiPushButton( this, "&Cancel",
				mCB(this,uiWellLogToolWin,rejectOK), true );
    cancelbut->attach( rightBorder, 20 );
    cancelbut->attach( ensureBelow, horSepar );
    cancelbut->attach( ensureRightOf, overwritefld_ );

    actionSelCB(0);
    displayLogs();
}


uiWellLogToolWin::~uiWellLogToolWin()
{
    deepErase( logdatas_ );
}


void  uiWellLogToolWin::overWriteCB(CallBacker*)
{
    savefld_->setSensitive( !overwritefld_->isChecked() );
}


void  uiWellLogToolWin::actionSelCB( CallBacker* )
{
    const int act = actionfld_->currentItem();

    thresholdfld_->display( act == 1 );
    replacespikevalfld_->display( act == 1 );
    replacespikefld_->display( act == 1 );

    gatelbl_->setText( act >1 ? "Clip rate (%)" : "Window size" );
    StepInterval<int> sp = act > 1 ? StepInterval<int>(0,100,10) 
				   : StepInterval<int>(1,1500,5);
    gatefld_->setInterval( sp );
    gatefld_->setValue( 300 );

    handleSpikeSelCB(0);
}


void uiWellLogToolWin::handleSpikeSelCB( CallBacker* )
{
    const int act = replacespikefld_->box()->currentItem();
    replacespikevalfld_->display( act == 2 );
}


bool uiWellLogToolWin::acceptOK( CallBacker* )
{
    for ( int idx=0; idx<logdatas_.size(); idx++ )
    {
	LogData& ld = *logdatas_[idx];
	Well::LogSet& ls = ld.logs_;
	bool overwrite = overwritefld_->isChecked();
	for ( int idl=ld.outplogs_.size()-1; idl>=0; idl-- )
	{
	    Well::Log* outplog = ld.outplogs_.remove( idl );
	    if ( overwrite )
	    {
		Well::Log* log = ls.getLog( outplog->name() );
		delete log; log = outplog;
	    }
	    else
	    {
		BufferString newnm( outplog->name() );
		newnm += savefld_->text();
		outplog->setName( newnm );
		if ( ls.getLog( outplog->name() ) )
		{
		    mErrRet( "One or more logs with this name already exists."
		    "\nPlease select a different extension for the new logs");
		}
		ls.add( outplog );
	    }
	    needsave_ = true; 
	}
    }
    close(); return true; 
}


bool uiWellLogToolWin::rejectOK( CallBacker* )
{ close(); return true; }


void uiWellLogToolWin::applyPushedCB( CallBacker* )
{
    const int act = actionfld_->currentItem();
    for ( int idldata=0; idldata<logdatas_.size(); idldata++ )
    {
	LogData& ld = *logdatas_[idldata]; deepErase( ld.outplogs_ );
	for ( int idlog=0; idlog<ld.inplogs_.size(); idlog++ )
	{
	    const Well::Log& inplog = *ld.inplogs_[idlog];
	    Well::Log* outplog = new Well::Log( inplog );
	    const int sz = inplog.size();
	    const int gate = gatefld_->getValue(); 
	    if ( sz< 2 || sz < 2*gate ) continue;
	    ld.outplogs_ += outplog;
	    const float* inp = inplog.valArr();
	    float* outp = outplog->valArr();
	    if ( act == 0 )
	    {
		Smoother1D<float> sm;
		sm.setInput( inp, sz );
		sm.setOutput( outp );
		const int winsz = gatefld_->getValue();
		sm.setWindow( HanningWindow::sName(), 0.95, winsz );
		sm.execute();
	    }
	    else if ( act == 1 )
	    { 
		Stats::Grubbs sgb;
		const float cutoff_grups = thresholdfld_->box()->getValue();
		TypeSet<int> grubbsidxs;
		mAllocVarLenArr( float, gatevals, gate )
		for ( int idx=gate/2; idx<sz-gate; idx+=gate  )
		{
		    float cutoffval = cutoff_grups + 1;
		    while ( cutoffval > cutoff_grups )
		    {
			for (int winidx=0; winidx<gate; winidx++)
			    gatevals[winidx]= outp[idx+winidx-gate/2];

			int idxtofix;
			cutoffval = sgb.getMax( mVarLenArr(gatevals), 
						gate, idxtofix ) ;
			if ( cutoffval > cutoff_grups  && idxtofix >= 0 )
			{
			    outp[idx+idxtofix-gate/2] = mUdf( float );
			    grubbsidxs += idx+idxtofix-gate/2;
			}
		    }
		}
		const int spkact = replacespikefld_->box()->currentItem();
		for ( int idx=0; idx<grubbsidxs.size(); idx++ )
		{
		    const int gridx = grubbsidxs[idx];
		    float& grval = outp[gridx];
		    if ( spkact == 2 )
		    {
			grval = replacespikevalfld_->getfValue();
		    }
		    else if ( spkact == 1 )
		    {
			float dah = outplog->dah( gridx );
			grval = outplog->getValue( dah, true ); 
		    }
		}
	    }
	    else if ( act == 2 )
	    {
		Interval<float> rg;
		float rate = gate/(float)100;
		DataClipSampler dcs( sz );
		dcs.add( outp, sz );
		rg = dcs.getRange( rate );
		for ( int idx=0; idx<sz; idx++ )
		{
		    if ( outp[idx] < rg.start ) outp[idx] = rg.start;
		    if ( outp[idx] > rg.stop )  outp[idx] = rg.stop;
		}
	    }
	}
    }
    displayLogs();
}


void uiWellLogToolWin::displayLogs()
{
    int nrdisp = 0;
    for ( int idx=0; idx<logdatas_.size(); idx++ )
    {
	ObjectSet<Well::Log>& inplogs = logdatas_[idx]->inplogs_;
	ObjectSet<Well::Log>& outplogs = logdatas_[idx]->outplogs_;
	for ( int idlog=0; idlog<inplogs.size(); idlog++ )
	{
	    uiWellLogDisplay* ld = logdisps_[nrdisp];
	    uiWellLogDisplay::LogData* wld = &ld->logData( true );
	    wld->wl_ = inplogs[idlog];
	    wld->disp_.color_ = Color::stdDrawColor( 1 );
	    wld->zoverlayval_ = 1;

	    wld = &ld->logData( false );
	    wld->wl_ = outplogs.validIdx( idlog ) ? outplogs[idlog] : 0;
	    wld->xrev_ = false;
	    wld->zoverlayval_ = 2;
	    wld->disp_.color_ = Color::stdDrawColor( 0 );

	    ld->setZRange( zdisplayrg_ );
	    ld->reDraw();
	    nrdisp ++;
	}
    }
}


