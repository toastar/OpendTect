#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H.Bril/K.Tingdahl
 Date:		13-10-1999
________________________________________________________________________

-*/

#include "basicmod.h"
#include "namedobj.h"
#include "objectset.h"
#include "threadlock.h"
#include "uistring.h"

class ProgressMeter;
namespace Threads { class ConditionVar; }


/*!\brief Generalization of something (e.g. a computation) that needs to be
	done in multiple steps. */

mExpClass(Basic) Task : public NamedMonitorable
{ mODTextTranslationClass(Task);
public:

    virtual		~Task();

    virtual void	setProgressMeter(ProgressMeter*)	{}
			//!<Must be called before execute()

    virtual uiString	message() const		{ return stdMessage(); }
    virtual uiString	nrDoneText() const	{ return stdNrDoneText(); }
    virtual od_int64	nrDone() const		{ return -1; }
				/*!<\note only used for displaying progress. */
    virtual od_int64	totalNr() const		{ return -1; }
				/*!\note only used for displaying progress. */
    virtual uiRetVal	errorWithDetails() const { return uiRetVal(message()); }

    virtual bool	execute()		= 0;

    enum Control	{ Run, Pause, Stop };
    virtual void	enableWorkControl(bool=true);
			//!<Must be called before execute()
    bool		workControlEnabled() const;
    virtual void	controlWork(Control);
    virtual Control	getState() const;

    static uiString	stdMessage()		{ return tr("Working");}
    static uiString	stdNrDoneText()		{ return tr("Nr Done");}

protected:

				Task(const char* nm=0);
    virtual bool		shouldContinue();
					//!<\returns wether we should continue
    Control			control_;
    Threads::ConditionVar*	workcontrolcondvar_;

};


/*!Helper class that facilitates a task that has multiple sub-tasks that
   are either run in parallel or in sequence.

   The class takes care of progress reporting as well as work
   control.
*/


mExpClass(Basic) TaskGroupController : public Task
{ mODTextTranslationClass(TaskGroupController);
public:

    od_int64		nrDone() const;
			//!<Percentage
    od_int64		totalNr() const	{ return 100; }

    uiString		nrDoneText() const
			{ return tr("Percentage done"); }

    void		enableWorkControl(bool=true);
    void		controlWork(Control);
			//!<Relays to controlled tasks

    int			nrTasks() const { return controlledtasks_.size();}
    const Task*		getTask(int idx) const { return controlledtasks_[idx]; }

protected:

    void		controlTask(Task*);
			//!<Does not take over memory management
    void		setEmpty();

private:

    ObjectSet<Task>	controlledtasks_;
    TypeSet<float>	nrdoneweights_;

};


/*!\brief A collection of tasks, that behave as a single task.  */

mExpClass(Basic) TaskGroup : public TaskGroupController
{
public:
			TaskGroup();
			~TaskGroup() { deepErase( tasks_ ); }
    void		addTask( Task* );
			//Becomes mine

    void		setParallel(bool)			{}
    void		showCumulativeCount( bool yn )
    			{ showcumulativecount_ = yn; }

    void		setProgressMeter(ProgressMeter*);
    void		setEmpty();
    void		getTasks(TaskGroup&);

    od_int64		nrDone() const;
    od_int64		totalNr() const;

    uiString		message() const;
    uiString		nrDoneText() const;

    virtual bool	execute();

protected:

    ObjectSet<Task>	tasks_;
    int			curtask_;
    bool		showcumulativecount_;

    mutable Threads::Lock lock_;

};


/*!\brief The generalization of something (e.g. a computation) where the steps
  must be done in sequence, i.e. not in parallel.
*/

mExpClass(Basic) SequentialTask : public Task
{
public:
			SequentialTask(const char* nm=0);
    virtual		~SequentialTask()			{}

    void		setProgressMeter(ProgressMeter*);
    ProgressMeter*	progressMeter()		{ return progressmeter_; }
    const ProgressMeter* progressMeter() const	{ return progressmeter_; }

    virtual int		doStep();
		/*!<\retval MoreToDo()		Not finished. Call me again.
		    \retval Finished()		Nothing more to do.
		    \retval ErrorOccurred()	Something went wrong.
		    \note if function returns a value greater than cMoreToDo(),
		     it should be interpreted as cMoreToDo(). */

    static int		ErrorOccurred()			{ return -1; }
    static int		Finished()			{ return 0; }
    static int		MoreToDo()			{ return 1; }
    static int		WarningAvailable()		{ return 2; }

    bool		execute();

protected:

    virtual int		nextStep()				= 0;
			/*!<\retval MoreToDo()	Not finished. Call me again.
			 \retval Finished()		Nothing more to do.
			 \retval ErrorOccurred()	Something went wrong.
			 \note if function returns a value greater than
			  cMoreToDo(),it should be interpreted as cMoreToDo().*/

    ProgressMeter*	progressmeter_;
    int			lastupdate_;

};



/*!\brief Class that can execute a task.

  Can be used as such, be inherited by fancy subclasses with user interface
  and progressbars etc.
*/

mExpClass(Basic) TaskRunner
{
public:
    static bool		execute(TaskRunner* tskr, Task& );
			//!<Taskrunner may be zero

			TaskRunner() : execres_(false)	{}
    virtual		~TaskRunner()			{}

    virtual bool	execute(Task& t)
			{ return (execres_ = t.execute()); }
    virtual bool	execResult() const		{ return execres_; }

protected:

    bool		execres_;

};
