/***************************************************************************
                                kompare_part.cpp  -  description
                                -------------------
        begin                   : Sun Mar 4 2001
        copyright               : (C) 2001-2003 by Otto Bruggeman
                                  and John Firebaugh
        email                   : otto.bruggeman@home.nl
                                  jfirebaugh@kde.org
****************************************************************************/

/***************************************************************************
**
**   This program is free software; you can redistribute it and/or modify
**   it under the terms of the GNU General Public License as published by
**   the Free Software Foundation; either version 2 of the License, or
**   (at your option) any later version.
**
***************************************************************************/

#include <qlayout.h>
#include <qwidget.h>

#include <kaction.h>
#include <kdebug.h>
#include <kfiletreeview.h>
#include <kfiledialog.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kstdaction.h>
#include <kinstance.h>
#include <ktempfile.h>
//#include <ktempdir.h>

#include <kio/netaccess.h>

#include "diffmodel.h"
#include "kompare_part.h"
#include "komparelistview.h"
#include "kompareconnectwidget.h"
#include "diffsettings.h"
#include "viewsettings.h"
#include "kompareprefdlg.h"
#include "komparesaveoptionswidget.h"
#include "kompareview.h"


ViewSettings* KomparePart::m_viewSettings = 0L;
DiffSettings* KomparePart::m_diffSettings = 0L;

KomparePart::KomparePart( QWidget *parentWidget, const char *widgetName,
                      QObject *parent, const char *name ) :
	KParts::ReadWritePart(parent, name),
	m_tempDiff( 0 ),
	m_info()
{
	// we need an instance
	setInstance( KomparePartFactory::instance() );

	if( !m_viewSettings ) {
		m_viewSettings = new ViewSettings( 0 );
	}
	if( !m_diffSettings ) {
		m_diffSettings = new DiffSettings( 0 );
	}

	// This creates the "Model creator" and connects the signals and slots
	m_modelList = new Diff2::KompareModelList( m_diffSettings, &m_info, this, "komparemodellist" );
	connect( m_modelList, SIGNAL(status( Kompare::Status )),
	         this, SLOT(slotSetStatus( Kompare::Status )) );
	connect( m_modelList, SIGNAL(error( QString )),
	         this, SLOT(slotShowError( QString )) );
	connect( m_modelList, SIGNAL(applyAllDifferences( bool )),
	         this, SLOT(updateActions()) );
	connect( m_modelList, SIGNAL(applyDifference( bool )),
	         this, SLOT(updateActions()) );
	connect( m_modelList, SIGNAL(applyAllDifferences( bool )),
	         this, SIGNAL(appliedChanged()) );
	connect( m_modelList, SIGNAL(applyDifference( bool )),
	         this, SIGNAL(appliedChanged()) );

	connect( m_modelList, SIGNAL( setModified( bool ) ),
	         this, SLOT( slotSetModified( bool ) ) );

	// This is the stuff to connect the "interface" of the kompare part to the model inside
	connect( m_modelList, SIGNAL(modelsChanged(const QPtrList<Diff2::DiffModel>*)),
	         this, SIGNAL(modelsChanged(const QPtrList<Diff2::DiffModel>*)) );

	connect( m_modelList, SIGNAL(setSelection(const Diff2::DiffModel*, const Diff2::Difference*)),
	         this, SIGNAL(setSelection(const Diff2::DiffModel*, const Diff2::Difference*)) );
	connect( this, SIGNAL(selectionChanged(const Diff2::DiffModel*, const Diff2::Difference*)),
	         m_modelList, SLOT(slotSelectionChanged(const Diff2::DiffModel*, const Diff2::Difference*)) );

	connect( m_modelList, SIGNAL(setSelection(const Diff2::Difference*)),
	         this, SIGNAL(setSelection(const Diff2::Difference*)) );
	connect( this, SIGNAL(selectionChanged(const Diff2::Difference*)),
	         m_modelList, SLOT(slotSelectionChanged(const Diff2::Difference*)) );

	connect( m_modelList, SIGNAL(applyDifference(bool)),
	         this, SIGNAL(applyDifference(bool)) );
	connect( m_modelList, SIGNAL(applyAllDifferences(bool)),
	         this, SIGNAL(applyAllDifferences(bool)) );
	connect( m_modelList, SIGNAL(applyDifference(const Diff2::Difference*, bool)),
	         this, SIGNAL(applyDifference(const Diff2::Difference*, bool)) );

	// This creates the viewwidget and connects the signals and slots
	m_diffView = new KompareView( m_viewSettings, parentWidget, widgetName );

	connect( m_modelList, SIGNAL(setSelection(const Diff2::DiffModel*, const Diff2::Difference*)),
	         m_diffView, SLOT(slotSetSelection(const Diff2::DiffModel*, const Diff2::Difference*)) );
	connect( m_modelList, SIGNAL(setSelection(const Diff2::Difference*)),
	         m_diffView, SLOT(slotSetSelection(const Diff2::Difference*)) );
	connect( m_diffView, SIGNAL(selectionChanged(const Diff2::Difference*)),
	         m_modelList, SLOT(slotSelectionChanged(const Diff2::Difference*)) );

	connect( m_modelList, SIGNAL(applyDifference(bool)),
	         m_diffView, SLOT(slotApplyDifference(bool)) );
	connect( m_modelList, SIGNAL(applyAllDifferences(bool)),
	         m_diffView, SLOT(slotApplyAllDifferences(bool)) );
	connect( m_modelList, SIGNAL(applyDifference(const Diff2::Difference*, bool)),
	         m_diffView, SLOT(slotApplyDifference(const Diff2::Difference*, bool)) );
	connect( this, SIGNAL(configChanged()), m_diffView, SLOT(slotConfigChanged()) );

	// notify the part that this is our internal widget
	setWidget( m_diffView );

	setupActions();

	readProperties( instance()->config() );

	// set our XML-UI resource file
	setXMLFile( "komparepartui.rc" );

	// we are read-write by default -> uhm what if we are opened by lets say konq in RO mode ?
	// Then we should not be doing this...
	setReadWrite( true );

	// we are not modified since we haven't done anything yet
	setModified( false );

}

