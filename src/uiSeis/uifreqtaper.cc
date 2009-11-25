/*+
_______________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bruno
 Date:          Nov 2009
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uifreqtaper.cc,v 1.2 2009-11-25 13:33:06 cvsbruno Exp $";

#include "uifreqtaper.h"
#include "uiamplspectrum.h"
#include "uiaxishandler.h"
#include "uigeninput.h"
#include "uigroup.h"
#include "uifunctiondisplay.h"
#include "uigraphicsscene.h"
#include "uilabel.h"
#include "uicombobox.h"
#include "uimsg.h"
#include "uiseparator.h"
#include "uislicesel.h"
#include "uislider.h"

#include "arrayndimpl.h"
#include "arrayndutils.h"
#include "iostrm.h"
#include "ctxtioobj.h"
#include "ioman.h"
#include "scaler.h"
#include "survinfo.h"
#include "seisbuf.h"
#include "seisread.h"
#include "seistrc.h"
#include "seisioobjinfo.h"
#include "seisselectionimpl.h"


uiFreqTaperDlg::uiFreqTaperDlg( uiParent* p, const FreqTaperSetup& s )
    : uiDialog( p, uiDialog::Setup("Frequency taper",
		    "Select taper parameters at cut-off frequency",mTODOHelpID))
    , cs_(new CubeSampling())		
    , posdlg_(0)
    , funcvals_(0)					
    , withpreview_(true)
{
    setCtrlStyle( LeaveOnly );
    
    if ( withpreview_ )
    {
	CallBack cbview = mCB(this,uiFreqTaperDlg,previewPushed);
	previewfld_ = new uiPushButton( this, "&Preview spectrum...", cbview, true);
    }

    uiFuncTaperDisp::Setup su;
    su.leftrg_ = s.minfreqrg_;    
    su.rightrg_ = s.maxfreqrg_; 
    su.datasz_ = (int) ( 0.5/SI().zStep() );
    su.is2sided_ = true; 
 
    su.xaxnm_ = "Frequency (Hz)"; 	
    su.yaxnm_ = "Gain (dB)";
    su.noxgridline_ = true;
    su.noygridline_ = true;
    su.ywidth_ = 2;
    su.ycol_.set( 255, 0, 0 );
    su.y2col_.set( 0, 0, 255 );

    drawer_ = new uiFuncTaperDisp( this, su );
    tapergrp_ = new uiFreqTaperGrp( this, s, drawer_ );
    tapergrp_->attach( ensureBelow, drawer_ );

    seisnm_ = s.seisnm_;

    uiSeparator* sep = new uiSeparator( this, "Sep" );
    previewfld_->attach( ensureBelow, sep );
    previewfld_->attach( centeredBelow, tapergrp_ );

    sep->attach( stretchedBelow, tapergrp_  );

    finaliseDone.notify( mCB(tapergrp_, uiFreqTaperGrp, taperChged ) );
}


uiFreqTaperDlg::~uiFreqTaperDlg()
{
    delete cs_;
    delete funcvals_;
    delete posdlg_;
}


class uiFreqTaperSelLineDlg : public uiDialog
{
public:

    uiFreqTaperSelLineDlg( uiParent* p, const SeisIOObjInfo& objinfo )
	: uiDialog(p,uiDialog::Setup("Select line from Data",0,mNoHelpID))
	, objinfo_(objinfo)  
{
    BufferString complbl ("Compute amplitude spectrum on ");
    if (  objinfo_.is2D() )
    {
	BufferStringSet linenames;
	objinfo.getLineNames( linenames );
	complbl += "line:";
	linesfld_ = new uiLabeledComboBox( this, complbl );
	for ( int idx=0; idx<linenames.size(); idx++ )
	    linesfld_->box()->addItem( linenames.get(idx) );
    }
    else
	inlcrlfld_ = new uiGenInput( this, complbl,
			    BoolInpSpec(true,"Inline","Crossline") );
    setOkText("Next >>" );
}

const char* getLineName()
{ return linesfld_->box()->text(); }

bool isInl()
{ return inlcrlfld_->getBoolValue(); }

protected:

    const SeisIOObjInfo& 	objinfo_;
    uiLabeledComboBox* 		linesfld_;
    uiGenInput*			inlcrlfld_;
};


#define mErrRet(msg) \
{ uiMSG().error(msg); return; }
void uiFreqTaperDlg::previewPushed(CallBacker*)
{
    SeisIOObjInfo objinfo( seisnm_ );  
    if ( !objinfo.isOK() )
	mErrRet( "Cannot read input data, please make sure you selected a valid data" );

    objinfo.getRanges( *cs_ );
    const int startinlval = cs_->hrg.inlRange().stop/2; 
    cs_->hrg.setInlRange( Interval<int>( startinlval, startinlval ) );

    const bool is2d = objinfo.is2D();
    uiFreqTaperSelLineDlg lineposdlg( this, objinfo );
    if ( lineposdlg.go() )
    {
	delete posdlg_; posdlg_ = 0;
	ZDomain::Info info;
	uiSliceSel::Type tp = is2d ? uiSliceSel::TwoD
				   : (lineposdlg.isInl() ? uiSliceSel::Inl 
							 : uiSliceSel::Crl);
	CallBack dummycb;
	posdlg_ = new uiSliceSelDlg( this, *cs_, *cs_, dummycb, tp, info );
	posdlg_->grp()->enableApplyButton( false );
	posdlg_->grp()->enableScrollButton( false );
	posdlg_->setModal( true );
    }

    if ( posdlg_ &&  posdlg_->go() )
    {
	const CubeSampling cs = posdlg_->getCubeSampling();
	SeisTrcReader rdr( objinfo.ioObj() );

	Seis::RangeSelData* sd = new Seis::RangeSelData( cs );
	rdr.setSelData( sd );
	rdr.prepareWork();

	SeisTrcBuf trcset( true );
	SeisBufReader sbfr( rdr, trcset );
	sbfr.execute();

	if ( !trcset.size() )
	    mErrRet( "No data extracted" );

	Array2DImpl<float> arr2d( trcset.size(), trcset.get(0)->size() );

	for ( int trcidx=0; trcidx<trcset.size(); trcidx++ )
	{
	    SeisTrc* trc = trcset.get(trcidx);
	    if ( !trc ) continue; 

	    for ( int idx=0; idx<trc->size(); idx++ )
	    {
		float val = trc->get( idx, 0 );
		if ( mIsUdf(val) )
		    val = 0;
		arr2d.set( trcidx, idx, val );
	    }
	}

	uiAmplSpectrum spec( this );
	spec.setData( arr2d  );
	delete funcvals_;
	funcvals_ = new Array1DImpl<float>( 0 ); 
	spec.getSpectrumData( *funcvals_ );
	drawer_->setup().fillbelowy2_ = true;
	drawer_->setFunction( *funcvals_, spec.getPosRange() );
    }
}



static const char* winname = "CosTaper";
#define mGetData() isminactive_ ? dd1_ : dd2_;
#define mMaxRg Interval<float>( dd2_.refrg_.start+0.05, datasz_ )
#define mMinRg Interval<float>( 0.05, dd1_.refrg_.stop )
#define mCheckLimitRanges()\
    dd1_.rg_.limitTo( mMinRg ); 	dd2_.rg_.limitTo( mMaxRg );\
    dd1_.rg_.stop = dd1_.refrg_.stop;   dd2_.rg_.start = dd2_.refrg_.start;
uiFreqTaperGrp::uiFreqTaperGrp( uiParent* p, 
				const FreqTaperSetup& s, 
				uiFuncTaperDisp* d )
    : uiGroup( p, "Frequency taper main group" )
    , drawer_(d)  
    , dd1_(d->leftTaperData())	  
    , dd2_(d->rightTaperData())	  
    , freqinpfld_(0)  
    , hasmin_(s.hasmin_)			
    , hasmax_(s.hasmax_)
    , isminactive_(s.hasmin_)
    , allfreqssetable_(s.allfreqssetable_)
    , datasz_((int)(0.5/(SI().zStep())))
{
    mCheckLimitRanges();
    setSlopeFromFreq();
    
    const char* tapertxt = "Slope (dB/Octave)";
    varinpfld_ = new uiGenInput( this, "", FloatInpSpec() );
    varinpfld_->setTitleText ( tapertxt );
    varinpfld_->setValue( dd1_.paramval_ );
    varinpfld_->valuechanged.notify(mCB( this, uiFreqTaperGrp, slopeChanged ));
    varinpfld_->valuechanged.notify( mCB(this, uiFreqTaperGrp, taperChged) );
    varinpfld_->setElemSzPol( uiObject::Small );

    inffreqfld_ = new uiGenInput( this, "Start/Stop frequency(Hz)",
				    FloatInpSpec().setName("Min frequency") );
    inffreqfld_->valuechanged.notify( mCB( this, uiFreqTaperGrp, freqChanged));
    inffreqfld_->attach( rightOf, varinpfld_ );
    inffreqfld_->setElemSzPol( uiObject::Small );
				    
    sliderfld_ = new uiSliderExtra( this, uiSliderExtra::Setup("")
				    .withedit(false)
				    .sldrsize(180)
				    .isvertical(false),
				    "slider" );
    sliderfld_->sldr()->setInterval( hasmin_ ? mMinRg : mMaxRg );
    sliderfld_->attach( rightOf, inffreqfld_ );
    sliderfld_->sldr()->valueChanged.notify( 
	    			mCB( this, uiFreqTaperGrp, sliderChanged ) );
    
    supfreqfld_ = new uiGenInput( this, "",
				    FloatInpSpec().setName("Max frequency") );
    supfreqfld_->valuechanged.notify( mCB( this, uiFreqTaperGrp, freqChanged));

    supfreqfld_->attach( rightOf, sliderfld_ );
    supfreqfld_->setElemSzPol( uiObject::Small );

    if ( hasmin_ && hasmax_ )
    {
	freqinpfld_ = new uiGenInput( this, "View ", BoolInpSpec(true, 
					"Min frequency", "Max frequency") );
	freqinpfld_->valuechanged.notify( 
			mCB(this,uiFreqTaperGrp,freqChoiceChged) );
	freqinpfld_->attach( centeredBelow, inffreqfld_ );
    }
    
    setPercentsFromFreq();
    finaliseDone.notify( mCB( this, uiFreqTaperGrp, putToScreen ) );
}


void uiFreqTaperGrp::freqChanged( CallBacker* )
{
    TaperData& dd = mGetData();
    dd.rg_.set ( inffreqfld_->getfValue(), supfreqfld_->getfValue() );

    mCheckLimitRanges();
    setPercentsFromFreq();
    setSlopeFromFreq();
    
    taperChged(0);
}


void uiFreqTaperGrp::sliderChanged( CallBacker* )
{
    NotifyStopper nsf1( inffreqfld_->valuechanged );
    NotifyStopper nsf2( supfreqfld_->valuechanged );
    float sval = sliderfld_->sldr()->getValue();
    if ( isminactive_ )
	inffreqfld_->setValue( sval );
    else
	supfreqfld_->setValue( sval );

    freqChanged(0);
}


void uiFreqTaperGrp::slopeChanged( CallBacker* )
{
    TaperData& dd = mGetData();
    dd.slope_ = varinpfld_->getfValue();
    setFreqFromSlope( dd.slope_ );
    setPercentsFromFreq();
    taperChged(0);
}


#define setToNearestInt(val)\
{\
    int ifr = mNINT( val  );\
    if ( mIsZero(val-ifr,1e-2) )\
	val = ifr;\
}
#define setTo1Decimal(val)\
{\
    val*=10;\
    val = (int)val;\
    val = (float)val/10;\
}
void uiFreqTaperGrp::putToScreen( CallBacker* )
{
    NotifyStopper nsf1( varinpfld_->valuechanged );
    NotifyStopper nsf2( inffreqfld_->valuechanged );
    NotifyStopper nsf3( supfreqfld_->valuechanged );
    NotifyStopper nsf4( sliderfld_->sldr()->valueChanged );

    TaperData& dd = mGetData();

    float freq1 = dd.rg_.start;
    float freq2 = dd.rg_.stop;

    setTo1Decimal( freq1 );
    setToNearestInt( freq1 ); 
    setTo1Decimal( freq2 );
    setToNearestInt( freq2 );
    
    inffreqfld_->setValue( freq1 );
    supfreqfld_->setValue( freq2 );

    sliderfld_->sldr()->setValue( isminactive_ ? freq1 : freq2 );

    float slope = dd.slope_;
    setTo1Decimal( slope );
    setToNearestInt( slope ); 
    varinpfld_->setValue( slope );
    
    if ( !allfreqssetable_ )
    {
	inffreqfld_->setSensitive( hasmin_ && isminactive_ );
	supfreqfld_->setSensitive( hasmax_ && !isminactive_ );
    }
} 


void uiFreqTaperGrp::setPercentsFromFreq()
{
    NotifyStopper nsf1( inffreqfld_->valuechanged );
    NotifyStopper nsf2( supfreqfld_->valuechanged );
    dd1_.paramval_ = hasmin_ ? dd1_.rg_.start / dd1_.rg_.stop : 0;
    dd2_.paramval_ = hasmax_ ? ( dd2_.rg_.stop - dd2_.rg_.start )
			       / ( datasz_ - dd2_.rg_.start ) : 0;

    drawer_->setWindows( dd1_.paramval_, dd2_.paramval_ );
}


#define mDec2Oct 0.301029996 //log(2)
void uiFreqTaperGrp::setFreqFromSlope( float slope )
{
    NotifyStopper nsf1( inffreqfld_->valuechanged );
    NotifyStopper nsf2( supfreqfld_->valuechanged );
    const float slopeindecade = (float)(slope/mDec2Oct);
    const float slopeinhertz = pow( 10, 1/slopeindecade );
    TaperData& dd = mGetData();

    if ( isminactive_ )
	dd1_.rg_.start = dd.rg_.stop/slopeinhertz;
    else
	dd2_.rg_.stop = dd.rg_.start*slopeinhertz;

    mCheckLimitRanges();
}


void uiFreqTaperGrp::setSlopeFromFreq()
{
    TaperData& d = mGetData();
    float slope = fabs( 1/Math::Log10( d.rg_.stop / d.rg_.start ) );
    d.slope_ = slope*mDec2Oct;
}


void uiFreqTaperGrp::taperChged( CallBacker* cb )
{
    drawer_->taperChged(0);
    putToScreen(0);
}


void uiFreqTaperGrp::freqChoiceChged( CallBacker* )
{
    NotifyStopper ns( sliderfld_->sldr()->valueChanged );
    if ( freqinpfld_ ) 
	isminactive_ = freqinpfld_->getBoolValue();
    else
	isminactive_ = hasmin_;

    sliderfld_->sldr()->setInterval( isminactive_ ? mMinRg : mMaxRg );

    setSlopeFromFreq();
    putToScreen(0);
}


void uiFreqTaperGrp::setFreqRange( Interval<float> fqrg )
{ 
    dd1_.rg_.start = fqrg.start;
    dd2_.rg_.stop = fqrg.stop;
    mCheckLimitRanges()
}


Interval<float> uiFreqTaperGrp::getFreqRange() const
{
    return Interval<float> ( dd1_.rg_.start, dd2_.rg_.stop );
} 




uiFuncTaperDisp::uiFuncTaperDisp( uiParent* p, const Setup& s )
    : uiFunctionDisplay( p, s )
    , is2sided_(s.is2sided_)  
    , window_(0)				
    , funcvals_(0)
    , orgfuncvals_(0)
    , displaytaper_(true)
    , taperchanged(this)    		 
{
    datasz_ = s.datasz_; 
    leftd_.rg_ = s.leftrg_;   	leftd_.refrg_ = s.leftrg_;   
    rightd_.rg_ = s.rightrg_; 	rightd_.refrg_ = s.rightrg_;
    setWindows( 0, 0 );

    xAxis()->setName( s.xaxnm_ ); 	
    yAxis(false)->setName( s.yaxnm_ );
}


uiFuncTaperDisp::~uiFuncTaperDisp()
{
    delete window_;
    delete leftd_.window_; 
    delete rightd_.window_;
    delete orgfuncvals_; 
}


void uiFuncTaperDisp::setFunction( Array1DImpl<float>& data, Interval<float> rg)
{
    int newsz_ = data.info().getSize(0);
    LinScaler scaler( 0, 0, datasz_, newsz_ );
    leftd_.rg_.stop = scaler.scale( leftd_.rg_.stop );
    leftd_.refrg_.stop = leftd_.rg_.stop;   

    rightd_.rg_.start = scaler.scale( rightd_.rg_.start );
    rightd_.refrg_.start = rightd_.rg_.start;

    datasz_ = newsz_;
    delete orgfuncvals_;
    orgfuncvals_ = new Array1DImpl<float>( data );
    funcvals_ = &data;
    funcrg_ = rg;

    setWindows( leftd_.paramval_, rightd_.paramval_ );
}



void uiFuncTaperDisp::setWindows( float leftvar, float rightvar )
{
    if ( leftvar == 1 ) leftvar -= 0.01;

    if ( is2sided_ )
    {
	delete leftd_.window_; leftd_.window_ = 0;
	leftd_.paramval_ = leftvar;
	leftd_.winsz_ = 2*(int)leftd_.rg_.stop;
	if ( leftvar )	
	    leftd_.window_ = new ArrayNDWindow( Array1DInfoImpl(leftd_.winsz_), 
						false, winname, leftvar );

	rightd_.paramval_ = rightvar;
	rightd_.winsz_ = 2*( datasz_ - (int)rightd_.rg_.start );
	delete rightd_.window_; rightd_.window_ = 0;
	if ( rightvar )	
	    rightd_.window_ = new ArrayNDWindow(Array1DInfoImpl(rightd_.winsz_),
						false, winname, 1-rightvar );
    }
    delete window_; window_ =0 ;
    window_ = new ArrayNDWindow(Array1DInfoImpl(datasz_),false,winname,leftvar);

    taperChged(0);
}


void uiFuncTaperDisp::taperChged( CallBacker* cb )
{
    if ( !window_ ) return;

    TypeSet<float> xvals;
    if ( is2sided_ )
    {
	for ( int idx=0; idx<datasz_; idx++ )
	{
	    float val = 0;
	    if ( leftd_.window_ && idx < (int)leftd_.rg_.stop )
		val = leftd_.window_->getValues()[leftd_.winsz_/2+idx];

	    if ( rightd_.window_ && idx > (int)rightd_.rg_.start ) 
		val= rightd_.window_->getValues()[idx-(int)rightd_.rg_.start];

	    xvals += idx;
	    window_->setValue( idx, 1-val );
	}
    }

    if ( funcvals_ )
    {
	setup_.drawliney2_ = displaytaper_;  
	window_->apply( orgfuncvals_, funcvals_ );
	setVals( funcrg_, window_->getValues(), datasz_ );
	setY2Vals( funcrg_, funcvals_->getData(), datasz_ );
    }
    else if ( xvals.size() ) 
    {
	setup_.drawliney_ = displaytaper_;  
	setVals( xvals.arr(), window_->getValues(), datasz_ );
    }

    taperchanged.trigger();
}





uiFreqTaperSel::uiFreqTaperSel( uiParent* p, const Setup& su )
	    : uiWindowFunctionSel( p, su )
	    , isminfreq_(su.isminfreq_)
	    , ismaxfreq_(su.ismaxfreq_)
	    , freqtaperdlg_(0)
	    , seisnm_(su.seisnm_)
{
}


void uiFreqTaperSel::winfuncseldlgCB( CallBacker* )
{
    setSelFreqs(0);
    FreqTaperSetup su;
    su.hasmin_ = isminfreq_;            
    su.hasmax_ = ismaxfreq_;
    su.minfreqrg_.set( selfreqrg_.start, freqrg_.start );
    su.maxfreqrg_.set( freqrg_.stop, selfreqrg_.stop );
    su.seisnm_ = seisnm_;

    delete freqtaperdlg_;
    freqtaperdlg_ = new uiFreqTaperDlg( this, su );
    freqtaperdlg_->windowClosed.notify( mCB(this,uiFreqTaperSel,windowClosed));
    freqtaperdlg_->show();
}


void uiFreqTaperSel::windowClosed( CallBacker* )
{
    const int winidx = windowtypefld_->getIntValue( 0 )-1;
    if ( !windowfuncs_.validIdx(winidx) && !onlytaper_ ) return;

    varinpfld_->setValue( freqtaperdlg_->getFreqRange() );
    windowChangedCB(0);
}


void uiFreqTaperSel::setIsMinMaxFreq(bool min, bool max)
{
    isminfreq_ = min; ismaxfreq_ = max;
    varinpfld_->setSensitive( min, 0, 0 );
    varinpfld_->setSensitive( max, 0 ,1 );
}


Interval<float> uiFreqTaperSel::freqValues() const
{
    return varinpfld_->getFInterval();
}


void uiFreqTaperSel::setInputFreqValue( float val, int fldnr )
{
    varinpfld_->setValue( val, fldnr );
    windowChangedCB(0);
}


void uiFreqTaperSel::setRefFreqs( Interval<float> fint )
{
    freqrg_ = fint;
}


void uiFreqTaperSel::setSelFreqs( CallBacker* )
{
    selfreqrg_.set( freqValues().start, freqValues().stop );
}

