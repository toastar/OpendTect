#_______________________Pmake__________________________________________________
#
#	CopyRight:	dGB Beheer B.V.
# 	Jan 2012	K. Tingdahl
#	RCS :		$Id$
#_______________________________________________________________________________

set(COINDIR "" CACHE PATH "COIN Location" )

macro(OD_SETUP_COIN)
    if ( (NOT DEFINED COINDIR) OR COINDIR STREQUAL "" )
	message( FATAL_ERROR "COINDIR not set")
    endif()

    FIND_PACKAGE( OpenGL )

    if (WIN32)
	FIND_LIBRARY(COINLIB NAMES Coin3 PATHS ${COINDIR}/lib REQUIRED )
	FIND_LIBRARY(SOQTLIB NAMES SoQt1 PATHS ${COINDIR}/lib REQUIRED )
    else()
	FIND_LIBRARY(COINLIB NAMES Coin PATHS ${COINDIR}/lib REQUIRED )
	FIND_LIBRARY(SOQTLIB NAMES SoQt PATHS ${COINDIR}/lib REQUIRED )
    endif()

    if (WIN32)
	FIND_LIBRARY(OD_SIMVOLEON_LIBRARY NAMES SimVoleon2
		     PATHS ${COINDIR}/lib REQUIRED )
    else()
	set(TMPVAR ${CMAKE_FIND_LIBRARY_SUFFIXES})
	set(CMAKE_FIND_LIBRARY_SUFFIXES ${OD_STATIC_EXTENSION})
	FIND_LIBRARY(OD_SIMVOLEON_LIBRARY NAMES SimVoleon
		     PATHS ${COINDIR}/lib REQUIRED )
	set(CMAKE_FIND_LIBRARY_SUFFIXES ${TMPVAR})
    endif()

    if ( OD_SUBSYSTEM MATCHES ${OD_CORE_SUBSYSTEM} )
	set( CARGS ${COINLIB} ${SOQTLIB} )
	if ( WIN32 )
	    set( CARGS ${CARGS} ${OD_SIMVOLEON_LIBRARY} )
	endif()

	foreach ( LIB ${CARGS} )
	    get_filename_component( COINLIBNAME ${LIB} NAME_WE )
	    if ( UNIX OR APPLE )
		if ( ${OD_PLFSUBDIR} STREQUAL "lux64" OR ${OD_PLFSUBDIR} STREQUAL "lux32" )
		    file( GLOB ALLLIBS "${COINDIR}/lib/${COINLIBNAME}.so.[0-9][0-9]" )
		elseif( APPLE )
		    file( GLOB ALLLIBS "${COINDIR}/lib/${COINLIBNAME}.[0-9][0-9].dylib" )
		endif()
		list( APPEND ARGS ${ALLLIBS} )
		list( GET ARGS 0 FILENM )
		OD_INSTALL_LIBRARY( ${FILENM} )
		list( REMOVE_ITEM ARGS ${ARGS} )
		set( ALLLIBS "" )
	    elseif( WIN32 )
		list( APPEND ARGS ${COINDIR}/bin/${COINLIBNAME}.dll )
	    endif()
	endforeach()
	if ( WIN32 )
	    install( PROGRAMS ${ARGS}
		    DESTINATION ${CMAKE_INSTALL_PREFIX}/bin/${OD_PLFSUBDIR}/${CMAKE_BUILD_TYPE} )
	endif()
    endif()


    if (OD_USECOIN)
	if ( OD_EXTRA_COINFLAGS )
	    add_definitions( ${OD_EXTRA_COINFLAGS} )
	endif( OD_EXTRA_COINFLAGS )

	list(APPEND OD_MODULE_INCLUDESYSPATH ${COINDIR}/include )
	set(OD_COIN_LIBS ${COINLIB} ${SOQTLIB} ${OPENGL_gl_LIBRARY} )
    endif()

    list(APPEND OD_MODULE_EXTERNAL_LIBS ${OD_COIN_LIBS} )
endmacro(OD_SETUP_COIN)
