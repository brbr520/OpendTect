/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Feb 2007
________________________________________________________________________

-*/

#include "uibitmapdisplay.h"

#include "uigraphicsitemimpl.h"
#include "uipixmap.h"
#include "uirgbarray.h"

#include "bitmap2rgb.h"
#include "bitmapmgr.h"
#include "datapackbase.h"
#include "datadistributionextracter.h"
#include "flatposdata.h"
#include "flatview.h"
#include "thread.h"
#include "threadlock.h"
#include "threadwork.h"


static bool isVisible( const FlatView::Appearance& app, bool wva )
{ return wva ? app.ddpars_.wva_.show_ : app.ddpars_.vd_.show_; }


class uiBitMapDisplayTask : public Task
{
public:

    typedef ColTab::Mapper	Mapper;

uiBitMapDisplayTask( FlatView::Appearance& app,
		     uiDynamicImageItem& display, bool isdynamic,
		     bool withalpha )
    : appearance_(app)
    , image_(new uiRGBArray(withalpha))
    , bitmap2image_(new BitMap2RGB(app,*image_))
    , display_(display)
    , isdynamic_(isdynamic)
    , wvabmpmgr_(0), vdbmpmgr_(0)
    , wvapack_(0), vdpack_(0)
    , setupdone_(false)
{
}


~uiBitMapDisplayTask()
{
    delete image_;
    delete bitmap2image_;
    delete wvabmpmgr_;
    delete vdbmpmgr_;
}


void setScope( const uiWorldRect& wr, const uiSize& sz )
{
    Threads::Locker lckr( lock_ );
    wr_ = wr; sz_ = sz;
}


void setDataPack( const FlatDataPack* fdp, bool wva )
{
    Threads::Locker lckr( lock_ );
    (wva ? wvapack_ : vdpack_) = fdp;
    if ( !fdp || fdp->nrArrays() < 1 )
	return;

    Mapper& mapper = getMapper( wva );
    if ( mapper.distribution().isEmpty() )
    {
	const ArrayND<float>* dataarr = fdp->arrayData( 0 );
	if ( dataarr->info().getTotalSz() )
	{
	    RangeLimitedDataDistributionExtracter<float> extracter(
							*fdp->arrayData(0) );
	    mapper.distribution() = *extracter.getDistribution();
	}
    }
    setupdone_ = false;
}


void reset()
{
    Threads::Locker lckr( lock_ );
    setupdone_ = false;
    if ( wvabmpmgr_ ) wvabmpmgr_->clearAll();
    if ( vdbmpmgr_ ) vdbmpmgr_->clearAll();
}


void setupBMPMgrs()
{
    Threads::Locker lckr( lock_ );
    if ( setupdone_ )
	return;

    setupdone_ = false;
    // WVA
    if ( !wvabmpmgr_ )
	wvabmpmgr_ = new BitMapMgr();

    if ( isVisible(appearance_,true) )
	wvabmpmgr_->init( wvapack_.ptr(), appearance_, true );

    // VD
    if ( !vdbmpmgr_ )
	vdbmpmgr_ = new BitMapMgr();

    if ( isVisible(appearance_,false) )
	vdbmpmgr_->init( vdpack_.ptr(), appearance_, false );
    setupdone_ = true;
}


bool execute()
{
    image_->setSize( sz_.width(), sz_.height() );

    Threads::Locker lckr( lock_ );

    TypeSet<Threads::Work> tasks;

    setupBMPMgrs();
// WVA
    if ( isVisible(appearance_,true) )
    {
	BitMapGenTask* wvatask = new BitMapGenTask( *wvabmpmgr_, wr_, sz_, sz_);
	tasks += Threads::Work( *wvatask, true );
    }

// VD
    if ( isVisible(appearance_,false) )
    {
	BitMapGenTask* vdtask = new BitMapGenTask( *vdbmpmgr_, wr_, sz_, sz_ );
	tasks += Threads::Work( *vdtask, true );
    }

    if ( !Threads::WorkManager::twm().addWork( tasks,
	       Threads::WorkManager::cDefaultQueueID() ) )
    {
	return false;
    }

    bitmap2image_->draw( wvabmpmgr_->bitMap(), vdbmpmgr_->bitMap(),
			 uiPoint(0,0), true );

    display_.setImage( isdynamic_, *image_, wr_ );
    display_.setVisible( !tasks.isEmpty() );
    return true;
}


Mapper& getMapper( bool iswva )
{
    return *(iswva ? appearance_.ddpars_.wva_.mapper_
		   : appearance_.ddpars_.vd_.mapper_);
}


Interval<float> getBitmapDataRange( bool iswva )
{
    return getMapper(iswva).getRange();
}

    bool			isdynamic_;
    bool			setupdone_;

    FlatView::Appearance&	appearance_;
    ConstRefMan<FlatDataPack>	wvapack_;
    ConstRefMan<FlatDataPack>	vdpack_;
    BitMapMgr*			wvabmpmgr_;
    BitMapMgr*			vdbmpmgr_;
    uiRGBArray*			image_;
    BitMap2RGB*			bitmap2image_;
    uiSize			sz_;
    uiWorldRect			wr_;
    uiDynamicImageItem&		display_;
    Threads::Lock		lock_;

};



