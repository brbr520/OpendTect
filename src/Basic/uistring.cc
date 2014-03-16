/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		Jan 2014
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "uistring.h"

#include "bufstring.h"
#include "refcount.h"
#include "ptrman.h"
#include "typeset.h"
#include "perthreadrepos.h"
#include "texttranslator.h"

#include <QString>
#include <QTranslator>


class uiStringData
{ mRefCountImplNoDestructor(uiStringData)
public:
    uiStringData( const char* originalstring, const char* context,
		  const char* application,
		  const char* disambiguation, int pluralnr )
	: originalstring_( originalstring )
	, translationcontext_( context )
	, translationpluralnumber_( pluralnr )
	, translationdisambiguation_( disambiguation )
	, application_( application )
    {
    }

    void setFrom( const uiStringData& d )
    {
	originalstring_ = d.originalstring_;
	translationcontext_ = d.translationcontext_;
	translationpluralnumber_ = d.translationpluralnumber_;
	translationdisambiguation_ = d.translationdisambiguation_;
	arguments_ = d.arguments_;
	application_ = d.application_;
    }

    void setFrom( const QString& qstr )
    {
	originalstring_.setEmpty();
	translationcontext_ = 0;
	translationpluralnumber_ = -1;
	translationdisambiguation_ = 0;
	application_ = 0;
	qstring_ = qstr;
    }

    const char* getFullString() const;

    void set(const char* orig);
    void fillQString(QString&,const QTranslator* translator=0) const;

    TypeSet<uiString>		arguments_;

    QString			qstring_;

    BufferString		originalstring_;
    const char* 		translationcontext_;
    const char* 		application_;
    const char* 		translationdisambiguation_;
    int 			translationpluralnumber_;

    int 			dirtycount_;
};


void uiStringData::set( const char* orig )
{
    originalstring_ = orig;
}


const char* uiStringData::getFullString() const
{
    if ( !arguments_.size() )
	return originalstring_.buf();

    QString qres;
    fillQString( qres, 0 );

    mDeclStaticString( ret );
    ret = qres;
    return ret.buf();
}



void uiStringData::fillQString( QString& res,
				const QTranslator* translator ) const
{
    if ( !originalstring_ || !*originalstring_ )
	return;

    if ( translator )
	res = translator->translate( translationcontext_, originalstring_,
				     translationdisambiguation_,
				     translationpluralnumber_ );
    else
	res = originalstring_;

    for ( int idx=0; idx<arguments_.size(); idx++ )
    {
	res = res.arg( arguments_[idx].getQtString() );
    }
}



uiString::uiString( const char* str )
    : data_( new uiStringData( 0, 0, 0, 0, -1 ) )
{
    data_->ref();
    *this = str;
}


uiString::uiString( const char* originaltext, const char* context,
		    const char* application,
		    const char* disambiguation, int pluralnr )
    : data_( new uiStringData(originaltext, context, application,
			      disambiguation, pluralnr ))
{
    data_->ref();
}


uiString::uiString( const uiString& str )
    : data_( str.data_ )
{
    data_->ref();

    *this = str;
}


uiString::uiString( const FixedString& str )
    : data_( new uiStringData( 0, 0, 0, 0, -1 ) )
{
    data_->ref();
    *this = str;
}


uiString::uiString( const BufferString& str )
    : data_( new uiStringData( 0, 0, 0, 0, -1 ) )
{
    data_->ref();
    *this = str;
}


uiString::~uiString()
{
    data_->unRef();
}


bool uiString::isEmpty() const
{
    return data_->originalstring_.isEmpty();
}


const char* uiString::getOriginalString() const
{
    return data_->originalstring_;
}


const char* uiString::getFullString() const
{
    return data_->getFullString();
}


const QString& uiString::getQtString() const
{
    data_->fillQString( data_->qstring_,
			TrMgr().getQTranslator(data_->application_) );
    return data_->qstring_;
}


uiString& uiString::operator=( const uiString& str )
{
    str.data_->ref();
    data_->unRef();
    data_ = str.data_;
    return *this;
}


uiString& uiString::setFrom( const uiString& str )
{
    if ( data_==str.data_ )
    {
	data_->unRef();
	data_ = new uiStringData( 0, 0, 0, 0, -1 );
	data_->ref();
    }

    data_->setFrom( *str.data_ );
    return *this;
}


void uiString::setFrom( const QString& qstr )
{
    data_->setFrom( qstr );
}


uiString& uiString::operator=( const FixedString& str )
{
    data_->set( str.str() );
    return *this;
}


uiString& uiString::operator=( const BufferString& str )
{
    data_->set( str.str() );

    return *this;
}


uiString& uiString::operator=( const char* str )
{
    data_->set( str );
    return *this;
}


uiString& uiString::arg( const uiString& newarg )
{
    data_->arguments_ += newarg;
    return *this;;
}


uiString& uiString::arg( const FixedString& a )
{ return arg( a.str() ); }


uiString& uiString::arg( const BufferString& a )
{ return arg( a.str() ); }


uiString& uiString::arg( const char* newarg )
{
    return arg( uiString(newarg) );
}


void uiString::translate( const QTranslator& tr , QString& res ) const
{
    data_->fillQString( res, &tr );

}


