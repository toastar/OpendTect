/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Y.C. Liu
 * DATE     : April 2007
-*/


#include "batchprog.h"

#include "file.h"
#include "filepath.h"
#include "dbman.h"
#include "ioobj.h"
#include "dbkey.h"
#include "oddirs.h"
#include "ptrman.h"
#include "segybatchio.h"
#include "segydirectdef.h"
#include "segyscanner.h"
#include "segyfiledef.h"
#include "scaler.h"
#include "seisresampler.h"
#include "seissingtrcproc.h"
#include "keystrs.h"
#include "moddepmgr.h"
#include "zdomain.h"

#include "prog.h"


bool BatchProgram::doImport( od_ostream& strm, IOPar& iop, bool is2d )
{
    PtrMan<IOPar> outpar = iop.subselect( sKey::Output() );
    if ( !outpar || outpar->isEmpty() )
	{ strm << "Batch parameters 'Ouput' empty" << od_endl; return false; }

    SEGY::FileSpec fs; fs.usePar( iop );
    IOObj* inioobj = fs.getIOObj( true );
    if ( !inioobj )
	{ strm << "Input file spec is not OK" << od_endl; return false; }
    const DBKey dbky = DBKey::getFromString( outpar->find(sKey::ID()) );
    PtrMan<IOObj> outioobj = DBM().get( dbky );
    if ( !outioobj )
	{ strm << "Output object spec is not OK" << od_endl; return false; }

    outpar->removeWithKey( sKey::ID() );
	// important! otherwise reader will try to read output ID ...

    SeisSingleTraceProc* stp = new SeisSingleTraceProc( *inioobj, *outioobj,
				"SEG-Y importer", &iop,
				tr("Importing traces") );
    stp->setProcPars( *outpar, is2d );
    return stp->go( strm );
}


bool BatchProgram::doExport( od_ostream& strm, IOPar& iop, bool is2d )
{
    PtrMan<IOPar> inppar = iop.subselect( sKey::Input() );
    if ( !inppar || inppar->isEmpty() )
	{ strm << "Batch parameters 'Input' empty" << od_endl; return false; }

    PtrMan<IOPar> outpar = iop.subselect( sKey::Output() );
    if ( !outpar || outpar->isEmpty() )
	{ strm << "Batch parameters 'Ouput' empty" << od_endl; return false; }

    DBKey indbky;
    inppar->get( sKey::ID(), indbky );
    PtrMan<IOObj> inioobj = DBM().get( indbky );
    if ( !inioobj )
	{ strm << "Input seismics is not OK" << od_endl; return false; }

    SEGY::FileSpec fs; fs.usePar( *outpar );
    IOObj* outioobj = fs.getIOObj( true );
    if ( !outioobj )
	{ strm << "Output SEG-Y file is not OK" << od_endl; return false; }

    int compnr;
    if ( !inppar->get( sKey::Component(), compnr ) )
	compnr = 0;
    
    if ( !ZDomain::isSI(inioobj->pars()) )
	ZDomain::Def::get(inioobj->pars()).set( outioobj->pars() );

    SEGY::FilePars fp; fp.usePar( *outpar );
    fp.fillPar( outioobj->pars() );
    SeisSingleTraceProc* stp = new SeisSingleTraceProc( *inioobj, *outioobj,
			    "SEG-Y exporter", outpar,
			    tr("Exporting traces"), compnr );
    stp->setProcPars( *outpar, is2d );
    return stp->go( strm );
}


static bool doScan( od_ostream& strm, IOPar& iop, bool isps, bool is2d )
{
    DBKey dbky;
    iop.get( sKey::Output(), dbky );
    if ( dbky.isInvalid() )
    {
	iop.get( IOPar::compKey(sKey::Output(),sKey::ID()), dbky );
	if ( dbky.isInvalid() )
	{
	    strm << "Parameter file lacks key 'Output[.ID]'" << od_endl;
	    return false;
	}
    }

    SEGY::FileSpec filespec;
    if ( !filespec.usePar( iop ) )
    {
	strm << "Missing or invalid file name in parameter file\n";
	return false;
    }
    SEGY::FileSpec::makePathsRelative( iop );

    SEGY::FileIndexer indexer( dbky, !isps, filespec, is2d, iop );
    if ( !indexer.go(strm) )
    {
	strm << indexer.message().getFullString();
	DBM().removeEntry( dbky );
	return false;
    }

    IOPar report;
    indexer.scanner()->getReport( report );

    if ( indexer.scanner()->warnings().size() == 1 )
	report.add( "Warning", indexer.scanner()->warnings().get(0) );
    else
    {
	for ( int idx=0; idx<indexer.scanner()->warnings().size(); idx++ )
	{
	    if ( !idx ) report.add( IOPar::sKeyHdr(), "Warnings" );
	    report.add( toString(idx+1 ),
			indexer.scanner()->warnings().get(idx) );
	}
    }

    report.write( strm, IOPar::sKeyDumpPretty() );

    return true;
}


bool BatchProgram::go( od_ostream& strm )
{
    OD::ModDeps().ensureLoaded("Seis");

    const FixedString task = pars().find( SEGY::IO::sKeyTask() );
    const bool isimport = task == SEGY::IO::sKeyImport();
    const bool isexport = task == SEGY::IO::sKeyExport();
    bool is2d = false; pars().getYN( SEGY::IO::sKeyIs2D(), is2d );

    if ( isimport )
	return doImport( strm, pars(), is2d );
    if ( isexport )
	return doExport( strm, pars(), is2d );

    const bool ispsindex = task == SEGY::IO::sKeyIndexPS();
    const bool isvolindex = task == SEGY::IO::sKeyIndex3DVol();

    if ( ispsindex || isvolindex )
	return doScan( strm, pars(), ispsindex, is2d );


    strm << "Unknown task: " << (task.isEmpty() ? "<empty>" : task)
			     << od_newline;
    return false;
}
