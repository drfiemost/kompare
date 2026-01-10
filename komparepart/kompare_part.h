/***************************************************************************
                                kompare_part.h
                                --------------
        begin                   : Sun Mar 4 2001
        Copyright 2001-2005,2009 Otto Bruggeman <bruggie@gmail.com>
        Copyright 2001-2003 John Firebaugh <jfirebaugh@kde.org>
        Copyright 2004      Jeff Snyder    <jeff@caffeinated.me.uk>
        Copyright 2007-2011 Kevin Kofler   <kevin.kofler@chello.at>
****************************************************************************/

/***************************************************************************
**
**   This program is free software; you can redistribute it and/or modify
**   it under the terms of the GNU General Public License as published by
**   the Free Software Foundation; either version 2 of the License, or
**   (at your option) any later version.
**
***************************************************************************/

#ifndef KOMPAREPART_H
#define KOMPAREPART_H

#include <kparts/factory.h>
#include <kparts/part.h>
#include <QVariantList>
#include <kompare.h>

#include "kompareinterface.h"

class QPrinter;
class QWidget;

class KTemporaryFile;
class KUrl;
class KAboutData;
class KAction;

namespace Diff2 {
class Difference;
class DiffModel;
class DiffModelList;
class KompareModelList;
}
class DiffSettings;
class ViewSettings;
class KompareSplitter;
class KompareView;

/**
 * This is a "Part".  It does all the real work in a KPart
 * application.
 *
 * @short Main Part
 * @author John Firebaugh <jfirebaugh@kde.org>
 * @author Otto Bruggeman <bruggie@home.nl>
 * @version 0.3
 */
