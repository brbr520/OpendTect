#ifndef uigraphicsviewbase_h
#define uigraphicsviewbase_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Satyaki Maitra
 Date:		March 2009
 RCS:		$Id: uigraphicsviewbase.h,v 1.15 2010-09-13 10:48:27 cvsbruno Exp $
________________________________________________________________________

-*/

#include "uiobj.h"

class Color;
class uiGraphicsItem;
class uiGraphicsScene;
class uiGraphicsViewBody;
class KeyboardEventHandler;
class MouseEventHandler;
class uiRect;


mClass uiGraphicsViewBase : public uiObject
{
friend class uiGraphicsViewBody;
public:
				uiGraphicsViewBase(uiParent*,const char*);
				~uiGraphicsViewBase();

    void			setScene(uiGraphicsScene&);
    				//!<Scene becomes mine
    uiGraphicsScene&		scene();
    void			show();
    enum ODDragMode		{ NoDrag, ScrollHandDrag, RubberBandDrag };
    enum ScrollBarPolicy	{ ScrollBarAsNeeded, ScrollBarAlwaysOff,
				  ScrollBarAlwaysOn };

    void			setScrollBarPolicy(bool hor,ScrollBarPolicy); 
    void			setDragMode(ODDragMode); 
    ODDragMode			dragMode() const;
    bool			isRubberBandingOn() const;

    void			setMouseTracking(bool);
    bool			hasMouseTracking() const;

    int				width() const; 
    int				height() const; 

    void			centreOn(uiPoint);
    uiRect			getSceneRect() const;
    void			setSceneRect(const uiRect&);

    uiPoint			getCursorPos() const; 
    uiPoint			getScenePos(float,float) const; 
    const uiPoint& 		getStartPos() const;
    const uiRect*		getSelectedArea() const	{return selectedarea_;}
    void			setViewArea(double x,double y,
	    				    double w,double h);
    uiRect			getViewArea() const;

    void                        setBackgroundColor(const Color&);
    Color		        backgroundColor() const;
    void                        uisetBackgroundColor(const Color&);
    Color		        uibackgroundColor() const;
    void			setNoBackGround();
    void			rePaintRect(const uiRect*); 
    void			enableScrollZoom()  { enabscrollzoom_ = true; }
    void			disableScrollZoom() { enabscrollzoom_ = false; }
    bool			scrollZoomEnabled()
    				{ return enabscrollzoom_; }
    
    bool			isCtrlPressed() const	{return isctrlpressed_;}
    void			setCtrlPressed( bool yn )
    				{ isctrlpressed_ = yn; }

    MouseEventHandler&		getNavigationMouseEventHandler();
    				/*!<This eventhandler handles events related to
				    navigation (rubberbands & panning). For general
				    calls, use getMouseEventHandler(). */
    MouseEventHandler&		getMouseEventHandler();
    KeyboardEventHandler&	getKeyboardEventHandler();


    CNotifier<uiGraphicsViewBase,uiSize> reSize; //!< CallBacker is OLD size
    Notifier<uiGraphicsViewBase> rubberBandUsed;
    Notifier<uiGraphicsViewBase> reDrawNeeded;
    Notifier<uiGraphicsViewBase> reDrawn;
    				//!< In practice, this happens only after reSize


protected:

    uiGraphicsViewBody*		body_;
    uiGraphicsViewBody&		mkbody(uiParent*,const char*);

    uiRect*			selectedarea_;
    uiGraphicsScene*		scene_;

    bool			isctrlpressed_;
    bool			enabscrollzoom_;
    bool			enabbgzoom_;
    void 			rubberBandCB(CallBacker*);

};

#endif
