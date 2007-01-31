/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Helene Huck
 Date:          January 2007
 RCS:           $Id: attribdatapack.cc,v 1.8 2007-01-31 12:01:42 cvshelene Exp $
________________________________________________________________________

-*/

#include "attribdatapack.h"
#include "attribdatacubes.h"
#include "flatdisp.h"
#include "arraynd.h"
#include "arrayndslice.h"


CubeDataPack::CubeDataPack( const Attrib::DataCubes& dc )
    : cube_(dc)
{
    cube_.ref();
    int unuseddim, dim0, dim1;
    if ( cube_.getInlSz() < 2 )
    {
	unuseddim = Attrib::DataCubes::cInlDim();
	dim0 = Attrib::DataCubes::cCrlDim();
	dim1 = Attrib::DataCubes::cZDim();
    }
    else if ( cube_.getCrlSz() < 2 )
    {
	unuseddim = Attrib::DataCubes::cCrlDim();
	dim0 = Attrib::DataCubes::cInlDim();
	dim1 = Attrib::DataCubes::cZDim();
    }
    else
    {
	unuseddim = Attrib::DataCubes::cZDim();
	dim0 = Attrib::DataCubes::cInlDim();
	dim1 = Attrib::DataCubes::cCrlDim();
    }

    arr2dsl_ = new Array2DSlice<float>( cube_.getCube(0) );
    arr2dsl_->setPos( unuseddim, 0 );
    arr2dsl_->setDimMap( 0, dim0 );
    arr2dsl_->setDimMap( 1, dim1 );
    arr2dsl_->init();
}
					

CubeDataPack::~CubeDataPack()
{
    delete arr2dsl_;
    cube_.unRef();
}


Array2D<float>& CubeDataPack::data()
{
    return *arr2dsl_;
}


const Array2D<float>& CubeDataPack::data() const
{
    return const_cast<CubeDataPack*>(this)->data();
}


const CubeSampling CubeDataPack::sampling() const
{
    return cube_.cubeSampling();
}



#define mBuildInterval( rg ) \
    StepInterval<double>( (double)rg.start, (double)rg.stop, (double)rg.step )

void FDCubeDataPack::positioning( FlatDisp::PosData& posdata )
{
    const CubeSampling cs = sampling();
    int nrinl = cs.nrInl();
    int nrcrl = cs.nrCrl();
    int nrzsamp = cs.nrZ();
    posdata.x1rg_ = nrinl<2 ? mBuildInterval( cs.hrg.crlRange() )
			    : mBuildInterval( cs.hrg.inlRange() );
    posdata.x2rg_ = nrinl>2 && nrcrl>2 ? mBuildInterval( cs.hrg.crlRange() )
				       : mBuildInterval( cs.zrg );
}


VertPolyLineDataPack::~VertPolyLineDataPack()
{
    delete arr_;
    delete pos_;
}
