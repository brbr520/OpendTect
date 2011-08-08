/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bruno
 Date:          April 2011
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uielasticpropsel.cc,v 1.2 2011-08-08 13:59:22 cvsbruno Exp $";

#include "uielasticpropsel.h"

#include "uibutton.h"
#include "uicombobox.h"
#include "uigeninput.h"
#include "uimsg.h"
#include "uiioobjsel.h"
#include "uiioobjmanip.h"
#include "uitabstack.h"
#include "uitoolbutton.h"

#include "elasticpropseltransl.h"
#include "mathexpression.h"
#include "propertyref.h"
#include "strmprov.h"

uiElasticPropSelGrp::uiSelInpGrp::uiSelInpGrp( uiParent* p, 
				const BufferStringSet& ppnms, int idx ) 
    : uiGroup( p, "Inp data group" )
    , idx_(idx)
    , propnms_(ppnms)
    , isactive_(false)		     
{
    varnmfld_ = new uiGenInput( this, "For" );
    varnmfld_->setElemSzPol( uiObject::Small );

    uiLabeledComboBox* lbl = new uiLabeledComboBox( this, "use");
    lbl->attach( rightOf, varnmfld_ );

    inpfld_ = lbl->box();
    inpfld_->addItems( ppnms );
    inpfld_->addItem( "Constant" );
    inpfld_->selectionChanged.notify( 
		    mCB(this,uiElasticPropSelGrp::uiSelInpGrp,selVarCB ) );

    ctefld_ = new uiGenInput( this, "Value", IntInpSpec() );
    ctefld_->attach( rightOf, lbl );
    ctefld_->setElemSzPol( uiObject::Small );

    display( false );

    setHAlignObj( lbl );
    selVarCB(0);
}


void uiElasticPropSelGrp::uiSelInpGrp::selVarCB( CallBacker* )
{
    const int selidx = inpfld_->currentItem();
    isconstant_ = selidx == inpfld_->size() -1;
    ctefld_->display( isconstant_ );
}


const char* uiElasticPropSelGrp::uiSelInpGrp::textOfVariable() const
{
    return isactive_ ? isconstant_ ? toString( ctefld_->getfValue() ) 
				   : inpfld_->text() 
		     : 0; 
}


void uiElasticPropSelGrp::uiSelInpGrp::setVariable( const char* txt, float val )
{
    isactive_ = txt;
    isconstant_ = !mIsUdf( val );
    ctefld_->display( isconstant_ );
    if ( isconstant_ )
	ctefld_->setValue( val );

    inpfld_->setCurrentItem( txt ); 
}


void uiElasticPropSelGrp::uiSelInpGrp::use( MathExpression* expr )
{
    const int nrvars = expr ? expr->nrUniqueVarNames() : 0;
    isactive_ =  idx_ < nrvars;
    display( isactive_ );

    if ( !isactive_ ) 
	return;

    const BufferString varnm = expr->uniqueVarName( idx_ );
    varnmfld_->setText( varnm );
    const int nearidx = propnms_.nearestMatch( varnm );
    if ( nearidx >= 0 )
	inpfld_->setCurrentItem( nearidx );
}



uiElasticPropSelGrp::uiElasticPropSelGrp( uiParent* p,
					const PropertyRefSelection& prs,
       					ElasticFormula& elformsel,
       					const TypeSet<ElasticFormula>& el )
    : uiGroup( p, "Prop Grp" )
    , proprefsel_(prs)  
    , elformsel_(elformsel)
    , availableformulas_(el)
    , expr_(0) 
{
    for ( int idx=0; idx<prs.size(); idx++ )
    {
	const PropertyRef* ref = prs[idx];
	propnms_.addIfNew( ref->name() );
    }
    if ( propnms_.isEmpty() )
	return;

    BufferStringSet predeftitles;
    for ( int idx=0; idx<availableformulas_.size(); idx++ )
	predeftitles.add( availableformulas_[idx].name() );

    const CallBack selcb( mCB(this,uiElasticPropSelGrp,selChgCB) );
    selmathfld_ = new uiLabeledComboBox( this, "Compute from" );
    selmathfld_->box()->addItem( "Defined quantity" );
    selmathfld_->box()->addItems( predeftitles );
    selmathfld_->box()->addItem( "Formula" );
    selmathfld_->box()->selectionChanged.notify(selcb); 

    formfld_ = new uiGenInput( this, "Formula " );
    formfld_->attach( alignedBelow, selmathfld_ );

    singleinpfld_ = new uiLabeledComboBox( this, "use" );
    singleinpfld_->attach( alignedBelow, selmathfld_ );
    singleinpfld_->box()->addItems( propnms_ );

    for ( int idx=0; idx<propnms_.size(); idx++ )
    {
	inpgrps_ += new uiElasticPropSelGrp::uiSelInpGrp( this, propnms_, idx );
	if ( idx )
	    inpgrps_[idx]->attach( alignedBelow, inpgrps_[idx-1] );
	else
	    inpgrps_[idx]->attach( alignedBelow, formfld_ );
    }
    formfld_->valuechanged.notify( selcb );

    putToScreen();
}


