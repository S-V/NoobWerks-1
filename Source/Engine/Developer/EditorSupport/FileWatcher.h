// watching changed asset files and initiating their hot-reloading
#pragma once

#include <vector>

#include <efsw/efsw.hpp>

#include <Developer/EditorSupport/EditorSupport.h>


struct efswData
{
	efsw::WatchID	id;
	std::string		folder;
	std::string		filename;
	std::string		oldFilename;
	efsw::Action	action;
};

struct AFileWatchListener
{
	virtual void ProcessChangedFile( const efswData& _data ) = 0;

	static const char* Action_To_Chars( efsw::Action action );

protected:
	virtual ~AFileWatchListener() {}
};

class MyFileWatcher : public efsw::FileWatchListener
{
	efsw::FileWatcher 		m_fileWatcher;
	TArray< efsw::WatchID >	m_watchedFolders;

	// these are needed because we receive file notifications in another thread:
	TArray< efswData >		m_notifications;
	SpinWait				m_notificationsCS;

public:
	MyFileWatcher();
	~MyFileWatcher();

	ERet Initialize();
	void Shutdown();

	// Starts watching ( in other thread )
	void StartWatching();

	efsw::WatchID AddFileWatcher( const char* folder, bool recursive );
	void RemoveFileWatchers();

	void ProcessNotifications( AFileWatchListener* listener );

	//--- efsw::FileWatchListener
	// NOTE: this callback is often executed in a separate thread
	virtual void handleFileAction(
		efsw::WatchID watchid,
		const std::string& dir,
		const std::string& filename,
		efsw::Action action, std::string oldFilename = ""
	) override;
};

//--------------------------------------------------------------//
//				End Of File.									//
//--------------------------------------------------------------//
