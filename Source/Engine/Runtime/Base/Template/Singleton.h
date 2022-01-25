#pragma once


/*
-------------------------------------------------------------------------
	A very simple singleton template.
	Don't (mis)use singletons!
-------------------------------------------------------------------------
*/
//
//	TGlobal< TYPE >
//
// If you want to make object of some class singleton
// you have to derive this class from TGlobal class.
//
template< class TYPE >
class TGlobal: NonCopyable {
public:
	TGlobal()
	{
		mxASSERT2(!HasInstance(),"Singleton class has already been instantiated.");
		gInstance = static_cast< TYPE* >( this );
	}
	~TGlobal()
	{
		mxASSERT(HasInstance());
		gInstance = nil;
	}

	static bool HasInstance()
	{
		return (gInstance != nil);
	}

	static TYPE & Get()
	{
		mxASSERT(HasInstance());
		return (*gInstance);
	}

	//NOTE: doesn't perform any checks!
	static TYPE* Ptr()
	{
		return gInstance;
	}

	typedef TGlobal<TYPE> THIS_TYPE;

	static void staticDestruct()
	{
		Get().~TGlobal();
	}

private:
	// A static pointer to an object of T type.
	// This is the core member of the singleton. 
	// As it is declared as static no more than
	// one object of this type can exist at the time.
	//
	mxTHREAD_LOCAL static TYPE *	gInstance; // the one and only instance
};

template< typename TYPE >
mxTHREAD_LOCAL TYPE * TGlobal< TYPE >::gInstance = nil;




/*
==========================================================
	Helper macros to implement singleton objects:
    
    - DECLARE_SINGLETON      put this into class declaration
    - IMPLEMENT_SINGLETON    put this into the implementation file
    - CONSTRUCT_SINGLETON    put this into the constructor
    - DESTRUCT_SINGLETON     put this into the destructor

    Get a pointer to a singleton object using the static Instance() method:

    Core::Server* coreServer = Core::Server::Instance();
==========================================================
*/
#define DECLARE_SINGLETON(type) \
public: \
    mxTHREAD_LOCAL static type * TheSingleton; \
    static type * Instance() { mxASSERT(nil != TheSingleton); return TheSingleton; }; \
    static bool HasInstance() { return nil != TheSingleton; }; \
	static type & Get() { mxASSERT(nil != TheSingleton); return (*TheSingleton); }; \
private:	PREVENT_COPY(type);


#define DECLARE_INTERFACE_SINGLETON(type) \
public: \
    static type * TheSingleton; \
    static type * Instance() { mxASSERT(nil != TheSingleton); return TheSingleton; }; \
    static bool HasInstance() { return nil != TheSingleton; }; \
	static type & Get() { mxASSERT(nil != TheSingleton); return (*TheSingleton); }; \
private:


#define IMPLEMENT_SINGLETON(type) \
    mxTHREAD_LOCAL type * type::TheSingleton = nil;


#define IMPLEMENT_INTERFACE_SINGLETON(type) \
    type * type::TheSingleton = nil;


#define CONSTRUCT_SINGLETON \
    mxASSERT(nil == TheSingleton); TheSingleton = this;


#define CONSTRUCT_INTERFACE_SINGLETON \
    mxASSERT(nil == TheSingleton); TheSingleton = this;


#define DESTRUCT_SINGLETON \
    mxASSERT(TheSingleton); TheSingleton = nil;


#define DESTRUCT_INTERFACE_SINGLETON \
    mxASSERT(TheSingleton); TheSingleton = nil;

//===========================================================================

#if MX_DEBUG

	//@todo: make sure that TYPE derives from this class
	template< class TYPE >
	struct TSingleInstance: NonCopyable
	{
		//static bool HasInstance() { return ms__hasBeenCreated; }
	protected:
		TSingleInstance()
		{
			mxASSERT( !ms__hasBeenCreated );
			//typeid(__super) == typeid(TYPE);
			ms__hasBeenCreated = true;
		}
		~TSingleInstance()
		{
			mxASSERT( ms__hasBeenCreated );
			ms__hasBeenCreated = false;
		}
	private:
		static bool ms__hasBeenCreated;
	};
	
	template< class TYPE >
	bool TSingleInstance<TYPE>::ms__hasBeenCreated = false;

#else // MX_DEBUG

	template< class TYPE >
	struct TSingleInstance: NonCopyable
	{
	protected:
		TSingleInstance()
		{
		}
		~TSingleInstance()
		{
		}
	};

#endif // !MX_DEBUG