void uiElasticPropSelGrp::selChgCB( CallBacker* )
{
    const int selidx = selmathfld_->box()->currentItem();
    const bool wantmathexpr = selidx > 0;

    elformsel_.setExpression( "" );

    if ( selidx == selmathfld_->box()->size() -1 )
    {
	BufferString* formulanm = new BufferString( "User defined formula" );
	elformsel_.setName( formulanm->buf() );
	elformsel_.setExpression( formfld_->text() );
    }
    else
    {
	const ElasticFormula* ef = availableformulas_.validIdx(selidx-1) ? 
					    &availableformulas_[selidx-1] : 0;
	formfld_->setText( ef ? ef->expression() : "" );
	BufferString* formulanm = new BufferString(selmathfld_->box()->text());
	elformsel_.setName( formulanm->buf() );
	if ( ef ) 
	    elformsel_.setExpression( ef->expression() );
    }

    putToScreen();
}


void uiElasticPropSelGrp::getMathExpr()
{
    delete expr_; expr_ = 0;
    const BufferString inp( formfld_->text() );
    if ( inp.isEmpty() ) return;

    MathExpressionParser mep( inp );
    expr_ = mep.parse();

    if ( !expr_ )
	uiMSG().warning( 
	BufferString("The provided expression cannot be used:\n",mep.errMsg()));
}


void uiElasticPropSelGrp::getFromScreen()
{
    elformsel_.variables().erase();
    elformsel_.setExpression( "" );

    if ( selmathfld_->box()->currentItem() == 0 )
	elformsel_.variables().add( singleinpfld_->box()->text() );
    else
    {
	elformsel_.setExpression( formfld_->text() );
	for ( int idx=0; idx<inpgrps_.size(); idx++ )
	{
	    const char* txt = inpgrps_[idx]->textOfVariable();
	    if ( txt ) 
		elformsel_.variables().add( txt ); 
	}
    }
}


void uiElasticPropSelGrp::putToScreen()
{
    const BufferString& expr = elformsel_.expression();
    const bool hasexpr = !expr.isEmpty() 
	|| selmathfld_->box()->currentItem() == selmathfld_->box()->size()-1;
    const BufferStringSet& selvariables = elformsel_.variables();

    selmathfld_->box()->setCurrentItem( elformsel_.name() );
    formfld_->setText( expr );

    getMathExpr();

    float val = mUdf(float);
    formfld_->setText( expr );
    for ( int idx=0; idx<inpgrps_.size(); idx++ )
    {
	const char* vartxt = elformsel_.parseVariable( idx, val );
	inpgrps_[idx]->setVariable( vartxt, val ); 
	inpgrps_[idx]->use( expr_ );
    }
    if ( !hasexpr )
    {
	const char* vartxt = elformsel_.parseVariable( 0, val );
	singleinpfld_->box()->setCurrentItem( vartxt );
    }

    formfld_->display( hasexpr );
    singleinpfld_->display( !hasexpr );
}



static const char** props = ElasticFormula::ElasticTypeNames();

