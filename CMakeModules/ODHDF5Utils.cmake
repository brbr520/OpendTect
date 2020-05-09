#_______________________Pmake___________________________________________________
#
#       CopyRight:	dGB Beheer B.V.
#       July 2018	Bert/Arnaud
#_______________________________________________________________________________

cmake_policy( SET CMP0057 NEW )

macro( HDF5CLEANUP )
  unset( HDF5_DIFF_EXECUTABLE CACHE )
  unset( HDF5_DIR CACHE )
  unset( HDF5_IS_PARALLEL CACHE )
  unset( HDF5_USE_STATIC_LIBRARIES CACHE )
  list(APPEND COMPS C CXX )
  list(APPEND LIBSUFFIXES dl hdf5 hdf5_cpp m pthread z)
  foreach( COMP IN LISTS COMPS )
    unset( HDF5_${COMP}_COMPILER_EXECUTABLE CACHE )
    unset( HDF5_${COMP}_COMPILER_NO_INTERROGATE CACHE )
    unset( HDF5_${COMP}_INCLUDE_DIR CACHE )
    foreach( LIBSUFFIX IN LISTS LIBSUFFIXES )
      unset( HDF5_${COMP}_LIBRARY_${LIBSUFFIX} CACHE )
      unset( HDF5_${LIBSUFFIX}_LIBRARY_DEBUG CACHE )
      unset( HDF5_${LIBSUFFIX}_LIBRARY_RELEASE CACHE )
    endforeach()
  endforeach()
endmacro( HDF5CLEANUP )

macro( CHECKTARGETTYPE TARGETNAME )
  if(TARGET ${TARGETNAME})
    set( LIBISTARGET TRUE )
  else()
    set( LIBISTARGET FALSE )
  endif()
endmacro()

macro( GETHDF5COMPDEF )
  CHECKTARGETTYPE(${HDF5_CXX_SHARED_LIBRARY})
  if ( LIBISTARGET )
    get_target_property( HDF5_COMPILEDEF ${HDF5_CXX_SHARED_LIBRARY} INTERFACE_COMPILE_DEFINITIONS )
  endif()
  if ( NOT DEFINED HDF5_COMPILEDEF OR NOT HDF5_COMPILEDEF )
    set( HDF5_COMPILEDEF "H5_BUILT_AS_DYNAMIC_LIB" )
  endif()
endmacro( GETHDF5COMPDEF )

macro( GET_HDF5_ROOT )
  if ( HDF5_DIFF_EXECUTABLE )
    get_filename_component( hdf5_path ${HDF5_DIFF_EXECUTABLE} DIRECTORY )
    get_filename_component( hdf5_path ${hdf5_path} DIRECTORY )
  elseif ( HDF5_INCLUDE_DIRS OR HDF5_INCLUDE_DIR OR HDF5_C_INCLUDE_DIR )
    if ( HDF5_C_INCLUDE_DIR )
	get_filename_component( hdf5_path ${HDF5_C_INCLUDE_DIR} DIRECTORY )
    elseif( HDF5_INCLUDE_DIR )
	list(GET HDF5_INCLUDE_DIR 0 HDF5_FIRST_INCLUDE_DIR )
	get_filename_component( hdf5_path ${HDF5_FIRST_INCLUDE_DIR} DIRECTORY )
    else()
	list(GET HDF5_INCLUDE_DIRS 0 HDF5_FIRST_INCLUDE_DIR )
	get_filename_component( hdf5_path ${HDF5_FIRST_INCLUDE_DIR} DIRECTORY )
    endif()
  elseif( WIN32 )
    set( hdf5_path "" )
  else()
    set( hdf5_path "/usr" )
  endif()
endmacro()

macro( SETHDF5DIR )
  if ( DEFINED HDF5_ROOT )
    if ( WIN32 AND EXISTS "${HDF5_ROOT}/cmake/hdf5" )
      set( HDF5_DIR "${HDF5_ROOT}/cmake/hdf5" )
    elseif( NOT WIN32 AND EXISTS "${HDF5_ROOT}/share/cmake/hdf5" )
      set( HDF5_DIR "${HDF5_ROOT}/share/cmake/hdf5" )
    endif()
  endif()
