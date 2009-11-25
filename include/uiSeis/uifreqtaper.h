#ifndef uiwindowfuncseldlg_h
#define uiwindowfuncseldlg_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bruno
 Date:		Nov 2009
 RCS:		$Id: uifreqtaper.h,v 1.2 2009-11-25 13:33:06 cvsbruno Exp $
________________________________________________________________________

-*/

#include "uidialog.h"
#include "uifunctiondisplay.h"
#include "uiwindowfunctionsel.h"
#include "uibutton.h"
#include "arrayndutils.h"
#include <arrayndimpl.h>

class uiGenInput;
class uiFuncTaperDisp;
class uiSliceSelDlg;
class uiSliderExtra;
class uiFreqTaperGrp;

class ArrayNDWindow;
class CubeSampling;


mStruct FreqTaperSetup
{
		    FreqTaperSetup()
			: hasmin_(false)
			, hasmax_(true)
			, seisnm_(0)		   
			, allfreqssetable_(false)
			{}

    const char* 	seisnm_;	
    bool 		hasmin_;	
    bool 		hasmax_;
    Interval<float> 	minfreqrg_;
    Interval<float>	maxfreqrg_;
    bool 		allfreqssetable_;	
};


mStruct TaperData
{
		    TaperData()
			: window_(0)  
			, paramval_(1)  
			{}

    Interval<float> 	rg_;
    Interval<float> 	refrg_;

    ArrayNDWindow*	window_;
    int 		winsz_;
    float		paramval_;
    float		slope_;
};


mClass uiFuncTaperDisp : public uiFunctionDisplay
{
public:

    mStruct Setup : public uiFunctionDisplay::Setup
    {
			Setup()
			    : is2sided_(false)
			    {}

	mDefSetupMemb(int,datasz);	
	mDefSetupMemb(const char*,xaxnm);	
	mDefSetupMemb(const char*,yaxnm);	
	mDefSetupMemb(Interval<float>,leftrg)	
	mDefSetupMemb(Interval<float>,rightrg)
	mDefSetupMemb(bool,is2sided);	
    };

			uiFuncTaperDisp(uiParent*,const Setup&);
			~uiFuncTaperDisp();
   

    Notifier<uiFuncTaperDisp> taperchanged;

    void 		setWindows(float,float rightvar=0);
    void		setFunction(Array1DImpl<float>&,Interval<float>);
    			
    float*		getWinValues() const 
			{ return window_ ? window_->getValues() : 0; } 
    float*		getFuncValues() const 
    			{ return funcvals_ ? funcvals_->getData() : 0; } 
   
    void		displayTaper( bool yn = true ) 
			{ displaytaper_ = yn; }
    void		taperChged(CallBacker*);

    TaperData&		leftTaperData() { return leftd_; }
    TaperData&		rightTaperData() { return rightd_; }

protected:

    TaperData		leftd_;
    TaperData		rightd_;

    ArrayNDWindow*	window_;

    Array1DImpl<float>* funcvals_; 
    Array1DImpl<float>* orgfuncvals_;
    Interval<float>	funcrg_;	

    bool		is2sided_;
    bool		displaytaper_;
    int 		datasz_;
};



mClass uiFreqTaperGrp : public uiGroup
{

public:
    
			uiFreqTaperGrp(uiParent*,
				       const FreqTaperSetup&,
				       uiFuncTaperDisp*);
			~uiFreqTaperGrp(){};
   

    void		setFreqRange(Interval<float>); 
    Interval<float>	getFreqRange() const; 
    void		taperChged(CallBacker*);

protected :
    
    TaperData		dd1_;
    TaperData		dd2_;

    uiGenInput*		varinpfld_;
    uiGenInput*		freqinpfld_;
    uiGenInput*		inffreqfld_;
    uiGenInput*		supfreqfld_;
    uiSliderExtra*	sliderfld_;
    uiFuncTaperDisp*    drawer_;
    
    bool		hasmin_;
    bool		hasmax_;
    bool		isminactive_;
    int 		datasz_;
    bool 		allfreqssetable_;	
    
    void		setSlopeFromFreq();
    void 		setPercentsFromFreq();
    void 		setFreqFromSlope(float);

    void		freqChoiceChged(CallBacker*);
    void 		freqChanged(CallBacker*);
    void 		putToScreen(CallBacker*);
    void 		sliderChanged(CallBacker*);
    void		slopeChanged(CallBacker*);
};



mClass uiFreqTaperDlg : public uiDialog
{
public:

			uiFreqTaperDlg(uiParent*,const FreqTaperSetup&);
			~uiFreqTaperDlg();
    
    Interval<float>	getFreqRange() const 
    			{ return tapergrp_->getFreqRange(); }

protected:


    uiFreqTaperGrp*	tapergrp_;
    uiFuncTaperDisp*    drawer_;
    Array1DImpl<float>* funcvals_; 

    bool		withpreview_;
    const char*		seisnm_;
    uiPushButton*	previewfld_;
    uiSliceSelDlg*	posdlg_;
    CubeSampling*	cs_;

    void		previewPushed(CallBacker*);
};



mClass uiFreqTaperSel : public uiWindowFunctionSel
{
public:
				uiFreqTaperSel(uiParent*,const Setup&);

    void                        setIsMinMaxFreq(bool,bool);
    void                        setInputFreqValue(float,int);
    Interval<float>             freqValues() const;
    void                        setRefFreqs(Interval<float>);

protected :

    Interval<float>             freqrg_;
    Interval<float>             selfreqrg_;
    bool                        isminfreq_;
    bool                        ismaxfreq_;
    uiFreqTaperDlg*             freqtaperdlg_;
    const char*                 seisnm_;

    void                        winfuncseldlgCB(CallBacker*);
    void                        windowClosed(CallBacker*);
    void                        setSelFreqs(CallBacker*);
};

#endif