KomparePart::~KomparePart()
{
}

void KomparePart::setupActions()
{
	// create our actions

	m_save            = KStdAction::save( this, SLOT(saveDestination()), actionCollection() );
	m_saveAll         = new KAction( i18n("Save &All"), "save_all", 0,
	                                 this, SLOT(saveAll()),
	                                 actionCollection(), "file_save_all" );
	m_saveDiff        = new KAction( i18n("Save .&diff"), 0,
	                                 this, SLOT(saveDiff()),
	                                 actionCollection(), "file_save_diff" );
	m_swap            = new KAction( i18n( "Swap Source with Destination" ), 0,
	                                 this, SLOT(slotSwap()),
	                                 actionCollection(), "file_swap" );
	m_diffStats       = new KAction( i18n( "Show Statistics" ), 0,
	                                 this, SLOT(slotShowDiffstats()),
	                                 actionCollection(), "file_diffstats" );

	KStdAction::preferences(this, SLOT(optionsPreferences()), actionCollection());
}

void KomparePart::updateActions()
{
	m_saveAll->setEnabled  ( m_modelList->isModified() );
	m_saveDiff->setEnabled ( m_modelList->mode() == Kompare::ComparingFiles || m_modelList->mode() == Kompare::ComparingDirs );
	m_swap->setEnabled     ( m_modelList->mode() == Kompare::ComparingFiles || m_modelList->mode() == Kompare::ComparingDirs );
	m_diffStats->setEnabled( m_modelList->modelCount() > 0 );

	const Diff2::DiffModel* model = m_modelList->selectedModel();
	if ( model )
		m_save->setEnabled( model->isModified() );
	else
		m_save->setEnabled( false );
}

bool KomparePart::openDiff( const KURL& url )
{
	kdDebug(8103) << "Url = " << url.url() << endl;

	emit kompareInfo( &m_info );

	m_info.mode = Kompare::ShowingDiff;
	m_info.source = url;
	bool result = false;
	m_info.localSource = fetchURL( url );
	if ( !m_info.localSource.isEmpty() )
	{
		kdDebug(8103) << "Download succeeded " << endl;
		result = m_modelList->openDiff( m_info.localSource );
		updateActions();
		updateStatus();
	}
	else
	{
		kdDebug(8103) << "Download failed !" << endl;
	}

	return result;
}

bool KomparePart::openDiff( const QString& diffOutput )
{
	bool value = false;

	emit kompareInfo( &m_info );

	m_info.mode = Kompare::ShowingDiff;

	if ( m_modelList->parseDiffOutput( QStringList::split( "\n", diffOutput ) ) == 0 )
	{
		value = true;
		m_modelList->show();
		updateActions();
		updateStatus();
	}
	return value;
}

