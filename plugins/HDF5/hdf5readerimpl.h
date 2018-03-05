#pragma once
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Feb 2018
________________________________________________________________________

-*/

#include "hdf5common.h"
#include "hdf5accessimpl.h"
#include "hdf5reader.h"
#include "H5Cpp.h"


namespace HDF5
{

mExpClass(HDF5) ReaderImpl : public Reader
			   , public AccessImpl
{
public:

    typedef H5::DataType	H5DataType;

			ReaderImpl();
			~ReaderImpl();

    const char*		fileName() const	{ return gtFileName(); }

    virtual void	getGroups(const GroupPath&,BufferStringSet&) const;
    virtual ArrayNDInfo* getDataSizes(const GroupPath&,uiRetVal&) const;

protected:

    virtual void	openFile(const char*,uiRetVal&);
    virtual void	closeFile()		{ doCloseFile(*this); }

};

} // namespace HDF5
