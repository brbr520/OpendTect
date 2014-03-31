/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Mar 2014
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "uiprobdenfuncgen.h"
#include "uieditpdf.h"

#include "uigeninput.h"
#include "uispinbox.h"
#include "uichecklist.h"
#include "uiioobjsel.h"
#include "uiseparator.h"
#include "uilabel.h"
#include "uimsg.h"

#include "sampledprobdenfunc.h"
#include "gaussianprobdenfunc.h"
#include "probdenfunctr.h"
#include "statruncalc.h"


static const float cMaxSampledBinVal = 100.0f;


static bool writePDF( const ProbDenFunc& pdf, const IOObj& ioobj )
{
    BufferString emsg;
    if ( ProbDenFuncTranslator::write(pdf,ioobj,&emsg) )
	return true;

    const char* msg = emsg.buf();
    if ( !msg || !*msg )
	msg = "Could not write PDF to disk";
    uiMSG().error( msg );
    return false;
}


class uiProbDenFuncGenSampled : public uiDialog
{
public:

			uiProbDenFuncGenSampled(uiParent*,int nrdim,bool gauss,
						MultiID&);

    uiSpinBox*		nrbinsfld_;
    ObjectSet<uiGenInput> nmflds_;
    ObjectSet<uiGenInput> rgflds_;
    ObjectSet<uiGenInput> expstdflds_;
    ObjectSet<uiGenInput> ccflds_;
    uiIOObjSel*		outfld_;

    MultiID&		ioobjky_;
    const int		nrdims_;
    int			nrbins_;
    TypeSet<Interval<float> > rgs_;
    TypeSet<float>	exps_;
    TypeSet<float>	stds_;
    TypeSet<float>	ccs_;
    BufferStringSet	dimnms_;

    inline bool		isGauss() const	{ return !expstdflds_.isEmpty(); }
    bool		getFromScreen();
    ProbDenFunc*	getPDF() const;

    bool		acceptOK(CallBacker*);
    void		chgCB(CallBacker*);
    void		rgChg(CallBacker*);
};


class uiProbDenFuncGenGaussian : public uiDialog
{
public:

			uiProbDenFuncGenGaussian(uiParent*,int nrdim,MultiID&);
			~uiProbDenFuncGenGaussian();

    MultiID&		ioobjky_;

    ProbDenFunc*	pdf_;
    uiEditGaussianProbDenFunc* pdffld_;
    uiIOObjSel*		outfld_;

    bool		acceptOK(CallBacker*);

};



uiProbDenFuncGen::uiProbDenFuncGen( uiParent* p )
    : uiDialog(p,Setup("Generate a PDF",mNoDlgTitle,"112.1.2"))
{

    choicefld_ = new uiCheckList( this, uiCheckList::OneOnly );
    choicefld_->addItem( "Create an editable PDF &filled with Gaussian values");
    choicefld_->addItem( "Create a full &Gaussian PDF" );
    choicefld_->addItem( "Create an &empty PDF to edit by hand" );
    choicefld_->changed.notify( mCB(this,uiProbDenFuncGen,choiceSel) );

    uiLabeledSpinBox* lsb = new uiLabeledSpinBox( this,
				"Number of dimensions (variables)" );
    lsb->attach( ensureBelow, choicefld_ );
    nrdimfld_ = lsb->box();
    nrdimfld_->setInterval( 1, 3, 1 );
    nrdimfld_->setValue( 2 );
}


void uiProbDenFuncGen::choiceSel( CallBacker* )
{
    const int choice = choicefld_->firstChecked();
    const bool isfullgauss = choice == 1;
    nrdimfld_->setInterval( 1, isfullgauss ? 20 : 3, 1 );
}


bool uiProbDenFuncGen::acceptOK( CallBacker* )
{
    const int choice = choicefld_->firstChecked();
    const int nrdims = nrdimfld_->getValue();

    if ( choice == 1 )
    {
	uiProbDenFuncGenGaussian dlg( this, nrdims, ioobjky_ );
	return dlg.go();
    }

    uiProbDenFuncGenSampled dlg( this, nrdims, choice==0, ioobjky_ );
    return dlg.go();
}