bool KomparePart::openDiff( const QStringList& diffOutput )
{
	bool value = false;

	emit kompareInfo( &m_info );

	m_info.mode = Kompare::ShowingDiff;

	if ( m_modelList->parseDiffOutput( diffOutput ) == 0 )
	{
		value = true;
		m_modelList->show();
		updateActions();
		updateStatus();
	}
	return value;
}

bool KomparePart::openDiff3( const KURL& diff3Url )
{
	// FIXME: Implement this !!!
	kdDebug(8103) << "Not implemented yet. Filename is: " << diff3Url.url() << endl;
	return false;
}

bool KomparePart::openDiff3( const QString& diff3Output )
{
	// FIXME: Implement this !!!
	kdDebug(8103) << "Not implemented yet. diff3 output is: " << endl;
	kdDebug(8103) << diff3Output << endl;
	return false;
}

bool KomparePart::exists( const QString& url )
{
	QFileInfo fi( url );
	if ( fi.exists() )
		return true;
	else
		return false;
}

const QString& KomparePart::fetchURL( const KURL& url )
{
	QString* tempFile = new QString( "" );
	if ( !url.isLocalFile() )
	{
		if ( ! KIO::NetAccess::download( url, *tempFile, widget() ) )
		{
			// FIXME: %1 should be bold
			slotShowError( i18n( "The url %1 can not be downloaded." ).arg( url.prettyURL() ) );
			*tempFile = "";
			return *tempFile;
		}
		else
			return *tempFile;
	}
	else
	{
		// is Local already, check if exists
		if ( exists( url.path() ) )
		{
			*tempFile = url.path();
			return *tempFile;
		}
		else
		{
			// FIXME: %1 should be bold
			slotShowError( i18n( "The url %1 does not exist on your system." ).arg( url.prettyURL() ) );
			return *tempFile;
		}
	}
}

void KomparePart::compare( const KURL& source, const KURL& destination )
{
	emit kompareInfo( &m_info );

	m_info.source = source;
	m_info.destination = destination;

	if ( ( m_info.localSource = fetchURL( source ) ).isEmpty() )
	{
		return;
	}

	if ( ( m_info.localDestination = fetchURL( destination ) ).isEmpty() )
	{
		// i hope a local file will not be removed if it was not downloaded...
		KIO::NetAccess::removeTempFile( m_info.localSource );
		return;
	}

	m_modelList->compare( m_info.localSource, m_info.localDestination );
}

void KomparePart::compareFiles( const KURL& sourceFile, const KURL& destinationFile )
{
	emit kompareInfo( &m_info );

	m_info.mode = Kompare::ComparingFiles;

	m_info.source = sourceFile;
	m_info.destination = destinationFile;

	m_info.localSource = fetchURL( sourceFile );
	m_info.localDestination = fetchURL( destinationFile );

	if ( !m_info.localSource.isEmpty() && !m_info.localDestination.isEmpty() )
	{
		m_modelList->compareFiles( m_info.localSource, m_info.localDestination );
		updateActions();
		updateStatus();
	}
	// Clean up needed ???
}

void KomparePart::compareDirs( const KURL& sourceDirectory, const KURL& destinationDirectory )
{
	emit kompareInfo( &m_info );

	m_info.mode = Kompare::ComparingDirs;

	m_info.source = sourceDirectory;
	m_info.destination = destinationDirectory;

	m_info.localSource = fetchURL( sourceDirectory );
	m_info.localDestination = fetchURL( destinationDirectory );

	if ( !m_info.localSource.isEmpty() && !m_info.localDestination.isEmpty() )
	{
		m_modelList->compareDirs( m_info.localSource, m_info.localDestination );
		updateActions();
		updateStatus();
	}
	// Clean up needed ???
}

void KomparePart::compare3Files( const KURL& /*originalFile*/, const KURL& /*changedFile1*/, const KURL& /*changedFile2*/ )
{
	// FIXME: actually implement this some day :)
	updateActions();
	updateStatus();
}

