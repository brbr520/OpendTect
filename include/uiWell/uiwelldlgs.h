#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          October 2003
________________________________________________________________________

-*/

#include "uiwellmod.h"
#include "uiselsimple.h"
#include "dbkey.h"
#include "ranges.h"
#include "position.h"
#include "uistring.h"

class uiButtonGroup;
class uiCheckBox;
class uiD2TModelGroup;
class uiGenInput;
class uiLabel;
class uiMultiWellSel;
class uiPushButton;
class uiTable;
class uiTableImpDataSel;
class uiUnitSel;
class uiWellSel;
class BufferStringSet;

namespace Table { class FormatDesc; }
namespace Well { class Data; class Track; class D2TModel; class Log;
		 class LogSet;}



/*! \brief Dialog for Well track editing. */

mExpClass(uiWell) uiWellTrackDlg : public uiDialog
{ mODTextTranslationClass(uiWellTrackDlg);
public:
			uiWellTrackDlg(uiParent*,Well::Data&);
			~uiWellTrackDlg();

    static const uiString	sCkShotData();
    static const uiString	sTimeDepthModel();

protected:

    Well::Data&		wd_;
    Well::Track&	track_;
    Well::Track*	orgtrack_;
    bool		writable_;

    Table::FormatDesc&	fd_;
    uiTable*		tbl_;
    uiGenInput*		uwifld_;
    uiGenInput*		glfld_;
    uiCheckBox*		zinftfld_;
    uiGenInput*		wellheadxfld_;
    uiGenInput*		wellheadyfld_;
    uiGenInput*		kbelevfld_;

    Coord3		origpos_;
    float		origgl_;

    void		fillTableCB(CallBacker*);
    bool		fillTable();
    void		fillSetFields(CallBacker* cb=0);
    void		updNowCB(CallBacker*);
    bool		updNow();
    void		readNew(CallBacker*);
    bool		rejectOK();
    bool		acceptOK();
    void		exportCB(CallBacker*);
    void		updateXpos(CallBacker*);
    void		updateYpos(CallBacker*);
    void		updateKbElev(CallBacker*);

    double		getX(int row) const;
    double		getY(int row) const;
    double		getZ(int row) const;
    float		getMD(int row) const;
    void		setX(int row,double);
    void		setY(int row,double);
    void		setZ(int row,double);
    void		setMD(int row,float);

    void		updatePos(bool isx);
    bool		rowIsIncomplete(int) const;
    bool		rowIsNotSet(int) const;
};


/*! \brief Dialog for D2T Model editing. */
mExpClass(uiWell) uiD2TModelDlg : public uiDialog
{ mODTextTranslationClass(uiD2TModelDlg);
public:
			uiD2TModelDlg(uiParent*,Well::Data&,bool chksh);
			~uiD2TModelDlg();

protected:

    Well::Data&		wd_;
    bool		cksh_;
    bool		writable_;
    Well::D2TModel*	orgd2t_; // Must be declared *below* others
    float		origreplvel_;

    uiTable*		tbl_;
    uiCheckBox*		zinftfld_;
    uiCheckBox*		timefld_;
    uiGenInput*		replvelfld_;

    void		fillTable(CallBacker*);
    void		fillReplVel(CallBacker*);
    bool		getFromScreen();
    void		updNow(CallBacker*);
    void		updReplVelNow(CallBacker*);
    void		dtpointChangedCB(CallBacker*);
    void		dtpointRemovedCB(CallBacker*);
    bool		updateDtpointDepth(int row);
    bool		updateDtpointTime(int row);
    bool		updateDtpoint(int row,float oldval);
    void		readNew(CallBacker*);
    bool		rejectOK();
    bool		acceptOK();
    void		expData(CallBacker*);
    void		getModel(Well::D2TModel&);
    void		correctD2TModelIfInvalid();

    void		getColLabels(BufferStringSet&) const;
    int			getTVDGLCol() const;
    int			getTVDSDCol() const;
    int			getTVDSSCol() const;
    int			getTimeCol() const;
    int			getVintCol() const;
    bool		rowIsIncomplete(int row) const;
    int			getPreviousCompleteRowIdx(int row) const;
    int			getNextCompleteRowIdx(int row) const;
    void		setDepthValue(int irow,int icol,float);
    float		getDepthValue(int irow,int icol) const;
    void		setTimeValue(int irow,float);
    float		getTimeValue(int irow) const;
};


/*! \brief Dialog for user made wells */

class uiColorInput;

mExpClass(uiWell) uiNewWellDlg : public uiGetObjectName
{ mODTextTranslationClass(uiNewWellDlg);
public:
			uiNewWellDlg(uiParent*);
			~uiNewWellDlg();

    const Color&	getWellColor();
    const char*		getWellName() const	{ return wellname_; }

protected:

    uiColorInput*	colsel_;
    BufferString	wellname_;
    BufferStringSet*	nms_;

    virtual bool	acceptOK();
    const BufferStringSet& mkWellNms();
};


/* brief Dialog for editing the logs's unit of measure */

mExpClass(uiWell) uiWellLogUOMDlg : public uiDialog
{ mODTextTranslationClass(uiWellLogUOMDlg);
public:
			uiWellLogUOMDlg(uiParent*,ObjectSet<Well::LogSet> wls,
					const BufferStringSet wellnms,
					const BufferStringSet lognms);

protected:

    ObjectSet<Well::Log>	logs_;
    ObjectSet<uiUnitSel>	unflds_;
    uiTable*			uominfotbl_;

    bool		acceptOK();

    void		fillTable(ObjectSet<Well::LogSet> wls,
			     const BufferStringSet& wellnms,
			     const BufferStringSet& lognms);
    bool		setUoMValues();
};



/* brief Dialog to set Depth-to-Time model to selected wells */

mExpClass(uiWell) uiSetD2TFromOtherWell : public uiDialog
{ mODTextTranslationClass(uiSetD2TFromOtherWell);
public:
			uiSetD2TFromOtherWell(uiParent*);
			~uiSetD2TFromOtherWell();

    void		setSelected(const DBKeySet&);

protected:

    bool		acceptOK();

    uiWellSel*		inpwellfld_;
    uiMultiWellSel*	wellfld_;
};
