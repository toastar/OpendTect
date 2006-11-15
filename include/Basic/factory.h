#ifndef factory_h
#define factory_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	A.H.Bril
 Date:		Sep 1994, Aug 2006
 RCS:		$Id: factory.h,v 1.3 2006-11-15 14:46:51 cvskris Exp $
________________________________________________________________________

-*/

#include "bufstringset.h"

/*!
Generalized static factory that can deliver instances of T, when no
variable is needed in the creation.

Usage. Each implementation of the base class T must add themselves
to the factory when application starts up, e.g. in an initClass() function:
\code
class A
{
public:
    virtual int		myFunc() 	= 0;
};

class B : public A
{
public:
    static A*		createFunc() { return new B; }
    static void		initClass()
    			{ thefactory.addCreator(createFunc,"My Name"); }
			    
    int			myFunc();
};

\endcode

Two macros are available to make a static accessfuncion for the factory:
\code
mDefineFactory( ClassName, FunctionName );
\endcode

that will create a static function that returns an instance to
Factory<ClassName>. The static function must be implemented in a src-file
with the macro

\code
mImplFactory( ClassName, FunctionName );
\endcode

*/


template <class T>
class Factory
{
public:
    typedef			T* (*Creator)();
    inline void			addCreator(Creator,const char* nm);
    inline T*			create(const char* nm) const;
    const BufferStringSet&	getNames() const { return names_; }
protected:

    BufferStringSet		names_;
    TypeSet<Creator>		creators_;
};


/*!
Generalized static factory that can deliver instances of T, when a
variable is needed in the creation.

Usage. Each implementation of the base class T must add themselves
to the factory when application starts up, e.g. in an initClass() function:
\code
class A
{
public:
    virtual int		myFunc() 	= 0;
};

class B : public A
{
public:
    static A*		createFunc(C* param) { return new B(param); }
    static void		initClass()
    			{ thefactory.addCreator(createFunc,"My Name"); }
			    
    int			myFunc();
};

\endcode

Two macros are available to make a static accessfuncion for the factory:
\code
mDefineFactory1Param( ClassName, ParamClass, FunctionName );
\endcode

that will create a static function that returns an instance to
Factory1Param<ClassName,ParamClass>. The static function must be implemented
in a src-file with the macro

\code
mImplFactory1Param( ClassName, ParamClass, FunctionName );
\endcode

*/


template <class T, class P>
class Factory1Param
{
public:
    typedef			T* (*Creator)(P);
    inline void			addCreator(Creator,const char* nm);
    inline T*			create(const char* nm, P, bool chknm=true)const;
    const BufferStringSet&	getNames() const { return names_; }
protected:

    BufferStringSet		names_;
    TypeSet<Creator>		creators_;
};


template <class T, class P0, class P1>
class Factory2Param
{
public:
    typedef			T* (*Creator)(P0,P1);
    inline void			addCreator(Creator,const char* nm);
    inline T*			create(const char* nm, P0, P1,
	    			       bool chknm=true)const;
    const BufferStringSet&	getNames() const { return names_; }
protected:

    BufferStringSet		names_;
    TypeSet<Creator>		creators_;
};


template <class T> inline
void Factory<T>::addCreator( Creator cr, const char* name )
{
    names_.add( name );
    creators_ += cr;
}


template <class T> inline
T* Factory<T>::create( const char* name ) const
{
    const int idx = names_.indexOf( name );
    if ( idx<0 ) return 0;

    return creators_[idx]();
}


template <class T, class P> inline
void Factory1Param<T,P>::addCreator( Creator cr, const char* name )
{
    names_.add( name );
    creators_ += cr;
}


template <class T, class P> inline
T* Factory1Param<T,P>::create( const char* name, P data, bool chk ) const
{
    if ( chk )
    {
	const int idx = names_.indexOf( name );
	if ( idx<0 ) return 0;

	return creators_[idx]( data );
    }

    for ( int idx=0; idx<creators_.size(); idx++ )
    {
	T* res = creators_[idx]( data );
	if ( res ) return res;
    }

    return 0;
}


template <class T, class P0, class P1> inline
void Factory2Param<T,P0,P1>::addCreator( Creator cr, const char* name )
{
    names_.add( name );
    creators_ += cr;
}


template <class T, class P0, class P1> inline
T* Factory2Param<T,P0,P1>::create( const char* name, P0 p0, P1 p1,
				   bool chk ) const
{
    if ( chk )
    {
	const int idx = names_.indexOf( name );
	if ( idx<0 ) return 0;

	return creators_[idx]( p0,p1 );
    }

    for ( int idx=0; idx<creators_.size(); idx++ )
    {
	T* res = creators_[idx]( p0, p1 );
	if ( res ) return res;
    }

    return 0;
}


#define mDefineFactory( T, funcname ) \
Factory<T>& funcname()


#define mImplFactory( T, funcname ) \
Factory<T>& funcname() \
{ \
    static Factory<T> inst; \
    return inst; \
} 


#define mDefineFactory1Param( T, P, funcname ) \
Factory1Param<T,P>& funcname()


#define mImplFactory1Param( T, P, funcname ) \
Factory1Param<T,P>& funcname() \
{ \
    static Factory1Param<T,P> inst; \
    return inst; \
} 


#define mDefineFactory2Param( T, P0, P1, funcname ) \
Factory2Param<T,P0,P1>& funcname()


#define mImplFactory2Param( T, P0, P1, funcname ) \
Factory2Param<T,P0,P1>& funcname() \
{ \
    static Factory2Param<T,P0,P1> inst; \
    return inst; \
} 
#endif