void KomparePart::openFileAndDiff( const KURL& file, const KURL& diffFile )
{
	emit kompareInfo( &m_info );

	if ( ( m_info.localSource = fetchURL( file ) ).isEmpty() )
		return;

	if ( ( m_info.localDestination = fetchURL( diffFile ) ).isEmpty() )
	{
		KIO::NetAccess::removeTempFile( m_info.localSource );
		return;
	}

	m_info.mode = Kompare::BlendingFile;

	if ( m_modelList->openFileAndDiff( m_info.localSource, m_info.localDestination ) )
	{
		kdDebug(8103) << "File merged with diff output..." << endl;
	}
	else
	{
		kdDebug(8103) << "Could not merge file with diff output..." << endl;
	}

	// Clean up after ourselves
	KIO::NetAccess::removeTempFile( m_info.localSource );
	KIO::NetAccess::removeTempFile( m_info.localDestination );
}

void KomparePart::openDirAndDiff ( const KURL& dir,  const KURL& diffFile )
{
	emit kompareInfo( &m_info );

	if ( ( m_info.localSource = fetchURL( dir ) ).isEmpty() )
		return;

	if ( ( m_info.localDestination = fetchURL( diffFile ) ).isEmpty() )
	{
		KIO::NetAccess::removeTempFile( m_info.localSource );
		return;
	}

	m_info.mode = Kompare::BlendingDir;

	// Still need to show the models even if something went wrong,
	// kompare will then act as if openDiff was called
	m_modelList->openDirAndDiff( m_info.localSource, m_info.localDestination );

	m_modelList->show();

	// Clean up after ourselves
	KIO::NetAccess::removeTempFile( m_info.localSource );
	KIO::NetAccess::removeTempFile( m_info.localDestination );
}

QStringList KomparePart::readFile()
{
	QStringList  lines;
	QFile file( m_file );
	file.open(  IO_ReadOnly );
	QTextStream stream( &file );

	kdDebug(8103) << "Reading from m_file = " << m_file << endl;
	while ( !stream.eof() )
	{
		lines.append( stream.readLine() );
	}

	file.close();

	return lines;
}

bool KomparePart::openFile()
{
	// This is called from openURL
	openDiff( readFile() );
	return true;
}

bool KomparePart::saveDestination()
{
	const Diff2::DiffModel* model = m_modelList->selectedModel();
	if ( model )
	{
		bool result = m_modelList->saveDestination( model );
		updateActions();
		updateStatus();
		return result;
	}
	else
	{
		return false;
	}
}

bool KomparePart::saveAll()
{
	bool result = m_modelList->saveAll();
	updateActions();
	updateStatus();
	return result;
}

void KomparePart::saveDiff()
{
	KDialogBase* dlg = new KDialogBase( widget(), "save_options",
	                                    true /* modal */, i18n("Diff Options"),
	                                    KDialogBase::Ok|KDialogBase::Cancel );
	KompareSaveOptionsWidget* w = new KompareSaveOptionsWidget(
	                                             m_info.localSource,
	                                             m_info.localDestination,
	                                             m_diffSettings, dlg );
	dlg->setMainWidget( w );
	KGuiItem saveGuiItem( i18n("Save") );
	dlg->setButtonOK( saveGuiItem );

	if( dlg->exec() ) {
		w->saveOptions();
		KConfig* config = instance()->config();
		saveProperties( config );
		config->sync();
		KURL url = KFileDialog::getSaveURL( m_info.destination.url(),
		              i18n("*.diff *.dif *.patch|Patch files"), widget(), i18n( "Save .diff" ) );
		// FIXME: if url is remote we still need to upload it
		kdDebug(8103) << "URL = " << url.prettyURL() << endl;
		kdDebug(8103) << "Directory = " << w->directory() << endl;
		kdDebug(8103) << "DiffSettings = " << m_diffSettings << endl;

		m_modelList->saveDiff( url.url(), w->directory(), m_diffSettings );
	}
	delete dlg;
}

KURL KomparePart::diffURL()
{
	// This should just call url from the ReadOnlyPart, or leave it out completely
	if( !m_info.source.isEmpty() ) {
		saveDiff(); // Why are we saving here ???
	}
	return m_info.source;
}

void KomparePart::slotSetStatus( enum Kompare::Status status )
{
	updateActions();

	switch( status ) {
	case Kompare::RunningDiff:
		emit setStatusBarText( i18n( "Running diff..." ) );
		break;
	case Kompare::Parsing:
		emit setStatusBarText( i18n( "Parsing diff output..." ) );
		break;
	case Kompare::FinishedParsing:
		updateStatus();
		break;
	case Kompare::FinishedWritingDiff:
		updateStatus();
		emit diffURLChanged();
		break;
	default:
		break;
	}
}

