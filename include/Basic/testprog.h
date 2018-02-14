#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H. Bril
 Date:		Dec 2013
________________________________________________________________________

 Single include getting many of the tools you need for test programs.

 The macro mInitTestProg() will take care of:
 1) Initialisation of program args
 2) A file-scope variable 'bool quiet': whether progress info is required
 3) A command line parser 'CommandLineParser clParser()'

-*/

#include "commandlineparser.h"
#include "genc.h"
#include "keystrs.h"
#include "debug.h"
#include "ptrman.h"
#include "od_ostream.h"

#ifdef __win__
# include "winmain.h"
#endif

// Use this in stand-alone test
#define mTestMainFnName testMain

int testMain(int,char**);

#ifndef mMainIsDefined
#define mMainIsDefined
int main(int argc, char** argv)
{
    ExitProgram( testMain( argc, argv ) );
}
#endif

static mUsedVar bool quiet = true;
static mUsedVar PtrMan<CommandLineParser> the_testprog_parser = 0;

static inline CommandLineParser& clParser()
{
    return *the_testprog_parser;
}

static inline mUsedVar od_ostream& tstStream( bool err=false )
{
    if ( !quiet || err )
    {
	if ( err )
	    od_ostream::logStream() << "[FAIL] ";
	return od_ostream::logStream();
    }
    return od_ostream::nullStream();
}


#define mTestProgInits() \
    od_init_test_program( argc, argv ); \
    the_testprog_parser = new CommandLineParser; \
    quiet = clParser().hasKey( sKey::Quiet() )

#define mInitTestProg() mTestProgInits()
#define mInitBatchTestProg() \
    int argc = GetArgC(); char** argv = GetArgV(); \
    mInitTestProg()

#define mRunStandardTestWithError( test, desc, err ) \
if ( !quiet ) \
    od_ostream::logStream() << desc;  \
if ( (test) ) \
{ \
    if ( !quiet ) \
	od_ostream::logStream() << " - SUCCESS\n"; \
} \
else \
{ \
    if ( quiet ) \
	od_ostream::logStream() << desc; \
    od_ostream::logStream() << " - FAIL"; \
    if ( err ) \
	od_ostream::logStream() << ": " << err << "\n"; \
\
    return false; \
}

#define mRunStandardTest( test, desc ) \
	mRunStandardTestWithError( test, desc, BufferString().str() )