class KomparePart : public KParts::ReadWritePart,
                    public KompareInterface
{
	Q_OBJECT
	Q_INTERFACES(KompareInterface)
public:
	/**
	* Default constructor
	*/
	KomparePart( QWidget *parentWidget, QObject *parent, const QVariantList & /*args*/);

	/**
	* Destructor
	*/
	virtual ~KomparePart();

	// Sessionmanagement stuff, added to the kompare iface
	// because they are not in the Part class where they belong
	// Should be added when bic changes are allowed again (kde 4.0)
	virtual int readProperties( KConfig *config );
	virtual int saveProperties( KConfig *config );
	// this one is called when the shell_app is about to close.
	// we need it now to save the properties of the part when apps don't (can't)
	// use the readProperties and saveProperties methods
	virtual bool queryClose();

	// Do we really want to expose this ???
	const Diff2::KompareModelList* model() const { return m_modelList; };

	static KAboutData *createAboutData();

public:
	// Reimplemented from the KompareInterface
	/**
	 * Open and parse the diff file at diffUrl.
	 */
	bool openDiff( const KUrl& diffUrl ) override;

	/** Added on request of Harald Fernengel */
	bool openDiff( const QString& diffOutput ) override;

	/** Open and parse the diff3 file at diff3Url */
	bool openDiff3( const KUrl& diff3URL ) override;

	/** Open and parse the file diff3Output with the output of diff3 */
	bool openDiff3( const QString& diff3Output ) override;

	/** Compare, with diff, source with destination */
	void compare( const KUrl& sourceFile, const KUrl& destinationFile ) override;
	
	/** Compare a Source file to a custom Destination string */
	void compareFileString( const KUrl & sourceFile, const QString & destination) override;
	
	/** Compare a custom Source string to a Destination file */
	void compareStringFile( const QString & source, const KUrl & destinationFile) override;

	/** Compare, with diff, source with destination */
	void compareFiles( const KUrl& sourceFile, const KUrl& destinationFile ) override;

	/** Compare, with diff, source with destination */
	void compareDirs ( const KUrl& sourceDir, const KUrl& destinationDir ) override;

	/** Compare, with diff3, originalFile with changedFile1 and changedFile2 */
	void compare3Files( const KUrl& originalFile, const KUrl& changedFile1, const KUrl& changedFile2 ) override;

	/** This will show the file and the file with the diff applied */
	void openFileAndDiff( const KUrl& file, const KUrl& diffFile ) override;

	/** This will show the directory and the directory with the diff applied */
	void openDirAndDiff ( const KUrl& dir,  const KUrl& diffFile ) override;

	/** Reimplementing this because this one knows more about the real part then the interface */
	void setEncoding( const QString& encoding ) override;

	// This is the interpart interface, it is signal and slot based so no "real" interface here
	// All you have to do is connect the parts from your application.
	// These just point to their counterpart in the KompareModelList or get called from their
	// counterpart in KompareModelList.
signals:
	void modelsChanged( const Diff2::DiffModelList* models );

	void setSelection( const Diff2::DiffModel* model, const Diff2::Difference* diff );
	void setSelection( const Diff2::Difference* diff );

	void selectionChanged( const Diff2::DiffModel* model, const Diff2::Difference* diff );
	void selectionChanged( const Diff2::Difference* diff );

	void applyDifference( bool apply );
	void applyAllDifferences( bool apply );
	void applyDifference( const Diff2::Difference*, bool apply );

	void configChanged();

	/*
	** This is emitted when a difference is clicked in the kompare view. You can connect to
	** it so you can use it to jump to this particular line in the editor in your app.
	*/
	void differenceClicked( int lineNumber );

	// Stuff that can probably be removed by putting it in the part where it belongs in my opinion
public slots:
	/** Save all destinations. */
	bool saveAll();

	/** Save the results of a comparison as a diff file. */
	void saveDiff();

	/** To enable printing, the part has the only interesting printable content so putting it here */
	void slotFilePrint();
	void slotFilePrintPreview();

signals:
	void appliedChanged();
	void diffURLChanged();
	void kompareInfo( Kompare::Info* info );
	void setStatusBarModelInfo( int modelIndex, int differenceIndex, int modelCount, int differenceCount, int appliedCount );
//	void setStatusBarText( const QString& text );
	void diffString(const QString&);

protected:
	/**
	 * This is the method that gets called when the file is opened,
	 * when using openURL( const KUrl& ) or in our case also openDiff( const KUrl& );
	 * return true when everything went ok, false if there were problems
	 */
	bool openFile() override;
	// ... Uhm we return true without saving ???
	bool saveFile() override { return true; };

	// patchFile
	bool patchFile(KUrl&);
	bool patchDir();

protected slots:
	void slotSetStatus( Kompare::Status status );
	void slotShowError( QString error );

	void slotSwap();
	void slotShowDiffstats();
	void slotRefreshDiff();
	void optionsPreferences();

	void updateActions();
	void updateCaption();
	void updateStatus();
	void compareAndUpdateAll();

	void slotPaintRequested( QPrinter* );

private:
	void cleanUpTemporaryFiles();
	void setupActions();
	bool exists( const QString& url );
	bool isDirectory( const KUrl& url );
	// FIXME (like in cpp file not urgent) Replace with enum, cant find a proper 
	// name now but it is private anyway so can not be used from outside
	bool fetchURL( const KUrl& url, bool isSource );

private:
	// Uhm why were these static again ???
	// Ah yes, so multiple instances of kompare use the
	// same settings after one of them changes them
	static ViewSettings* m_viewSettings;
	static DiffSettings* m_diffSettings;

	Diff2::KompareModelList* m_modelList;

	KompareView*             m_view;
	KompareSplitter*         m_splitter;

	KAction*                 m_saveAll;
	KAction*                 m_saveDiff;
	KAction*                 m_swap;
	KAction*                 m_diffStats;
	KAction*                 m_diffRefresh;
	KAction*                 m_print;
	KAction*                 m_printPreview;

	KTemporaryFile*          m_tempDiff;

	struct Kompare::Info     m_info;
};

#endif // KOMPAREPART_H
