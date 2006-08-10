#ifndef horizonscanner_h
#define horizonscanner_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	N. Hemstra
 Date:		Feb 2004
 RCS:		$Id: horizonscanner.h,v 1.6 2006-08-10 13:51:37 cvsnanne Exp $
________________________________________________________________________

-*/

#include "executor.h"
#include "bufstringset.h"
#include "ranges.h"

class IOPar;
class PosGeomDetector;

class HorizonScanner : public Executor
{
public:

			HorizonScanner(const char* fnm);
			HorizonScanner(const BufferStringSet& fnms);
			~HorizonScanner();

    const char*		message() const;
    int			totalNr() const;
    int			nrDone() const;
    const char*		nrDoneText() const;

    void		setUndefValue(float udf)	{ udfval = udf; }
    void		setPosIsXY(bool yn)		{ isxy = yn; }
    bool		posIsXY() const			{ return isxy; }
    bool		needZScaling() const		{ return doscale; }
    bool		analyzeData();

    StepInterval<int>	inlRg() const;
    StepInterval<int>	crlRg() const;
    bool		gapsFound(bool inl) const;
    int			nrAttribValues() const;

    static const char*	defaultUserInfoFile();
    void		launchBrowser(const char* fnm=0) const;
    void		report(IOPar&) const;

protected:

    int			nextStep();

    void		init();

    int			chnksz;
    mutable int		totalnr;
    PosGeomDetector&	geomdetector;
    BufferStringSet	filenames;
    BufferStringSet	rejectedlines;

    bool		firsttime;
    bool		isxy;
    bool		doscale;
    float		udfval;
    int			nrattribvals;
    TypeSet<Interval<float> > valranges;

};


#endif