uiElasticPropSelDlg::uiElasticPropSelDlg( uiParent* p, 
					const PropertyRefSelection& prs,
					const MultiID& elpropselid )
    : uiDialog(p,uiDialog::Setup("Synthetic layers property selection",
		"Select quantities to compute density, p-wave and s-wave"
		,mTODOHelpID))
    , ctio_(*mMkCtxtIOObj(ElasticPropSelection)) 
{
    ElasticPropSelection* eps = ElasticPropSelection::get( elpropselid );
    elpropsel_ = eps ? *eps : *(new ElasticPropSelection()); 
    if ( !eps ) 
	ElasticPropGuess( prs, elpropsel_ );
    delete eps;

    ts_ = new uiTabStack( this, "Property selection tab stack" );
    ObjectSet<uiGroup> tgs;
    for ( int idx=0; props[idx]; idx++ )
    {
	ElasticFormula::ElasticType tp; 
	ElasticFormula::parseEnumElasticType( props[idx], tp );
	tgs += new uiGroup( ts_->tabGroup(), props[idx]);
	TypeSet<ElasticFormula> formulas;
	ElFR().getByType( tp, formulas );
	propflds_ += new uiElasticPropSelGrp( tgs[idx], prs, 
					elpropsel_.getFormula( tp ), formulas );
	ts_->addTab( tgs[idx], props[idx] );
    }

    uiGroup* gengrp = new uiGroup( this, "buttons" );
    gengrp->attach( ensureBelow, ts_ );
    uiToolButton* opentb = new uiToolButton( gengrp, "open.png",
				"Open stored property selection",
				mCB(this,uiElasticPropSelDlg,openPropSelCB) );
    uiToolButton* stb = new uiToolButton( gengrp, "save.png",
				"Save property selection",
				mCB(this,uiElasticPropSelDlg,savePropSelCB) );
    stb->attach( rightOf, opentb );
}


uiElasticPropSelDlg::~uiElasticPropSelDlg()
{
   delete ctio_.ioobj; delete &ctio_; 
}


void uiElasticPropSelDlg::elasticPropSelectionChanged( CallBacker* )
{
    for ( int idx=0; idx<propflds_.size(); idx++ )
	propflds_[idx]->putToScreen();
}


#define mErrRet(s) { uiMSG().error(s); return false; }

bool uiElasticPropSelDlg::acceptOK( CallBacker* )
{
    if( !ctio_.ioobj )
	savePropSel(); 
    else 
	doSave( *ctio_.ioobj );

    return true;
}


bool uiElasticPropSelDlg::savePropSel() 
{
    for ( int idx=0; idx<propflds_.size(); idx++ )
	propflds_[idx]->getFromScreen();

    if ( !ctio_.ioobj )
    {
	ctio_.ctxt.forread = false;
	uiIOObjSelDlg dlg( this, ctio_ );
	if ( !dlg.go() || !dlg.ioObj() )
	    return false;
	ctio_.setObj( dlg.ioObj()->clone() );
    }
    if ( !ctio_.ioobj ) 
	return false;

    storedmid_ = ctio_.ioobj->key();

    return  doSave( *ctio_.ioobj );
}


bool uiElasticPropSelDlg::doSave( const IOObj& ioobj )
{
    const BufferString fnm( ioobj.fullUserExpr(false) );
    StreamData sd( StreamProvider(fnm).makeOStream() );
    bool rv = false;
    if ( !sd.usable() )
	uiMSG().error( "Cannot open output file" );
    else if ( !elpropsel_.put(&ioobj) )
	uiMSG().error( "Cann not write file" );
    else
	rv = true;

    sd.close();
    return rv;
}


bool uiElasticPropSelDlg::openPropSel()
{
    ctio_.ctxt.forread = true;
    uiIOObjSelDlg dlg( this, ctio_ );
    if ( !dlg.go() || !dlg.ioObj() )
	return false;

    ctio_.setObj( dlg.ioObj()->clone() );

    const BufferString fnm( ctio_.ioobj->fullUserExpr(true) );
    StreamData sd( StreamProvider(fnm).makeIStream() );
    if ( !sd.usable() )
	{ uiMSG().error( "Cannot open input file" ); return false; }

    ElasticPropSelection* elp = elpropsel_.get(ctio_.ioobj);
    sd.close();

    if ( elp )
    {
	elpropsel_ = *elp;
	delete elp;
	elasticPropSelectionChanged(0);
	return true;
    }
    else
	mErrRet( "Unable to write elastic property selection" );
    return false;
}