uiBitMapDisplay::uiBitMapDisplay( FlatView::Appearance& app, bool withalpha )
    : appearance_(app)
    , display_(new uiDynamicImageItem)
    , withalpha_(withalpha)
    , basetask_(new uiBitMapDisplayTask(app,*display_,false,withalpha))
    , finishedcb_(mCB( this, uiBitMapDisplay, dynamicTaskFinishCB ))
    , overlap_(0.5f)
    , wvapack_(0), vdpack_(0)
    , rangeUpdated(this)
{
    const int nrcpu = Threads::getNrProcessors();
    if ( nrcpu<4 )
	overlap_ = 0.25f;
    else if ( nrcpu<2 )
	overlap_ = 0.1f;

    display_->wantsData().notify( mCB(this,uiBitMapDisplay,reGenerateCB) );
    workqueueid_ = Threads::WorkManager::twm().addQueue(
				Threads::WorkManager::SingleThread,
				"BitmapDisplay");
}


uiBitMapDisplay::~uiBitMapDisplay()
{
    Threads::WorkManager::twm().removeQueue( workqueueid_, false );

    delete basetask_;
    delete display_;
}



void uiBitMapDisplay::removeDisplay()
{
    if ( display_ )
	display_->wantsData().remove( mCB(this,uiBitMapDisplay,reGenerateCB) );

    display_ = 0;
}


bool uiBitMapDisplay::isVisible( bool wva ) const
{
    return wva ? appearance_.ddpars_.wva_.show_
	       : appearance_.ddpars_.vd_.show_;
}


StepInterval<double>
	uiBitMapDisplay::getDataPackRange( bool wva, bool x1 ) const
{
    const FlatDataPack* fdp = wva ? wvapack_.ptr() : vdpack_.ptr();
    return fdp ? fdp->posData().range(x1) : StepInterval<double>::udf();
}


const uiWorldRect& uiBitMapDisplay::boundingBox() const
{ return boundingbox_; }

void uiBitMapDisplay::setBoundingBox( const uiWorldRect& bb )
{ boundingbox_ = bb; }


void uiBitMapDisplay::setDataPack( const FlatDataPack* fdp, bool wva )
{
    if ( wva && wvapack_.ptr() != fdp )
    {
	wvapack_ = fdp;
    }
    if ( !wva && vdpack_.ptr() != fdp )
    {
	vdpack_ = fdp;
    }
}


void uiBitMapDisplay::update()
{
    basetask_->reset();

    if ( !isVisible(true) && !isVisible(false) )
    {
	display_->setVisible( false );
	return;
    }

    PtrMan<Task> dynamictask = createDynamicTask( false );

    uiWorldRect wr( boundingBox() );
    wr.swapVer();

    const bool wva = isVisible( true );
    const uiSize sz( getDataPackRange(wva,true).nrSteps()+1,
		     getDataPackRange(wva,false).nrSteps()+1 );
    basetask_->setScope( wr, sz );

    TypeSet<Threads::Work> updatework;
    if ( dynamictask )
	updatework += Threads::Work( *dynamictask, false );
    updatework += Threads::Work( *basetask_, false );
    if ( !Threads::WorkManager::twm().addWork( updatework ) )
	return;

    display_->setVisible( true );
}



uiGraphicsItem* uiBitMapDisplay::getDisplay()
{
    return display_;
}


void uiBitMapDisplay::reGenerateCB(CallBacker*)
{
    const bool issnapshot = display_->isSnapshot();

    Task* dynamictask = createDynamicTask( issnapshot );
    if ( !dynamictask )
	return;

    const int queueid = issnapshot ? Threads::WorkManager::cDefaultQueueID()
				   : workqueueid_;
    if ( !issnapshot )
	Threads::WorkManager::twm().emptyQueue( queueid, false );

    Threads::WorkManager::twm().addWork(
	    Threads::Work(*dynamictask,true) , &finishedcb_, queueid, true );
}


Task* uiBitMapDisplay::createDynamicTask( bool issnapshot  )
{
    if ( !display_ )
	return 0;

    const uiWorldRect wr = display_->wantedWorldRect();
    const uiSize sz = display_->wantedScreenSize();
    if ( sz.width()<=0 || sz.height()<=0 )
	return 0;

    uiBitMapDisplayTask* dynamictask =
	new uiBitMapDisplayTask( appearance_, *display_, true, withalpha_ );
    dynamictask->setDataPack( wvapack_.ptr(), true );
    dynamictask->setDataPack( vdpack_.ptr(), false );

    uiWorldRect computewr = wr;
    uiSize computesz = sz;
    if ( !issnapshot )
    {
	const double expandx = wr.width()*overlap_ * (wr.revX() ? -1 : 1 );
	const double expandy = wr.height()*overlap_ * (wr.revY() ? 1 : -1 );

	computewr = uiWorldRect( wr.left()-expandx, wr.top()-expandy,
				 wr.right()+expandx, wr.bottom()+expandy );
	computewr.limitTo( boundingBox() );
	computesz = uiSize(
		mNINT32(sz.width()/wr.width()*computewr.width()),
		mNINT32(sz.height()/wr.height()*computewr.height()) );
    }

    dynamictask->setScope( computewr, computesz );

    return dynamictask;
}


void uiBitMapDisplay::dynamicTaskFinishCB( CallBacker* )
{
}