uiProbDenFuncGenSampled::uiProbDenFuncGenSampled( uiParent* p, int nrdim,
						  bool isgauss, MultiID& ky )
    : uiDialog(p,Setup("Generate editable PDF",mNoDlgTitle,mTODOHelpKey))
    , nrdims_(nrdim)
    , ioobjky_(ky)
{
    for ( int idx=0; idx<nrdims_; idx++ )
    {
	uiGenInput* nmfld = new uiGenInput( this, nrdims_ == 1 ?
		"Variable name" : BufferString("Dimension ",idx+1,": Name") );
	uiGenInput* rgfld = new uiGenInput( this, "Value range",
				FloatInpSpec(), FloatInpSpec() );
	rgfld->valuechanged.notify( mCB(this,uiProbDenFuncGenSampled,rgChg) );
	nmflds_ += nmfld; rgflds_ += rgfld;
	if ( idx )
	    nmfld->attach( alignedBelow, nmflds_[idx-1] );
	rgfld->attach( rightOf, nmfld );
    }

    uiLabeledSpinBox* lsb = new uiLabeledSpinBox( this,
	    nrdims_ == 1 ? "Number of bins" : "Number of bins per dimension");
    nrbinsfld_ = lsb->box();
    nrbinsfld_->setInterval( 3, 10000, 1 );
    nrbinsfld_->setValue( 25 );
    lsb->attach( alignedBelow, nmflds_[nmflds_.size()-1] );

    uiGroup* alfld = lsb;
    if ( isgauss )
    {
	for ( int idx=0; idx<nrdims_; idx++ )
	{
	    BufferString lbltxt( nrdims_ == 1 ? "Exp/Std" : "Dimension " );
	    if ( nrdims_ > 1 )
		lbltxt.add( idx+1 ).add( ": Exp/Std" );
	    uiGenInput* expstdfld = new uiGenInput( this, lbltxt,
				    FloatInpSpec(), FloatInpSpec() );
	    expstdfld->attach( alignedBelow, alfld );
	    expstdflds_ += expstdfld;
	    alfld = expstdfld;
	}

#define	mMkCorrFld(s,pos,att) \
	fld = new uiGenInput( this, s, FloatInpSpec(0) ); \
	fld->setElemSzPol( uiObject::Small ); \
	fld->attach( pos, att ); \
	ccflds_ += fld;
	if ( nrdims_ > 1 )
	{
	    uiGenInput* fld;
	    mMkCorrFld( nrdims_ == 2 ? "Correlation" : "Correlation 1 -> 2",
			rightOf, expstdflds_[1] );
	    if ( nrdims_ > 2 )
	    {
		mMkCorrFld( "Correlation 1 -> 3", alignedBelow, ccflds_[0] );
		mMkCorrFld( "Correlation 2 -> 3", rightOf, ccflds_[1] );
	    }
	}
    }

    uiSeparator* sep = new uiSeparator( this );
    sep->attach( stretchedBelow, alfld );

    IOObjContext ctxt( ProbDenFuncTranslatorGroup::ioContext() );
    ctxt.forread = false;
    outfld_ = new uiIOObjSel( this, ctxt );
    outfld_->attach( alignedBelow, alfld );
    outfld_->attach( ensureBelow, sep );
}


void uiProbDenFuncGenSampled::rgChg( CallBacker* cb )
{
    mDynamicCastGet(uiGenInput*,ginp,cb)
    if ( !ginp )
	{ pErrMsg("Huh? ginp null"); return; }
    const int dimidx = rgflds_.indexOf( ginp );
    if ( dimidx < 0 )
	{ pErrMsg("Huh? dimidx<0"); return; }
    uiGenInput* stdfld = expstdflds_[dimidx];
    if ( !stdfld->isUndef(0) )
	return;

    const Interval<float> rg( ginp->getFInterval() );
    if ( mIsUdf(rg.start) || mIsUdf(rg.stop) )
	return;

    stdfld->setValue( rg.center(), 0 );
}


#define mErrRet(s) { uiMSG().error(s); return false; }

#define mGetCCValue(ifld) \
{ \
    float cc = ccflds_[ifld]->getfValue(); \
    if ( mIsUdf(cc) ) cc = 0; \
    if ( cc < -cMaxGaussianCC() || cc > cMaxGaussianCC() ) \
	mErrRet( sGaussianCCRangeErrMsg() ) \
    ccs_ += cc; \
}


