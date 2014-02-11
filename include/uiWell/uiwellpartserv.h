#ifndef uiwellpartserv_h
#define uiwellpartserv_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          August 2003
 RCS:           $Id$
________________________________________________________________________

-*/

#include "uiwellmod.h"
#include "uiapplserv.h"
#include "bufstringset.h"

namespace Well { class Data; class LogDisplayParSet; }

class uiWell2RandomLineDlg;
class uiWellDispPropDlg;
class uiWellImportAsc;
class uiD2TModelGen;
class uiD2TMLogSelDlg;

/*!
\ingroup uiWell
\brief Part Server for Wells
*/

mExpClass(uiWell) uiWellPartServer : public uiApplPartServer
{
public:
				uiWellPartServer(uiApplService&);
				~uiWellPartServer();

    const char*			name() const		{ return "Wells"; }

				// Services
    void			importTrack();
    void			importLogs();
    void			importMarkers();
    void			bulkImportTrack();
    void			bulkImportLogs();
    void			bulkImportMarkers();
    void			bulkImportD2TModel();

    void			manageWells();
    void			launchRockPhysics();
    bool			selectWells(ObjectSet<MultiID>&);

    bool			hasLogs(const MultiID&) const;
    void			getLogNames(const MultiID&,
					    BufferStringSet&) const;

    bool			editDisplayProperties(const MultiID&);
    void			displayIn2DViewer(const MultiID&);
    bool			showAmplSpectrum(const MultiID&,const char*);

    bool			dispLineOnCreation(){ return disponcreation_; }
    const char*			getRandLineMultiID()   { return multiid_; }
    void			selectWellCoordsForRdmLine();
    void			getRdmLineCoordinates(TypeSet<Coord>&);
    void			sendPreviewEvent();
    Notifier<uiWellPartServer>	randLineDlgClosed;
    Notifier<uiWellPartServer>	uiwellpropDlgClosed;
    void			setPreviewIds( const TypeSet<int>& ids )
				{ previewids_ = ids; }
    TypeSet<int>&		getPreviewIds()	{ return previewids_; }

    void			createSimpleWells()	{ simpImp(0); }
    const BufferStringSet&	createdWellIDs()	{ return crwellids_; }

    void			doLogTools();

    void			createWellFromPicks();
    const char*			askWellName();
    bool			setupNewWell(BufferString&, Color&);

    bool			storeWell(const TypeSet<Coord3>&,const char* nm,
					  MultiID&);

    void			setSceneID( int id )	{ cursceneid_ = id; }
    int				getSceneID() const	{ return cursceneid_; }

    static int		        evPreviewRdmLine();
    static int			evCleanPreview();
    static int			evDisplayWell();

protected:

    uiWellImportAsc*		uiwellimpdlg_;
    uiWell2RandomLineDlg*	rdmlinedlg_;
    uiWellDispPropDlg*		uiwellpropdlg_;
    uiD2TModelGen*		uid2tmgen_;
    uiD2TMLogSelDlg*		uid2tmlogseldlg_;
    TypeSet<int>		previewids_;

    BufferStringSet		crwellids_; // for uiSimpleMultiWellCreate

    int				cursceneid_;
    bool			disponcreation_;
    const char*			multiid_;
    bool			allapplied_;
    bool			isdisppropopened_;

    void			importReadyCB(CallBacker*);
    void			rdmlnDlgClosed(CallBacker*);
    void			wellPropDlgClosed(CallBacker*);
    void			saveWellDispProps(const Well::Data*);
    void			saveWellDispProps(const Well::Data&,
						  const MultiID&);
    void			applyAll(CallBacker*);
    void			simpImp(CallBacker*);

};

#endif

