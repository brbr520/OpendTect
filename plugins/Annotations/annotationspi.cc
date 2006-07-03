/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          August 2004
 RCS:           $Id: annotationspi.cc,v 1.1 2006-07-03 20:02:06 cvskris Exp $
________________________________________________________________________

-*/

#include "plugins.h"
#include "treeitem.h"
#include "uiodscenemgr.h"


extern "C" int GetAnnotationsPluginType()
{
    return PI_AUTO_INIT_LATE;
}


extern "C" PluginInfo* GetAnnotationsPluginInfo()
{
    static PluginInfo retpii = {
	"Annotations",
	"dGB (Nanne Hemstra)",
	"=dgb",
	"Annotation display utilities" };
    return &retpii;
}


extern "C" const char* InitAnnotationsPlugin( int, char** )
{
    ODMainWin()->sceneMgr().treeItemFactorySet()->addFactory(
	    			new Annotations::TreeItemFactory, 10000 );
    return 0;
}
