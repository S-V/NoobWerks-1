/*
=============================================================================
	File:	Profiler.h
	Desc:	A simple hierarchical profiler.
			Swiped from Bullet / OGRE / Game Programming Gems 3
			( original code by Greg Hjelstrom & Byon Garrabrant).
=============================================================================
*/
#pragma once

//----------------------------------------------------------------------------------
// Profiling/instrumentation support
//----------------------------------------------------------------------------------

#if MX_ENABLE_PROFILING

	//
	//	PtProfileNode - A node in the Profile Hierarchy Tree.
	//
	class PtProfileNode {
	public:
			PtProfileNode( const char * name, PtProfileNode * parent );
			~PtProfileNode( void );

		PtProfileNode *		Get_Sub_Node( const char * name );

		PtProfileNode *		Get_Parent( void )		{ return Parent; }
		PtProfileNode *		Get_Sibling( void )		{ return Sibling; }
		PtProfileNode *		Get_Child( void )		{ return Child; }

		void				CleanupMemory();
		void				Reset( void );
		void				Call( void );
		bool				Return( void );

		const char *		Get_Name( void )			{ return Name; }
		int					Get_Total_Calls( void )		{ return TotalCalls; }
		float				Get_Total_Time( void )		{ return TotalTime; }

	protected:
		const char *	Name;
		int				TotalCalls;	// number of times category was profiled
		float			TotalTime;	// total time this category incurred
		//float			MaxTime;	// maximum time this category took
		//float			MinTime;	// minimum time this category took
		unsigned long 	StartTime;
		int				RecursionCounter;

		PtProfileNode *	Parent;
		PtProfileNode *	Child;
		PtProfileNode *	Sibling;
	};

	///An iterator to navigate through the tree
	class PtProfileIterator
	{
	public:
		// Access all the children of the current parent
		void			First(void);
		void			Next(void);
		bool			Is_Done(void);
		bool            Is_Root(void) { return (CurrentParent->Get_Parent() == 0); }

		void			Enter_Child( int index );		// Make the given child the new parent
		void			Enter_Largest_Child( void );	// Make the largest child the new parent
		void			Enter_Parent( void );			// Make the current parent's parent the new parent

		// Access the current child
		const char *	Get_Current_Name( void )			{ return CurrentChild->Get_Name(); }
		int				Get_Current_Total_Calls( void )		{ return CurrentChild->Get_Total_Calls(); }
		float			Get_Current_Total_Time( void )		{ return CurrentChild->Get_Total_Time(); }

		// Access the current parent
		const char *	Get_Current_Parent_Name( void )			{ return CurrentParent->Get_Name(); }
		int				Get_Current_Parent_Total_Calls( void )	{ return CurrentParent->Get_Total_Calls(); }
		float			Get_Current_Parent_Total_Time( void )	{ return CurrentParent->Get_Total_Time(); }

	protected:

		PtProfileNode *	CurrentParent;
		PtProfileNode *	CurrentChild;

		PtProfileIterator( PtProfileNode * start );
		friend	class		PtProfileManager;
	};


	///The Manager for the Profile system
	class PtProfileManager {
	public:
		static	void				Start_Profile( const char * name );
		static	void				Stop_Profile( void );

		static	void				CleanupMemory(void)		{ Root.CleanupMemory(); }

		static	void				Reset( void );
		static	void				Increment_Frame_Counter( void );
		static	int					Get_Frame_Count_Since_Reset( void )		{ return FrameCounter; }
		static	float				Get_Time_Since_Reset( void );

		static	PtProfileIterator *	Get_Iterator( void )	{ return new PtProfileIterator( &Root ); }
		static	void				Release_Iterator( PtProfileIterator * iterator ) { delete( iterator ); }

		static void	dumpRecursive( PtProfileIterator* profileIterator, int spacing, class ALog* logger );

		static void	dumpAll( class ALog* logger );

	private:
		static	PtProfileNode			Root;
		static	PtProfileNode *			CurrentNode;
		static	int						FrameCounter;
		static	unsigned long int		ResetTime;
	};


	///ProfileSampleClass is a simple way to profile a function's scope
	///Use the mxPROFILE_SCOPE macro at the start of scope to time
	class PtProfileSample {
	public:
		PtProfileSample( const char * name )
		{ 
			PtProfileManager::Start_Profile( name );
		}
		~PtProfileSample( void )
		{ 
			PtProfileManager::Stop_Profile();
		}
	};

	//
	// Profiling instrumentation macros
	//

	#define	mxPROFILE_SCOPE( name )			PtProfileSample __profile( name )

	#define	mxPROFILE_FUNCTION				PtProfileSample __profile( __FUNCTION__ )

	#define	mxPROFILE_SCOPE_BEGIN( name )	{ PtProfileSample __profile( name )
	#define mxPROFILE_SCOPE_END				}

	#define mxPROFILE_INCREMENT_FRAME_COUNTER	PtProfileManager::Increment_Frame_Counter()

#else // ifndef MX_ENABLE_PROFILING

	#define	mxPROFILE_SCOPE( name )

	#define	mxPROFILE_FUNCTION

	#define	mxPROFILE_SCOPE_BEGIN( name )
	#define mxPROFILE_SCOPE_END

	#define mxPROFILE_INCREMENT_FRAME_COUNTER

#endif // ifndef MX_ENABLE_PROFILING

//----------------------------------------------------------//
//				End Of File.									//
//----------------------------------------------------------//
