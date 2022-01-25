// watching changed asset files and initiating their hot-reloading
#include "EditorSupport_PCH.h"
#pragma hdrstop
#include <EditorSupport/FileWatcher.h>


const char* AFileWatchListener::Action_To_Chars( efsw::Action action )
{
	switch( action ) {
		case efsw::Actions::Add :		return "add";
		case efsw::Actions::Delete :	return "Delete";
		case efsw::Actions::Modified :	return "Modified";
		case efsw::Actions::Moved :		return "Moved";
			mxDEFAULT_UNREACHABLE( return mxEMPTY_STRING );
	}
	return mxEMPTY_STRING;
}

MyFileWatcher::MyFileWatcher()
{
}

MyFileWatcher::~MyFileWatcher()
{
	this->Shutdown();
}

ERet MyFileWatcher::Initialize()
{
	m_notificationsCS.Initialize();
	return ALL_OK;
}

void MyFileWatcher::Shutdown()
{
	this->RemoveFileWatchers();
	m_notificationsCS.Shutdown();
}

void MyFileWatcher::StartWatching()
{
	m_fileWatcher.watch();
}

void ConvertToNativePath( String & _path )
{
	if( _path.nonEmpty() )
	{
		Str::ReplaceChar( _path, '/', '\\' );
		if( _path.getLast() != '\\' ) {
			Str::Append( _path, '\\' );
		}
	}
}

efsw::WatchID MyFileWatcher::AddFileWatcher( const char* folder, bool recursive )
{
	String256 nativePath;
	Str::CopyS( nativePath, folder );
	ConvertToNativePath( nativePath );

	efsw::WatchID watchID = m_fileWatcher.addWatch( nativePath.c_str(), this, recursive );
	if( watchID < 0 ) {
		ptERROR( "%s", efsw::Errors::Log::getLastErrorLog().c_str() );
		return watchID;
	}
	ptPRINT("Started watching folder: '%s'", folder);
	m_watchedFolders.add( watchID );
	return watchID;
}

void MyFileWatcher::RemoveFileWatchers()
{
	nwFOR_EACH(const efsw::WatchID& watch_id, m_watchedFolders)
	{
		m_fileWatcher.removeWatch( watch_id );
	}
	m_watchedFolders.clear();
}

void MyFileWatcher::ProcessNotifications( AFileWatchListener* listener )
{
	SpinWait::Lock scopedLock( m_notificationsCS );

	nwFOR_EACH(const efswData& efsw_data, m_notifications)
	{
		//DBGOUT("Folder: '%s', FileName: '%s', Action: '%s', OldFileName: '%s'\n",
		//	data.folder.c_str(), data.filename.c_str(),
		//	AFileWatchListener::Action_To_Chars(data.action), data.oldFilename.c_str());
		listener->ProcessChangedFile( efsw_data );
	}

	m_notifications.clear();
}

void MyFileWatcher::handleFileAction(
		efsw::WatchID watchid,
		const std::string& dir,
		const std::string& filename,
		efsw::Action action, std::string oldFilename
	)
{
	SpinWait::Lock scopedLock( m_notificationsCS );

	efswData notification = { watchid, dir, filename, oldFilename, action };
	m_notifications.add( notification );
}
