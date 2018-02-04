#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Prajjaval Singh
 Date:		Jan 2017
________________________________________________________________________

-*/

#include "uiodmainmod.h"
#include "callback.h"
#include "uistring.h"

class uiStratLayerModelManager;
class uiStratLayerModel;


class uiStratLayerModelManager : public CallBacker
{ mODTextTranslationClass(uiStratLayerModelManager);
public:

			uiStratLayerModelManager();


    bool		haveExistingDlg();
    uiStratLayerModel*	getDlg();
    void		launchLayerModel(const char* modnm,int opt);

    void		addToTreeWin();

    static uiStratLayerModelManager& uislm_manager();
    static uiStratLayerModel* getUILayerModel();

    static void		initClass();
    static void		doBasicLayerModel();
    static void		doLayerModel(const char* modnm,int opt);

protected:

    void		survChg( CallBacker* );
    void		winClose( CallBacker* );
    void		startCB( CallBacker* cb );

    uiStratLayerModel*	dlg_;

};
