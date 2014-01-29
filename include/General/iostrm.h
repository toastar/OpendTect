#ifndef iostrm_H
#define iostrm_H

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H. Bril
 Date:		2-8-1995
 RCS:		$Id$
________________________________________________________________________


-*/

#include "generalmod.h"
#include "streamconn.h"
#include "ioobj.h"
#include "ranges.h"
class StreamProvider;


/*\brief An IOStream is a file (default) or command entry in the omf. */


mExpClass(General) IOStream : public IOObj
{
public:
			IOStream(const char* nm=0,const char* id=0,
				 bool =false);
    bool		isBad() const;
    bool		isCommand() const		{ return iscomm_; }

    void		copyFrom(const IOObj*);
    const char*		fullUserExpr(bool forread=true) const;
    const char*		getExpandedName(bool forread,
					bool fillwildcard=true) const;
    void		genDefaultImpl()		{ genFileName(); }

    const char*		connType() const;
    Conn*		getConn(bool) const;

    bool		implExists(bool forread) const;
    bool		implReadOnly() const;
    bool		implRemove() const;
    bool		implShouldRemove() const;
    bool		implSetReadOnly(bool) const;
    bool		implRename(const char*,const CallBack* cb=0);

    bool		multiConn() const
			{ return isMulti() && curfnr_ <= fnrs_.stop; }
    int			connNr() const
			{ return curfnr_; }
    bool		toNextConnNr()
			{ curfnr_ += fnrs_.step; return validNr(); }
    int			lastConnNr() const
			{ return fnrs_.stop; }
    int			nextConnNr() const
			{ return curfnr_+fnrs_.step; }
    void		resetConnNr()
			{ curfnr_ = fnrs_.start; }
    void		setConnNr( int nr )
			{ curfnr_ = nr; }

    const char*		fileName() const		{ return fname_; }
    const char*		subDirName() const		{ return dirName(); }
    const char*		fullDirName() const;
    void		setFileName(const char*);
    void		setExt( const char* ext )	{ extension_ = ext; }
    void		genFileName();

    const char*		reader() const			{ return fname_; }
    const char*		writer() const			{ return writecmd_; }
    void		setReader(const char*);
    void		setWriter(const char*);

    int			zeroPadding() const		{ return padzeros_; }
    void		setZeroPadding( int zp )	{ padzeros_ = zp; }
    StepInterval<int>&	fileNumbers()			{ return fnrs_; }
    const StepInterval<int>& fileNumbers() const	{ return fnrs_; }

    bool		isMulti() const
			{ return fnrs_.start != fnrs_.stop; }

protected:

    bool		getFrom(ascistream&);
    bool		putTo(ascostream&) const;

    BufferString	fname_;
    BufferString	writecmd_;
    bool		iscomm_;
    int			nrfiles_;
    int			padzeros_;
    StepInterval<int>	fnrs_;
    int			curfnr_;
    BufferString	extension_;

    StreamProvider*	getStreamProv(bool,bool f=true) const;
    bool		implDo(bool,bool) const;
    inline bool		validNr() const
			{ return curfnr_*fnrs_.step <= fnrs_.stop*fnrs_.step; }

};


#endif