bool uiProbDenFuncGenSampled::getFromScreen()
{
    nrbins_= nrbinsfld_->getValue();
    od_int64 totalbins = nrbins_;
    totalbins = Math::IntPowerOf( totalbins, nrdims_ );
    if ( totalbins > 100000
      && !uiMSG().askGoOn(BufferString("You have requested a total of ",
	      totalbins, " bins.\nAre you sure this is what you want?")) )
	return false;

    dimnms_.setEmpty(); rgs_.setEmpty();
    exps_.setEmpty(); stds_.setEmpty(); ccs_.setEmpty();
    for ( int idim=0; idim<nrdims_; idim++ )
    {
	dimnms_.add( nmflds_[idim]->text() );
	if ( dimnms_.get(idim).isEmpty() )
	    mErrRet( "Please enter a name for each variable" )

	float exp = expstdflds_[idim]->getfValue(0);
	float stdev = expstdflds_[idim]->getfValue(1);
	if ( mIsUdf(exp) || mIsUdf(stdev) )
	    mErrRet( "Please fill all expectations and standard deviations" )
	exps_ += exp; stds_ += stdev;

	Interval<float> rg;
	rg.start = rgflds_[idim]->getfValue(0);
	rg.stop = rgflds_[idim]->getfValue(1);
	if ( mIsUdf(rg.start) || mIsUdf(rg.stop) )
	    mErrRet( "Please fill all variable ranges" )
	rg.sort();
	rgs_ += rg;

	if ( idim == 1 )
	    mGetCCValue( 0 )
	if ( idim == 2 )
	    { mGetCCValue( 1 ) mGetCCValue( 2 ) }
    }

    const IOObj* pdfioobj = outfld_->ioobj();
    if ( !outfld_->ioobj() )
	return false;

    ioobjky_ = pdfioobj->key();
    return true;
}


// Note that a Sampled PDF's SD starts at the center of a bin
// Thus, to get user's range, we need to start and stop half a step inward
#define mSetSD( sd, rg ) \
	spdf->sd.step = (rg.stop-rg.start) / nrbins_; \
	spdf->sd.start = rg.start + spdf->sd.step / 2


