/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : March 2018
-*/

#include "hdf5reader.h"
#include "hdf5writer.h"
#include "arrayndimpl.h"
#include "uistrings.h"

mImplClassFactory( HDF5::AccessProvider, factory );


HDF5::AccessProvider* HDF5::AccessProvider::mkProv( int idx )
{
    if ( idx<0 || idx>=factory().size() )
	idx = factory().size()-1;
    if ( idx<0 )
	return 0;

    return factory().create( factory().key(idx) );
}


HDF5::Reader* HDF5::AccessProvider::mkReader( int idx )
{
    AccessProvider* prov = mkProv( idx );
    Reader* rdr = 0;
    if ( prov )
    {
	rdr = prov->getReader();
	delete prov;
    }
    return rdr;
}


HDF5::Writer* HDF5::AccessProvider::mkWriter( int idx )
{
    AccessProvider* prov = mkProv( idx );
    Writer* wrr = 0;
    if ( prov )
    {
	wrr = prov->getWriter();
	delete prov;
    }
    return wrr;
}


HDF5::Access::Access()
    : file_(0)
{
}


HDF5::Access::~Access()
{
    // cannot do closeFile(), the reader or writer has already been destructed
}


uiRetVal HDF5::Access::open( const char* fnm )
{
    closeFile();

    uiRetVal uirv;
    openFile( fnm, uirv );

    return uirv;
}


uiString HDF5::Access::sHDF5PackageDispName()
{
    return tr("HDF5 File Access");
}


uiString HDF5::Access::sHDF5Err()
{
    return tr("HDF5 Error");
}


uiString HDF5::Access::sFileNotOpen()
{
    return tr("Could not open HDF5 file");
}


uiRetVal HDF5::Writer::putInfo( const char* grpnm, const IOPar& iop )
{
    uiRetVal uirv;
    ptInfo( DataSetKey(grpnm), iop, uirv );
    return uirv;
}


uiRetVal HDF5::Writer::putInfo( const DataSetKey& dsky, const IOPar& iop )
{
    uiRetVal uirv;
    ptInfo( dsky, iop, uirv );
    return uirv;
}


uiRetVal HDF5::Writer::putData( const DataSetKey& dsky, const ArrayNDInfo& inf,
				const Byte* data, ODDataType dt )
{
    uiRetVal uirv;
    if ( data && inf.getTotalSz() > 0 )
	ptData( dsky, inf, data, dt, uirv );
    return uirv;
}


uiRetVal HDF5::Writer::putData( const DataSetKey& dsky, const FloatArrND& arr )
{
    uiRetVal uirv;
    const ArrayNDInfo& inf = arr.info();
    if ( inf.getTotalSz() < 1 )
	return uirv;

    Byte* data = (Byte*)arr.getData();
    ArrayND<float>* regarr = 0;
    if ( !data )
    {
	regarr = ArrayNDImpl<float>::clone( arr );
	if ( !regarr )
	    { uirv.add( uiStrings::phrCannotAllocateMemory() ); return uirv; }
	data = (Byte*)regarr->getData();
	if ( !data )
	{
	    const char* e_msg = "no getData from ArrayNDImpl";
	    pErrMsg( e_msg );
	    uirv.add( uiStrings::phrInternalErr(e_msg) );
	    return uirv;
	}
    }

    ptData( dsky, inf, data, OD::F32, uirv );
    delete regarr;
    return uirv;
}
