#ifndef emsurface_h
#define emsurface_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: emsurface.h,v 1.59 2005-11-30 22:00:48 cvskris Exp $
________________________________________________________________________


-*/

#include "emobject.h"
#include "position.h"

template <class T, class AT> class TopList;

/*!
Rules for surfaces.

A horizon can have many patches that can be used for reversed faults.


     ----------------------
     |                    |
     |       xxxxxx       |
     |     xxxxxxxxxx     |
     |   xx Section 1 xxx   |
     |  XXXXXXXXXXXXXXX   |
     |                    |
     |                    |
     |     Section 0        |
     |                    |
     |                    |
     |                    |
     ----------------------

The area marked with x is an area with an reversed fault, and the area with x
is an own patch, while the white area is another patch. In the border between
the patches, the nodes are defined on both patches, with the same coordinate.
In addition, they are also linked together. 
*/

class BinID;
class RowCol;

namespace EM
{
class EMManager;

class EdgeLineManager;
class SurfaceAuxData;
class SurfaceRelations;
class SurfaceGeometry;

/*!\brief Base class for surfaces
  This is the base class for surfaces like horizons and faults. A surface is made up by one or more segments or patches, so they can overlap. 
*/


class Surface : public EMObject
{
public:
    int				nrSections() const;
    EM::SectionID		sectionID(int) const;
    BufferString		sectionName(const SectionID&) const;
    bool			canSetSectionName() const;
    bool			setSectionName(const SectionID&,const char*,
					       bool addtohistory);
    bool			removeSection(SectionID,bool hist);

    void			cleanUp();

    bool			isAtEdge(const EM::PosID&) const;
    bool			isLoaded() const;
    Executor*			saver();
    Executor*			loader();

    const char*			dbInfo() const		 { return dbinfo; }
    void			setDBInfo(const char* s) { dbinfo = s; }

    bool			isChanged(int=-1) const;
    void			resetChangedFlag(int=-1);

    virtual bool		usePar( const IOPar& );
    virtual void		fillPar( IOPar& ) const;

    EMObjectIterator*		createIterator( const SectionID& ) const;

    SurfaceRelations&		relations;
    EdgeLineManager&		edgelinesets;
    SurfaceGeometry&		geometry;
    SurfaceAuxData&		auxdata;

protected:
    friend class		SurfaceGeometry;
    friend class		SurfaceAuxData;
    friend class		EMObject;

    				Surface(EMManager&, SurfaceGeometry&);
    				~Surface();
    Geometry::Element*		getElementInternal( SectionID );

    BufferString		dbinfo;
};


}; // Namespace


#endif
