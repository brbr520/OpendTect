#ifndef energyattrib_h
#define energyattrib_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Kristofer Tingdahl
 Date:          07-10-1999
 RCS:           $Id: energyattrib.h,v 1.6 2005-12-13 10:03:41 cvshelene Exp $
________________________________________________________________________

-*/


#include "attribprovider.h"


/*!\brief Energy Attribute

Energy gate=

Calculates the squared sum of the gate's samples divided by the number of
samples in the gate.

Input:
0		Data

Outputs:
0		The energy
*/
    

namespace Attrib
{

class Energy: public Provider
{
public:
    static void		initClass();
			Energy(Desc&);

    static const char*  attribName()		{ return "Energy"; }
    static const char*  gateStr()		{ return "gate"; }

protected:
    static Provider*    createInstance(Desc&);

    bool		getInputOutput(int input,TypeSet<int>& res) const;
    bool		getInputData(const BinID&, int idx);
    bool		computeData(const DataHolder&,const BinID& relpos,
				    int t0,int nrsamples) const;

    const Interval<float>* reqZMargin(int input,int output) const
    			   { return &gate; }
    
    Interval<float>	gate;
    int			dataidx_;
    ObjectSet<const DataHolder>	inputdata;
};

}; // namespace Attrib


/*!\mainpage Standard Attributes

  This module contains the definition of the 'standard' OpendTect attributes.
  Contained are attributes like Energy, Similarity, Volume Statistics, etc.
  The base class for all attributes is the AttribCalc class.

  The Attribute factories are defined in the Attribute Engine module
  (AttribEng).

  If you want to make your own attributes, please consult the Programmer's
  manual, section 'Plugins'. You'll find an annotated example of the Coherency
  attribute implementation.

*/


#endif
