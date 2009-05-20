#ifndef vismarchingcubessurfacedisplay_h
#define vismarchingcubessurfacedisplay_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: vismarchingcubessurfacedisplay.h,v 1.16 2009-05-20 21:51:25 cvskris Exp $
________________________________________________________________________


-*/

#include "attribsel.h"
#include "emposid.h"
#include "visobject.h"
#include "vissurvobj.h"
#include "ranges.h"

class MarchingCubesSurfaceEditor;
template <class T> class Array3D;

namespace visBase
{
    class BoxDragger;
    class Dragger;
    class Ellipsoid;
    class IndexedPolyLine;
    class MarchingCubesSurface;
    class PickStyle;
    class Transformation;
    class InvisibleLineDragger;
};


namespace EM { class MarchingCubesSurface; }


namespace visSurvey
{
class Scene;

/*!\brief Used for displaying welltracks, markers and logs


*/

mClass MarchingCubesDisplay : public visBase::VisualObjectImpl,
			      public visSurvey::SurveyObject
{
public:
    static MarchingCubesDisplay*create()
				mCreateDataObj(MarchingCubesDisplay);

    MultiID		getMultiID() const;
    bool		isInlCrl() const	{ return true; }

    bool		hasColor() const	{ return true; }
    Color		getColor() const;
    void		setColor(Color);
    bool		allowMaterialEdit() const { return true; }
    NotifierAccess*	materialChange();


    void		setDisplayTransformation(mVisTrans*);
    mVisTrans*		getDisplayTransformation();
    void		setRightHandSystem(bool);


    bool		setVisSurface(visBase::MarchingCubesSurface*);
    			//!<Creates an EMObject for it.
    bool		setEMID(const EM::ObjectID&);
    EM::ObjectID	getEMID() const;

    SurveyObject::AttribFormat	getAttributeFormat(int) const;
    int				nrAttribs() const;
    bool			canAddAttrib(int) const;
    bool			canRemoveAttrib() const;
    bool			canHandleColTabSeqTrans(int) const;
    const ColTab::MapperSetup*	getColTabMapperSetup(int,int) const;

    void			setColTabMapperSetup(int,
	    				const ColTab::MapperSetup&);
    const ColTab::Sequence*	getColTabSequence(int) const;
    bool			canSetColTabSequence() const;
    void                	setColTabSequence(int,const ColTab::Sequence&);
    void                   	setSelSpec(int,const Attrib::SelSpec&);
    const Attrib::SelSpec*	getSelSpec(int attrib) const;

    void			getRandomPos(DataPointSet&) const;
    void			setRandomPosData( int attrib,
	    					  const DataPointSet*);

protected:

    static const char*		sKeyEarthModelID()	{ return "EM ID"; }

    virtual			~MarchingCubesDisplay();
    void			updateVisFromEM(bool onlyshape);
    virtual void		fillPar(IOPar&,TypeSet<int>& saveids) const;
    virtual int			usePar(const IOPar&);

    visBase::MarchingCubesSurface*	displaysurface_;
    EM::MarchingCubesSurface*		emsurface_;
    Attrib::SelSpec			selspec_;
};

};


#endif
