#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Bril
 Date:          Sep 2002
________________________________________________________________________

-*/

#include "uiearthmodelmod.h"

#include "emposid.h"
#include "trckeysampling.h"
#include "dbkey.h"
#include "position.h"
#include "uiapplserv.h"
#include "uistring.h"


class BinIDValueSet;
class BufferStringSet;
class TrcKeyZSampling;
class DataPointSet;
class TrcKeySampling;
class SurfaceInfo;
class ZAxisTransform;
class uiBulkFaultImport;
class uiBulkHorizonImport;
class uiCreateHorizon;
class uiExportFault;
class uiExportHorizon;
class uiImportFault3D;
class uiImportFaultStickSet2D;
class uiImportHorizon;
class uiSurfaceMan;
class uiVariogramDisplay;

namespace EM { class EMObject; class EMManager; class SurfaceIODataSelection; }
namespace Pick { class Set; }
namespace PosInfo { class Line2DData; }

template <class T> class Array2D;


/*!\brief Earth Model UI Part Server */

mExpClass(uiEarthModel) uiEMPartServer : public uiApplPartServer
{ mODTextTranslationClass(uiEMPartServer);
public:
			uiEMPartServer(uiApplService&);
			~uiEMPartServer();

    const char*		name() const			{ return "EarthModel"; }

			// Services
    bool		import3DHorGeom(bool bulk=false);
    bool		import3DHorAttr();
    bool		export3DHorizon();
    bool		export2DHorizon();
    bool		importFault(bool bulk);
    bool		importFaultStickSet();
    void		import2DFaultStickset();
    bool		exportFault();
    bool		exportFaultStickSet();
    void		createHorWithConstZ(bool is2d);

    DBKey		getStorageID(const EM::ObjectID&) const;
    EM::ObjectID	getObjectID(const DBKey&) const;

    uiString		getName(const EM::ObjectID&) const;
    uiString		getType(const EM::ObjectID&) const;

    int			nrAttributes(const EM::ObjectID&) const;
    bool		isGeometryChanged(const EM::ObjectID&) const;
    bool		isChanged(const EM::ObjectID&) const;
    bool		isEmpty(const EM::ObjectID&) const;
    bool		isFullResolution(const EM::ObjectID&) const;
    bool		isFullyLoaded(const EM::ObjectID&) const;

    void		displayEMObject(const DBKey&);
    bool		fillHoles(const EM::ObjectID&,bool);
			/*!<return bool is overwrite old horizon or not. */
    bool		filterSurface(const EM::ObjectID&);
			/*!<return bool is overwrite old horizon or not. */
    void		fillPickSet(Pick::Set&,DBKey);
    void		deriveHor3DFrom2D(const EM::ObjectID&);
    bool		askUserToSave(const EM::ObjectID&,bool withcancl) const;
			/*!< If object has changed, user is asked whether
			    to save it or not, and if so, the object is saved.
			    Returns false when save option is cancelled. */

    TrcKeySampling	horizon3DDisplayRange() const
				{ return selectedrg_; }
    void		setHorizon3DDisplayRange(const TrcKeySampling&);
			/*!<Users can change the display range, hor 3D only. */

    void		selectHorizons(ObjectSet<EM::EMObject>&,bool is2d);
			//!<Returned set is reffed and must be unrefed by caller
    void		selectFaults(ObjectSet<EM::EMObject>&,bool is2d);
			//!<Returned set is reffed and must be unrefed by caller
    void		selectFaultStickSets(ObjectSet<EM::EMObject>&);
			//!<Returned set is reffed and must be unrefed by caller
    void		selectBodies(ObjectSet<EM::EMObject>&);
			//!<Returned set is reffed and must be unrefed by caller
    bool		showLoadAuxDataDlg(const EM::ObjectID&);
    int			loadAuxData(const EM::ObjectID&,const char*,
				    bool removeold=true);
			/*!<Loads the specified data into memory and returns
			    its auxdatanr. */
    bool		loadAuxData(const EM::ObjectID&,const BufferStringSet&,
				    bool removeold=true);

    bool		showLoadFaultAuxDataDlg(const EM::ObjectID&);
    bool		storeFaultAuxData(const EM::ObjectID& id,
					  BufferString& auxdatanm,
					  const Array2D<float>& data);
    void		manageSurfaces(const char* typ);
    void		manage2DHorizons();
    void		manage3DHorizons();
    void		manageFaultStickSets();
    void		manage3DFaults();
    void		manageBodies();
    bool		loadSurface(const DBKey&,
				    const EM::SurfaceIODataSelection* s=0);
    void		getSurfaceInfo(ObjectSet<SurfaceInfo>&);
    static void         getAllSurfaceInfo(ObjectSet<SurfaceInfo>&,bool);
    void		getSurfaceDef3D(const TypeSet<EM::ObjectID>&,
				        BinIDValueSet&,
				        const TrcKeySampling&) const;
    void		getSurfaceDef2D(const DBKeySet&,
					const BufferStringSet& sellines,
					TypeSet<Coord>&,
					TypeSet<Pos::SurvID>&,
					TypeSet< Interval<float> >&);

    bool		storeObject(const EM::ObjectID&,
				    bool storeas=false) const;
    bool		storeObject(const EM::ObjectID&,bool storeas,
				    DBKey& storagekey,
				    float shift=0) const;
    bool		storeAuxData(const EM::ObjectID&,
				     BufferString& auxdataname,
				     bool storeas=false) const;
    int			setAuxData(const EM::ObjectID&,
				   DataPointSet&,const char* nm,int valnr,
				   float shift);
    bool		getAuxData(const EM::ObjectID&,int auxdatanr,
				   DataPointSet&, float& shift) const;
    bool		getAllAuxData(const EM::ObjectID&,DataPointSet&,
				      TypeSet<float>* shfs=0,
				      const TrcKeyZSampling* cs=0) const;
    bool		interpolateAuxData(const EM::ObjectID&,const char* nm,
					   DataPointSet& res);
    bool		filterAuxData(const EM::ObjectID&,const char* nm,
				      DataPointSet& res);
    bool		computeVariogramAuxData(const EM::ObjectID&,const char*,
						DataPointSet&);
    bool		attr2Geom(const EM::ObjectID&,const char* nm,
				  const DataPointSet&);
    bool		geom2Attr(const EM::ObjectID&);
    ZAxisTransform*	getHorizonZAxisTransform(bool is2d);

    DBKey		genRandLine(int opt);
    bool		dispLineOnCreation()	{ return disponcreation_; }

    void		removeUndo();

    static int		evDisplayHorizon();
    static int		evRemoveTreeObject();

			// Interaction stuff
    const EM::ObjectID&	selEMID() const			{ return selemid_; }
    EM::EMObject*	selEMObject();

    void		removeTreeObject(const EM::ObjectID&);

    void		managePreLoad();
    void		fillPar(IOPar&) const;
    bool		usePar(const IOPar&);

protected:

    void		selectSurfaces(ObjectSet<EM::EMObject>&,
				       const char* type);
    bool		loadAuxData(const EM::ObjectID&,const TypeSet<int>&,
				    bool removeold=true);
    bool		changeAuxData(const EM::ObjectID&,const char* nm,
				      bool interp,DataPointSet& res);
    void		importReadyCB(CallBacker*);
    void		survChangedCB(CallBacker*);
    void		displayOnCreateCB(CallBacker*);

    EM::ObjectID	selemid_;
    EM::EMManager&	em_;
    uiImportHorizon*	imphorattrdlg_;
    uiImportHorizon*	imphorgeomdlg_;
    uiBulkHorizonImport* impbulkhordlg_;
    uiImportFault3D*	impfltdlg_;
    uiBulkFaultImport*	impbulkfltdlg_;
    uiImportFault3D*	impfltstickdlg_;
    uiImportFaultStickSet2D*	impfss2ddlg_;
    uiExportHorizon*	exphordlg_;
    uiExportFault*	expfltdlg_;
    uiExportFault*	expfltstickdlg_;
    uiCreateHorizon*	crhordlg_;

    TrcKeySampling		selectedrg_;
    bool		disponcreation_;

    ObjectSet<uiVariogramDisplay>	variodlgs_;

    static const char*  sKeySectionID() { return "Section ID"; }
    uiSurfaceMan*	man2dhordlg_;
    uiSurfaceMan*	man3dhordlg_;
    uiSurfaceMan*	ma3dfaultdlg_;
    uiSurfaceMan*	manfssdlg_;
    uiSurfaceMan*	manbodydlg_;
};