void KomparePart::updateStatus()
{
	QString source = m_info.source.prettyURL();
	QString destination = m_info.destination.prettyURL();

	QString text;

	switch ( m_info.mode )
	{
	case Kompare::ComparingFiles :
		text = i18n( "Comparing file %1 with file %2" )
		   .arg( source )
		   .arg( destination );
		emit setStatusBarText( text );
		emit setWindowCaption( source + ":" + destination );
		break;
	case Kompare::ComparingDirs :
		text = i18n( "Comparing files in %1 with files in %2" )
		   .arg( source )
		   .arg( destination );
		emit setStatusBarText( text );
		emit setWindowCaption( source + ":" + destination );
		break;
	case Kompare::ShowingDiff :
		emit setStatusBarText( i18n( "Viewing diff output from %1" ) .arg( source ) );
		emit setWindowCaption( source );
		break;
	case Kompare::BlendingFile :
		text = i18n( "Blending diff output from %1 into file %2" )
		    .arg( source )
		    .arg( destination );
		emit setStatusBarText( text );
		emit setWindowCaption( source + ":" + destination );
		break;
	case Kompare::BlendingDir :
		text = i18n( "Blending diff output from %1 into folder %2" )
		    .arg( m_info.source.prettyURL() )
		    .arg( m_info.destination.prettyURL() );
		emit setStatusBarText( text );
		emit setWindowCaption( source + ":" + destination );
		break;
	default:
		break;
	}
}

void KomparePart::slotShowError( QString error )
{
	KMessageBox::error( widget(), error );
}

void KomparePart::slotSwap()
{
	if ( isModified() )
	{
		int query = KMessageBox::warningYesNoCancel
		            (
		                widget(),
		                i18n( "You have made changes to the destination.\n"
		                      "Would you like to save them?" ),
		                i18n(  "Save Changes?" ),
		                i18n(  "Save" ),
		                i18n(  "Discard" )
		            );

		if ( query == KMessageBox::Yes )
			m_modelList->saveAll();
		if ( query == KMessageBox::Cancel )
			return; // Abort prematurely so no swapping
	}

	// Swap the info in the Kompare::Info struct
	KURL url = m_info.source;
	m_info.source = m_info.destination;
	m_info.destination = url;

	QString string = m_info.localSource;
	m_info.localSource = m_info.localDestination;
	m_info.localDestination = string;

	// Update window caption and statusbar text
	updateStatus();

	m_modelList->swap();
}

void KomparePart::slotShowDiffstats( void )
{
	// Fetch all the args needed for komparestatsmessagebox
	// oldfile, newfile, diffformat, noofhunks, noofdiffs

	QString oldFile;
	QString newFile;
	QString diffFormat;
	int filesInDiff;
	int noOfHunks;
	int noOfDiffs;

	oldFile = m_modelList->selectedModel() ? m_modelList->selectedModel()->sourceFile()  : QString( "" );
	newFile = m_modelList->selectedModel() ? m_modelList->selectedModel()->destinationFile() : QString( "" );

	if ( m_modelList->selectedModel() )
	{
		switch( m_info.format ) {
		case Kompare::Unified :
			diffFormat = i18n( "Unified" );
			break;
		case Kompare::Context :
			diffFormat = i18n( "Context" );
			break;
		case Kompare::RCS :
			diffFormat = i18n( "RCS" );
			break;
		case Kompare::Ed :
			diffFormat = i18n( "Ed" );
			break;
		case Kompare::Normal :
			diffFormat = i18n( "Normal" );
			break;
		case Kompare::UnknownFormat :
		default:
			diffFormat = i18n( "Unknown" );
			break;
		}
	}
	else
	{
		diffFormat = "";
	}

	filesInDiff = m_modelList->modelCount();

	noOfHunks = m_modelList->selectedModel() ? m_modelList->selectedModel()->hunkCount() : 0;
	noOfDiffs = m_modelList->selectedModel() ? m_modelList->selectedModel()->differenceCount() : 0;

	if ( m_modelList->modelCount() == 0 ) { // no diff loaded yet
		KMessageBox::information( 0L, i18n(
		    "No diff file, or no 2 files have been diffed. "
		    "Therefore no stats are available."),
		    i18n("Diff Statistics"), QString::null, false );
	}
	else if ( m_modelList->modelCount() == 1 ) { // 1 file in diff, or 2 files compared
		KMessageBox::information( 0L, i18n(
		    "Statistics:\n"
		    "\n"
		    "Old file: %1\n"
		    "New file: %2\n"
		    "\n"
		    "Format: %3\n"
		    "Number of hunks: %4\n"
		    "Number of differences: %5")
		    .arg(oldFile).arg(newFile).arg(diffFormat)
		    .arg(noOfHunks).arg(noOfDiffs),
		    i18n("Diff Statistics"), QString::null, false );
	} else { // more than 1 file in diff, or 2 directories compared
		KMessageBox::information( 0L, i18n(
		    "Statistics:\n"
		    "\n"
		    "Number of files in diff file: %1\n"
		    "Format: %2\n"
		    "\n"
		    "Current old file: %3\n"
		    "Current new file: %4\n"
		    "\n"
		    "Number of hunks: %5\n"
		    "Number of differences: %6")
		    .arg(filesInDiff).arg(diffFormat).arg(oldFile)
		    .arg(newFile).arg(noOfHunks).arg(noOfDiffs),
		    i18n("Diff Statistics"), QString::null, false );
	}
}

