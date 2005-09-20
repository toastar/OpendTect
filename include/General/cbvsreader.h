#ifndef cbvsreader_h
#define cbvsreader_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	A.H.Bril
 Date:		12-3-2001
 Contents:	Common Binary Volume Storage format header
 RCS:		$Id: cbvsreader.h,v 1.25 2005-09-20 16:28:08 cvsbert Exp $
________________________________________________________________________

-*/

#include <cbvsio.h>
#include <cbvsinfo.h>
#include <datainterp.h>
#include <iostream>

class BinIDRange;


/*!\brief Reader for CBVS format

The stream it works on will be deleted on destruction or if close() is
explicitly called.

If you construct with glob_info_only == true, you cannot use the reader. After
construction, the info() is then usable but no precise positioning is available.
In other words, the trailer is not read.

*/

class CBVSReader : public CBVSIO
{
public:

			CBVSReader(std::istream*,bool glob_info_only=false);
			~CBVSReader();

    const CBVSInfo&	info() const		{ return info_; }
    void		close();

    BinID		nextBinID() const;

    bool		goTo(const BinID&,bool nearest_is_ok=false);
    bool		toStart();
    bool		toNext()	{ return skip(false); }
    bool		skip(bool force_next_position=false);
			//!< if force_next_position, will skip all traces
			//!< at current position.

    bool		hasAuxInfo() const		{ return auxnrbytes; }
    void		fetchAuxInfo( bool yn=true )	{ needaux = yn; }
    bool		getAuxInfo(PosAuxInfo&);
    			//!< Gets the aux info. Follow by
			//!< fetch() to get the sample data.
    bool		fetch(void** buffers,const bool* comps=0,
				const Interval<int>* samps=0,
				int offs=0);
    			//!< Gets the sample data.
			//!< 'comps', if provided, selects the components.
			//!< If 'samps' is non-null, it should hold start
			//!< and end sample to read. offs is an offset
			//!< in the buffers.

    static const char*	check(std::istream&);
			//!< Determines whether a file is a CBVS file
			//!< returns an error message, or null if OK.

    int			trcNrAtPosition() const		{ return posidx; }

    const TypeSet<Coord>& trailerCoords() const	 { return trailercoords_; }

protected:

    std::istream&	strm_;
    CBVSInfo		info_;

    void		getAuxInfoSel(const char*);
    bool		readComps();
    bool		readGeom();
    bool		readTrailer();
    void		getText(int,BufferString&);
    void		toOffs(std::streampos);
    bool		getNextBinID(BinID&,int&,int&) const;
    int			getPosNr(const BinID&,bool,bool) const;
    Coord		getTrailerCoord(const BinID&) const;
    void		mkPosNrs();
    bool		goToPosNrOffs(int posnr);
    void		setPos(int,const BinID&,int,int);

private:

    bool		hinfofetched;
    int			bytespertrace;
    BinID		firstbinid;
    BinID		lastbinid;
    int			posidx;
    int			auxnrbytes;
    bool		needaux;
    DataInterpreter<int> iinterp;
    DataInterpreter<float> finterp;
    DataInterpreter<double> dinterp;
    BinIDRange&		bidrg;
    Interval<int>	samprg;
    TypeSet<int>	posnrs;

    bool		readInfo(bool);
    bool		nextPosIdx();

    std::streampos	lastposfo;
    std::streampos	datastartfo;

    friend class	CBVSReadMgr;
    mutable int		curinlinfnr_;
    mutable int		cursegnr_;
    CoordPol		coordPol() const	{ return coordpol_; }

};


#endif
