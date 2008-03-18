#ifndef uilatlong2coord_h
#define uilatlong2coord_h
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert
 Date:          Mar 2008
 RCS:           $Id: uilatlong2coord.h,v 1.1 2008-03-18 15:39:54 cvsbert Exp $
________________________________________________________________________

-*/

#include "uidialog.h"
class LatLong2Coord;
class SurveyInfo;
class uiGenInput;
class uiCheckBox;

class uiLatLong2CoordDlg : public uiDialog
{
public:
			uiLatLong2CoordDlg(uiParent*,const LatLong2Coord&,
					   const SurveyInfo* si=0);
			~uiLatLong2CoordDlg();

    const LatLong2Coord& ll2C() const		{ return ll2c_; }

protected:

    LatLong2Coord&	ll2c_;
    const SurveyInfo*	si_;

    uiGenInput*		coordfld_;
    uiGenInput*		latlongfld_;
    uiCheckBox*		isftfld_;

    bool		acceptOK(CallBacker*);
};


#endif