endmacro()

macro( GET_BUILD_TYPE TARGETNM )
  CHECKTARGETTYPE( ${TARGETNM} )
  if ( LIBISTARGET )
    get_target_property( HDF5_BUILD_TYPE ${TARGETNM} IMPORTED_CONFIGURATIONS )
    list( LENGTH HDF5_BUILD_TYPE HDF5_NRBUILDS )
    if ( ${HDF5_NRBUILDS} GREATER "1" )
      if ( "${CMAKE_BUILD_TYPE}" STREQUAL "Release" )
        if ( "RELEASE" IN_LIST HDF5_BUILD_TYPE )
	  set( HDF5_BUILD_TYPE "RELEASE" )
        elseif( "RELWITHDEBINFO" IN_LIST HDF5_BUILD_TYPE )
	  set( HDF5_BUILD_TYPE "RELWITHDEBINFO" )
        elseif( "DEBUG" IN_LIST HDF5_BUILD_TYPE )
	  set( HDF5_BUILD_TYPE "DEBUG" )
        endif()
      elseif ( "${CMAKE_BUILD_TYPE}" STREQUAL "Debug" )
        if( "DEBUG" IN_LIST HDF5_BUILD_TYPE )
	  set( HDF5_BUILD_TYPE "DEBUG" )
        elseif( "RELWITHDEBINFO" IN_LIST HDF5_BUILD_TYPE )
	  set( HDF5_BUILD_TYPE "RELWITHDEBINFO" )
        elseif ( "RELEASE" IN_LIST HDF5_BUILD_TYPE )
	  set( HDF5_BUILD_TYPE "RELEASE" )
        endif()
      elseif ( "${CMAKE_BUILD_TYPE}" STREQUAL "RelWithDebInfo" )
        if( "RELWITHDEBINFO" IN_LIST HDF5_BUILD_TYPE )
	  set( HDF5_BUILD_TYPE "RELWITHDEBINFO" )
        elseif( "DEBUG" IN_LIST HDF5_BUILD_TYPE )
	  set( HDF5_BUILD_TYPE "DEBUG" )
        elseif ( "RELEASE" IN_LIST HDF5_BUILD_TYPE )
	  set( HDF5_BUILD_TYPE "RELEASE" )
        endif()
      endif()
    endif()
  else()
    string( TOUPPER ${CMAKE_BUILD_TYPE} HDF5_BUILD_TYPE )
  endif()
endmacro()