ProbDenFunc* uiProbDenFuncGenSampled::getPDF() const
{
    Stats::CalcSetup csu; csu.require( Stats::Max );
    Stats::RunCalc<float> rc( csu );
    ProbDenFunc* ret = 0;
    if ( nrdims_ == 1 )
    {
	Gaussian1DProbDenFunc gpdf( exps_[0], stds_[0] );
	Sampled1DProbDenFunc* spdf = new Sampled1DProbDenFunc;
	spdf->bins_.setSize( nrbins_ );
	mSetSD( sd_, rgs_[0] );
	for ( int idx=0; idx<nrbins_; idx++ )
	{
	    const float pos = spdf->sd_.atIndex( idx );
	    const float val = gpdf.value( pos );
	    spdf->bins_.set( idx, val );
	    rc.addValue( val );
	}
	spdf->setDimName( 0, dimnms_.get(0) );
	ret = spdf;
    }
    else if ( nrdims_ == 2 )
    {
	Gaussian2DProbDenFunc gpdf;
	gpdf.exp0_ = exps_[0]; gpdf.std0_ = stds_[0];
	gpdf.exp1_ = exps_[1]; gpdf.std1_ = stds_[1];
	gpdf.cc_ = ccs_[0];

	Sampled2DProbDenFunc* spdf = new Sampled2DProbDenFunc;
	spdf->bins_.setSize( nrbins_, nrbins_ );
	mSetSD( sd0_, rgs_[0] );
	mSetSD( sd1_, rgs_[1] );
	for ( int i0=0; i0<nrbins_; i0++ )
	{
	    const float p0 = spdf->sd0_.atIndex( i0 );
	    for ( int i1=0; i1<nrbins_; i1++ )
	    {
		const float p1 = spdf->sd1_.atIndex( i1 );
		const float val = gpdf.value( p0, p1 );
		spdf->bins_.set( i0, i1, val );
		rc.addValue( val );
	    }
	}
	spdf->setDimName( 0, dimnms_.get(0) );
	spdf->setDimName( 1, dimnms_.get(1) );
	ret = spdf;
    }
    else
    {
	GaussianNDProbDenFunc gpdf( 3 );
	for ( int idim=0; idim<nrdims_; idim++ )
	    gpdf.vars_[idim] = GaussianNDProbDenFunc::VarDef( dimnms_.get(idim),
						exps_[idim], stds_[idim] );
	gpdf.corrs_ += GaussianNDProbDenFunc::Corr( 0, 1, ccs_[0] );
	gpdf.corrs_ += GaussianNDProbDenFunc::Corr( 0, 2, ccs_[1] );
	gpdf.corrs_ += GaussianNDProbDenFunc::Corr( 1, 2, ccs_[2] );

	SampledNDProbDenFunc* spdf = new SampledNDProbDenFunc( 3 );
	const TypeSet<int> szs( 3, nrbins_ );
	spdf->bins_.setSize( szs.arr() );
	mSetSD( sds_[0], rgs_[0] );
	mSetSD( sds_[1], rgs_[1] );
	mSetSD( sds_[2], rgs_[2] );

	TypeSet<float> poss( 3, 0.0f );
	TypeSet<int> idxs( 3, 0 );
	for ( idxs[0]=0; idxs[0]<nrbins_; idxs[0]++ )
	{
	    poss[0] = spdf->sds_[0].atIndex( idxs[0] );
	    for ( idxs[1]=0; idxs[1]<nrbins_; idxs[1]++ )
	    {
		poss[1] = spdf->sds_[1].atIndex( idxs[1] );
		for ( idxs[2]=0; idxs[2]<nrbins_; idxs[2]++ )
		{
		    poss[2] = spdf->sds_[2].atIndex( idxs[2] );
		    const float val = gpdf.value( poss );
		    spdf->bins_.setND( idxs.arr(), val );
		    rc.addValue( val );
		}
	    }
	}

	spdf->dimnms_ = dimnms_;
	ret = spdf;
    }

    const float maxval = rc.max();
    if ( maxval )
	ret->scale( cMaxSampledBinVal / maxval );

    return ret;
}


bool uiProbDenFuncGenSampled::acceptOK( CallBacker* )
{
    if ( !getFromScreen() )
	return false;

    PtrMan<ProbDenFunc> pdf = getPDF();
    if ( !pdf )
	return false;

    return writePDF( *pdf, *outfld_->ioobj() );
}



uiProbDenFuncGenGaussian::uiProbDenFuncGenGaussian( uiParent* p, int nrdim,
						    MultiID& ky )
    : uiDialog(p,Setup("Generate Gaussian PDF",mNoDlgTitle,mTODOHelpKey))
    , ioobjky_(ky)
{
    if ( nrdim == 1 )
	pdf_ = new Gaussian1DProbDenFunc;
    else if ( nrdim == 2 )
	pdf_ = new Gaussian2DProbDenFunc;
    else
	pdf_ = new GaussianNDProbDenFunc( nrdim );
    pdffld_ = new uiEditGaussianProbDenFunc( this, *pdf_, true, true );

    uiSeparator* sep = 0;
    if ( nrdim < 3 )
    {
	sep = new uiSeparator( this );
	sep->attach( stretchedBelow, pdffld_ );
    }

    IOObjContext ctxt( ProbDenFuncTranslatorGroup::ioContext() );
    ctxt.forread = false;
    outfld_ = new uiIOObjSel( this, ctxt );
    if ( !sep )
	outfld_->attach( ensureBelow, pdffld_ );
    else
    {
	outfld_->attach( alignedBelow, pdffld_ );
	outfld_->attach( ensureBelow, sep );
    }
}


uiProbDenFuncGenGaussian::~uiProbDenFuncGenGaussian()
{
    delete pdf_;
}


bool uiProbDenFuncGenGaussian::acceptOK( CallBacker* )
{
    const IOObj* pdfioobj = outfld_->ioobj();
    if ( !pdfioobj || !pdffld_->commitChanges() )
	return false;

    ioobjky_ = pdfioobj->key();
    return writePDF( *pdf_, *outfld_->ioobj() );
}
