/***************************************************************************
                                kompareprefdlg.cpp  -  description
                                -------------------
        begin                   : Sun Mar 4 2001
        copyright               : (C) 2001-2003 by Otto Bruggeman
                                  and John Firebaugh
                                  (C) 2007      Kevin Kofler
        email                   : otto.bruggeman@home.nl
                                  jfirebaugh@kde.org
                                  kevin.kofler@chello.at
****************************************************************************/

/***************************************************************************
**
**   This program is free software; you can redistribute it and/or modify
**   it under the terms of the GNU General Public License as published by
**   the Free Software Foundation; either version 2 of the License, or
**   (at your option) any later version.
**
***************************************************************************/

#include <kdebug.h>
#include <kiconloader.h>
#include <klocale.h>

#include "diffpage.h"
#include "viewpage.h"

#include "kompareprefdlg.h"

// implementation

KomparePrefDlg::KomparePrefDlg( ViewSettings* viewSets, DiffSettings* diffSets ) : KPageDialog( 0 )
{
	setFaceType( KPageDialog::List );
	setWindowTitle( i18n( "Preferences" ) );
	setButtons( Help|Default|Ok|Apply|Cancel );
	setDefaultButton( Ok );
	setModal( true );
	showButtonSeparator( true );

	// ok i need some stuff in that pref dlg...
	//setIconListAllVisible(true);

	m_viewPage = new ViewPage();
	KPageWidgetItem *item = addPage( m_viewPage, i18n( "View" ) );
	item->setHeader( i18n( "View Settings" ) );
	m_viewPage->setSettings( viewSets );

	m_diffPage = new DiffPage();
	item = addPage( m_diffPage, i18n( "Diff" ) );
	item->setHeader( i18n( "Diff Settings" ) );
	m_diffPage->setSettings( diffSets );

//	frame = addVBoxPage( i18n( "" ), i18n( "" ), UserIcon( "" ) );

	connect( this, SIGNAL(defaultClicked()), SLOT(slotDefault()) );
	connect( this, SIGNAL(helpClicked()), SLOT(slotHelp()) );
	connect( this, SIGNAL(applyClicked()), SLOT(slotApply()) );
	connect( this, SIGNAL(okClicked()), SLOT(slotOk()) );
	connect( this, SIGNAL(cancelClicked()), SLOT(slotCancel()) );

	adjustSize();
}

KomparePrefDlg::~KomparePrefDlg()
{

}

/** No descriptions */
void KomparePrefDlg::slotDefault()
{
	kDebug(8103) << "SlotDefault called -> Settings should be restored to defaults..." << endl;
	// restore all defaults in the options...
	m_viewPage->setDefaults();
	m_diffPage->setDefaults();
}

/** No descriptions */
void KomparePrefDlg::slotHelp()
{
	// show some help...
	// figure out the current active page
	// and give help for that page
}

/** No descriptions */
void KomparePrefDlg::slotApply()
{
	kDebug(8103) << "SlotApply called -> Settings should be applied..." << endl;
	// well apply the settings that are currently selected
	m_viewPage->apply();
	m_diffPage->apply();

	emit applyClicked();
}

/** No descriptions */
void KomparePrefDlg::slotOk()
{
	kDebug(8103) << "SlotOk called -> Settings should be applied..." << endl;
	// Apply the settings that are currently selected
	m_viewPage->apply();
	m_diffPage->apply();

	//accept();
}

/** No descriptions */
void KomparePrefDlg::slotCancel()
{
	// discard the current settings and use the present ones
	m_viewPage->restore();
	m_diffPage->restore();

	//reject();
}

#include "kompareprefdlg.moc"