macro( OD_SETUP_HDF5 )

  if ( NOT DEFINED HDF5_ROOT )
    set( HDF5_ROOT "" CACHE PATH "HDF5 Location" )
    HDF5CLEANUP()
  elseif ( DEFINED HDF5_DIFF_EXECUTABLE OR DEFINED HDF5_C_INCLUDE_DIR OR DEFINED HDF5_INCLUDE_DIRS )
    GET_HDF5_ROOT()
    if ( "${HDF5_ROOT}" STREQUAL "" )
      set( HDF5_ROOT "${hdf5_path}" CACHE PATH "HDF5 Location" FORCE )
      set( HDF5_FOUND TRUE )
    elseif ( NOT "${HDF5_ROOT}" STREQUAL "${hdf5_path}" )
      HDF5CLEANUP()
    endif()
  endif()

  if ( NOT HDF5_FOUND )
    SETHDF5DIR()
    set( HDF5_USE_STATIC_LIBRARIES False )
    find_package( HDF5 QUIET NAMES hdf5 COMPONENTS C CXX shared )
    if ( NOT HDF5_FOUND )
	find_package( HDF5 REQUIRED COMPONENTS C CXX )
    endif()
    if ( HDF5_FOUND )
      GET_HDF5_ROOT()
      set( HDF5_ROOT "${hdf5_path}" CACHE PATH "HDF5 Location" FORCE )
      if ( NOT HDF5_DIR )
	SETHDF5DIR()
        HDF5CLEANUP()
	UNSET( HDF5_FOUND )
	find_package( HDF5 QUIET NAMES hdf5 COMPONENTS C CXX shared )
	if ( NOT HDF5_FOUND )
	    find_package( HDF5 REQUIRED COMPONENTS C CXX )
	endif()
      endif()
      if ( WIN32 )
	#Add path for Visual Studio
	GET_BUILD_TYPE( ${HDF5_CXX_SHARED_LIBRARY} )
	if ( LIBISTARGET )
          get_target_property( HDF5_CXXLIB ${HDF5_CXX_SHARED_LIBRARY} IMPORTED_LOCATION_${HDF5_BUILD_TYPE} )
	  if ( HDF5_CXXLIB )
            get_filename_component( HDF5_RUNTIMEDIR ${HDF5_CXXLIB} DIRECTORY )
            list( APPEND OD_${OD_MODULE_NAME}_RUNTIMEPATH ${HDF5_RUNTIMEDIR} )
	  endif()
	elseif( HDF5_ROOT )
	  list( APPEND OD_${OD_MODULE_NAME}_RUNTIMEPATH "${HDF5_ROOT}/bin" )
	endif()
      endif()
      list(APPEND HDF5_LIBRARIES "${HDF5_C_SHARED_LIBRARY}")
      list(APPEND HDF5_LIBRARIES "${HDF5_CXX_SHARED_LIBRARY}")
      foreach( HDF5_LIB IN LISTS HDF5_LIBRARIES )
	
        set( INSTALL_TARGET False )
	if(TARGET ${HDF5_LIB})
	  set( INSTALL_TARGET True )
	  if ( WIN32 )
	    get_target_property( HDF5_LIB ${HDF5_LIB} IMPORTED_LOCATION_${HDF5_BUILD_TYPE} )
	  else()
	    GET_BUILD_TYPE( ${HDF5_LIB} )
	    get_target_property( HDF5_LIBDIR ${HDF5_LIB} IMPORTED_LOCATION_${HDF5_BUILD_TYPE} )
	    if ( HDF5_LIBDIR )
	      get_filename_component( HDF5_LIBDIR ${HDF5_LIBDIR} DIRECTORY )
	      get_target_property( HDF5_LIBSONAME ${HDF5_LIB} IMPORTED_SONAME_${HDF5_BUILD_TYPE} )
	      set( HDF5_LIB ${HDF5_LIBDIR}/${HDF5_LIBSONAME} )
	    endif()
	  endif()
	elseif ( HDF5_LIB )
	  get_filename_component( HDF5_LIBFNM ${HDF5_LIB} NAME_WE )
	  string(FIND ${HDF5_LIBFNM} "hdf5" INSTALL_TARGET)
	  if ( ${INSTALL_TARGET} LESS 0 )
	    set( INSTALL_TARGET False )
	  else()
	    set( INSTALL_TARGET True )
	    if ( WIN32 )
	      set( HDF5_LIB "${HDF5_ROOT}/bin/${HDF5_LIBFNM}.dll" )
	    endif()
	  endif()
	endif()
	if ( ${INSTALL_TARGET} AND HDF5_LIB AND EXISTS ${HDF5_LIB} )
	  list( APPEND OD_${OD_MODULE_NAME}_PLUGIN_EXTERNAL_DLL ${HDF5_LIB} )
	  if ( OD_INSTALL_DEPENDENT_LIBS )
	    OD_INSTALL_LIBRARY( ${HDF5_LIB} ${CMAKE_BUILD_TYPE} )
	  endif(OD_INSTALL_DEPENDENT_LIBS)
	endif()
      endforeach()
    else()
      HDF5CLEANUP()
      unset( HDF5_ROOT CACHE )
      set( HDF5_ROOT "" CACHE PATH "HDF5 Location" )
    endif()
    if ( NOT HDF5_DIR )
      unset( HDF5_DIR CACHE )
    endif()
  endif()

endmacro( OD_SETUP_HDF5 )
