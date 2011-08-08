#ifndef elasticpropsel_h
#define elasticpropsel_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bruno
 Date:		May 2011
 RCS:		$Id: elasticpropsel.h,v 1.8 2011-08-08 13:59:22 cvsbruno Exp $
________________________________________________________________________

-*/

/*! brief assigns values to an elastic layer depending on user defined parameters !*/

#include "ailayer.h"
#include "elasticprop.h"
#include "propertyref.h"

class IOObj;
class MultiID;

mClass ElasticPropSelection : public NamedObject
{
public:
				ElasticPropSelection(const char* nm=0);
				ElasticPropSelection(
					const ElasticPropSelection& elp)
				{ *this = elp; }

    ElasticPropSelection&     	operator =(const ElasticPropSelection&);
    inline bool         	operator ==(const ElasticPropSelection& e) const
   				{ return name() == e.name(); }
    inline bool         	operator !=(const ElasticPropSelection& e) const
    				{ return name() != e.name(); }


    ElasticFormula&		getFormula(ElasticFormula::ElasticType);
    const ElasticFormula&	getFormula(ElasticFormula::ElasticType) const;

    static ElasticPropSelection* get(const MultiID&);
    static ElasticPropSelection* get(const IOObj*);
    bool                	put(const IOObj*) const;

    bool			isValidInput() const;

    void			fillPar(IOPar&) const;
    void			usePar(const IOPar&);

    const TypeSet<ElasticFormula>& getFormulas() const 
    				{ return selectedformulas_; }

protected :
    TypeSet<ElasticFormula> 	selectedformulas_;
};



mClass ElasticPropGen
{
public:
    			ElasticPropGen(const ElasticPropSelection& eps,
					const PropertyRefSelection& rps)
			    : elasticprops_(eps), refprops_(rps) {}

    void		fill(AILayer&,const float* proprefvals,int sz);
    void		fill(ElasticLayer&,const float* proprefvals,int sz);
protected:

    ElasticPropSelection elasticprops_;
    const PropertyRefSelection& refprops_;

    float		setVal(const ElasticFormula& ef,
	    			const float* proprefvals,int proprefsz); 
};



mClass ElasticPropGuess
{
public:
    			ElasticPropGuess(const PropertyRefSelection&,
						ElasticPropSelection&);
protected:
    void		guessQuantity(const PropertyRefSelection&,
				    ElasticFormula::ElasticType);

    PropertyRef::StdType elasticToStdType(ElasticFormula::ElasticType) const;

    ElasticPropSelection& elasticprops_; 
};


#endif

