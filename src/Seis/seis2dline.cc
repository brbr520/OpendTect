/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : June 2004
-*/

static const char* rcsID = "$Id: seis2dline.cc,v 1.8 2004-08-27 10:07:32 bert Exp $";

#include "seis2dline.h"
#include "seistrctr.h"
#include "seisbuf.h"
#include "survinfo.h"
#include "strmprov.h"
#include "ascstream.h"
#include "filegen.h"
#include "keystrs.h"
#include "iopar.h"
#include "ioobj.h"
#include "errh.h"

const char* Seis2DLineGroup::sKeyAttrib = "Attribute";
const char* Seis2DLineGroup::sKeyDefAttrib = "Seis";


ObjectSet<Seis2DLineIOProvider>& S2DLIOPs()
{
    static ObjectSet<Seis2DLineIOProvider>* theinst = 0;
    if ( !theinst ) theinst = new ObjectSet<Seis2DLineIOProvider>;
    return *theinst;
}


//------
// Translator mechanism is only used for selection etc.

class TwoDSeisTrcTranslator : public SeisTrcTranslator
{			isTranslator(TwoD,SeisTrc) public:
			TwoDSeisTrcTranslator( const char* s1, const char* s2 )
			: SeisTrcTranslator(s1,s2)	{}

    const char*		defExtension() const            { return "2ds"; }
    bool		implRemove(const IOObj*) const;
    bool		initRead_(); // supporting getRanges()
    bool		initWrite_(const SeisTrc&)	{ return false; }

};

defineTranslator(TwoD,SeisTrc,"2D");


bool TwoDSeisTrcTranslator::implRemove( const IOObj* ioobj ) const
{
    if ( !ioobj ) return true;
    BufferString fnm( ioobj->fullUserExpr(true) );
    Seis2DLineGroup lg( fnm );
    const int nrlines = lg.nrLines();
    for ( int iln=0; iln<nrlines; iln++ )
	lg.remove( 0 );
    File_remove( fnm, NO );
    return true;
}


bool TwoDSeisTrcTranslator::initRead_()
{
    if ( !conn->ioobj )
	{ errmsg = "Cannot reconstruct 2D filename"; return false; }
    BufferString fnm( conn->ioobj->fullUserExpr(true) );
    if ( !File_exists(fnm) ) return false;
    Seis2DLineGroup lg( fnm );
    lg.getTxtInfo( 0, pinfo->usrinfo, pinfo->stdinfo );
    const int nrlines = lg.nrLines();
    if ( nrlines < 1 )
	{ errmsg = "No lines"; return false; }

    StepInterval<int> trg; StepInterval<float> zrg;
    if ( !lg.getRanges(0,trg,zrg) )
	{ errmsg = "No range info"; return false; }
    StepInterval<int> newtrg; StepInterval<float> newzrg;
    for ( int iln=1; iln<nrlines; iln++ )
    {
	if ( lg.getRanges(iln,newtrg,newzrg) )
	{
	    if ( newtrg.stop > trg.stop ) trg.stop = newtrg.stop;
	    if ( newtrg.step < trg.step ) trg.step = newtrg.step;
	    if ( newzrg.start < zrg.start ) zrg.start = newzrg.start;
	    if ( newzrg.stop > zrg.stop ) zrg.stop = newzrg.stop;
	    if ( newzrg.step < zrg.step ) zrg.step = newzrg.step;
	}
    }

    insd.start = zrg.start;
    insd.step = zrg.step;
    innrsamples = (int)((zrg.stop-zrg.start) / zrg.step + 1.5);
    pinfo->inlrg.start = pinfo->crlrg.start = 0;
    pinfo->inlrg.stop = nrlines - 1; pinfo->crlrg.stop = trg.stop;
    pinfo->inlrg.step = 1; pinfo->crlrg.step = trg.step;
    return true;
}


//------


Seis2DLineGroup::~Seis2DLineGroup()
{
    deepErase( pars_ );
}


Seis2DLineGroup& Seis2DLineGroup::operator =( const Seis2DLineGroup& lg )
{
    if ( &lg == this ) return *this;
    fname_ = lg.fname_;
    readFile();
    return *this;
}


void Seis2DLineGroup::init( const char* fnm )
{
    fname_ = fnm;
    BufferString type = "CBVS";
    readFile( &type );

    liop_ = 0;
    const ObjectSet<Seis2DLineIOProvider>& liops = S2DLIOPs();
    for ( int idx=0; idx<liops.size(); idx++ )
    {
	Seis2DLineIOProvider* liop = liops[idx];
	if ( type == liop->type() )
	    { liop_ = liop; break; }
    }
}


const char* Seis2DLineGroup::type() const
{
    return liop_ ? liop_->type() : "CBVS";
}


const char* Seis2DLineGroup::lineName( int idx ) const
{
    return idx >= pars_.size() ? 0 : pars_[idx]->name().buf();
}


const char* Seis2DLineGroup::attribute( int idx ) const
{
    const char* res = 0;
    if ( idx < pars_.size() )
	res = pars_[idx]->find( sKeyAttrib );
    return res ? res : sKeyDefAttrib;
}


