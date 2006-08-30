#ifndef seispsfact_h
#define seispsfact_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	A.H. Bril
 Date:		21-3-1996
 RCS:		$Id: seispsfact.h,v 1.1 2006-08-30 16:03:26 cvsbert Exp $
________________________________________________________________________

-*/

#include "seispsioprov.h"

#define sKeySeisPSTranslatorGroup "Pre-Stack Seismics"
defineTranslatorGroup(SeisPS,sKeySeisPSTranslatorGroup);
defineTranslator(CBVS,SeisPS,"CBVS");


#endif