bool KomparePart::queryClose()
{
	if( !isModified() ) return true;

	int query = KMessageBox::warningYesNoCancel
	            (
	                widget(),
	                i18n("You have made changes to the destination.\n"
	                     "Would you like to save them?" ),
	                i18n( "Save Changes?" ),
	                i18n( "Save" ),
	                i18n( "Discard" )
	            );

	if( query == KMessageBox::Cancel ) return false;
	if( query == KMessageBox::Yes )    return m_modelList->saveAll();
	return true;
}

int KomparePart::readProperties( KConfig *config )
{
	config->setGroup( "View" );
	m_viewSettings->loadSettings( config );
	config->setGroup( "DiffSettings" );
	m_diffSettings->loadSettings   ( config );
	emit configChanged();
	return 0;
}

int KomparePart::saveProperties( KConfig *config )
{
	config->setGroup( "View" );
	m_viewSettings->saveSettings( config );
	config->setGroup( "DiffSettings" );
	m_diffSettings->saveSettings   ( config );
	return 0;
}

void KomparePart::optionsPreferences()
{
	// show preferences
	KomparePrefDlg* pref = new KomparePrefDlg( m_viewSettings, m_diffSettings );

	if ( pref->exec() ) {
		KConfig* config = instance()->config();
		saveProperties( config );
		config->sync();
		//FIXME: maybe this signal should also be emitted when
		// Apply is pressed. I'll figure it out this week.
		emit configChanged();
	}
}

void KomparePart::slotSetModified( bool modified )
{
	setModified( modified );
	updateActions();
}

// It's usually safe to leave the factory code alone.. with the
// notable exception of the KAboutData data
#include <kaboutdata.h>

KInstance*  KomparePartFactory::s_instance = 0L;
KAboutData* KomparePartFactory::s_about = 0L;

KomparePartFactory::KomparePartFactory()
    : KParts::Factory()
{
}

KomparePartFactory::~KomparePartFactory()
{
	delete s_instance;
	delete s_about;

	s_instance = 0L;
}

KParts::Part* KomparePartFactory::createPartObject( QWidget *parentWidget, const char *widgetName,
                                                  QObject *parent, const char *name,
                                                  const char *classname, const QStringList & /*args*/ )
{
	// Create an instance of our Part
	KomparePart* obj = new KomparePart( parentWidget, widgetName, parent, name );

	// See if we are to be read-write or not
	if (QCString(classname) == "KParts::ReadOnlyPart")
		obj->setReadWrite(false);

	KGlobal::locale()->insertCatalogue("kompare");

	return obj;
}

KInstance* KomparePartFactory::instance()
{
	if( !s_instance )
	{
		s_about = new KAboutData("komparepart", I18N_NOOP("KomparePart"), "3.2");
		s_about->addAuthor("John Firebaugh", "Author", "jfirebaugh@kde.org");
		s_about->addAuthor("Otto Bruggeman", "Author", "otto.bruggeman@home.nl" );
		s_instance = new KInstance(s_about);
	}
	return s_instance;
}

extern "C"
{
	void* init_libkomparepart()
	{
		return new KomparePartFactory;
	}
}

#include "kompare_part.moc"