BufferString Seis2DLineGroup::lineKey( const char* lnm, const char* attrnm )
{
    BufferString ret( lnm );
    if ( attrnm && strcmp(attrnm,sKeyDefAttrib) )
	{ ret += "|"; ret += attrnm; }
    return ret;
}


BufferString Seis2DLineGroup::lineNamefromKey( const char* key )
{
    BufferString ret( key );
    char* ptr = strchr( ret.buf(), '|' );
    if ( ptr ) *ptr = '\0';
    return ret;
}


BufferString Seis2DLineGroup::attrNamefromKey( const char* key )
{
    BufferString ret;
    char* ptr = key ? strchr( key, '|' ) : 0;
    if ( ptr ) ret = ptr + 1;
    return ret;
}


static const char* sKeyFileType = "2D Line Group Data";

void Seis2DLineGroup::readFile( BufferString* type )
{
    deepErase( pars_ );

    StreamData sd = StreamProvider( fname_ ).makeIStream();
    if ( !sd.usable() ) return;

    ascistream astrm( *sd.istrm, true );
    if ( !astrm.isOfFileType( sKeyFileType ) )
	{ sd.close(); return; }

    while ( !atEndOfSection(astrm.next()) )
    {
	if ( astrm.hasKeyword(sKey::Name) )
	    setName( astrm.value() );
	if ( astrm.hasKeyword(sKey::Type) && type )
	    *type = astrm.value();
    }

    while ( astrm.type() != ascistream::EndOfFile )
    {
	IOPar* newpar = new IOPar;
	while ( !atEndOfSection(astrm.next()) )
	{
	    if ( astrm.hasKeyword(sKey::Name) )
		newpar->setName( astrm.value() );
	    else
		newpar->set( astrm.keyWord(), astrm.value() );
	}
	if ( newpar->size() < 1 )
	    delete newpar;
	else
	    pars_ += newpar;
    }

    sd.close();
}


void Seis2DLineGroup::writeFile() const
{
    BufferString wrfnm( fname_ ); wrfnm += "_new";

    StreamData sd = StreamProvider( wrfnm ).makeOStream();
    if ( !sd.usable() ) return;

    ascostream astrm( *sd.ostrm );
    if ( !astrm.putHeader( sKeyFileType ) )
	{ sd.close(); return; }

    astrm.put( sKey::Name, name() );
    astrm.put( sKey::Type, type() );
    astrm.put( "Number of lines", pars_.size() );
    astrm.newParagraph();

    for ( int ipar=0; ipar<pars_.size(); ipar++ )
    {
	const IOPar& iopar = *pars_[ipar];
	astrm.put( sKey::Name, iopar.name() );
	for ( int idx=0; idx<iopar.size(); idx++ )
	    astrm.put( iopar.getKey(idx), iopar.getValue(idx) );
	astrm.newParagraph();
    }

    sd.close();

    if ( File_exists(fname_) )
	File_remove( fname_, 0 );
    File_rename( wrfnm, fname_ );
}


Executor* Seis2DLineGroup::lineFetcher( int ipar, SeisTrcBuf& tbuf,
					const SeisSelData* sd) const
{
    if ( !liop_ )
    {
	ErrMsg("No suitable 2D line extraction object found");
	return 0;
    }

    return liop_->getFetcher( *pars_[ipar], tbuf, sd );
}


Executor* Seis2DLineGroup::lineAdder( IOPar* newiop,
				      const SeisTrcBuf& tbuf ) const
{
    if ( !newiop || !newiop->size() || !tbuf.size() )
    {
	ErrMsg("No data for line add provided");
	return 0;
    }
    if ( !liop_ )
    {
	ErrMsg("No suitable 2D line creation object found");
	return 0;
    }

    const IOPar* previop = pars_.size() ? pars_[pars_.size()-1] : 0;
    return liop_->getPutter( *newiop, tbuf, previop );
}


void Seis2DLineGroup::commitAdd( IOPar* newiop )
{
    pars_ += newiop;
    writeFile();
}


bool Seis2DLineGroup::isEmpty( int ipar ) const
{
    return liop_ ? liop_->isEmpty( *pars_[ipar] ) : true;
}


void Seis2DLineGroup::remove( int ipar )
{
    if ( ipar > pars_.size() ) return;
    IOPar* iop = pars_[ipar];
    if ( liop_ )
	liop_->removeImpl(*iop);

    pars_ -= iop;
    delete iop;
    writeFile();
}


bool Seis2DLineGroup::getTxtInfo( int ipar, BufferString& uinf,
				  BufferString& stdinf ) const
{
    return liop_ ? liop_->getTxtInfo(*pars_[ipar],uinf,stdinf) : false;
}


bool Seis2DLineGroup::getRanges( int ipar, StepInterval<int>& sii,
				 StepInterval<float>& sif ) const
{
    return liop_ ? liop_->getRanges(*pars_[ipar],sii,sif) : false;
}
