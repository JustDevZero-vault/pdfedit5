/*
 * PDFedit - free program for PDF document manipulation.
 * Copyright (C) 2006-2009  PDFedit team: Michal Hocko,
 *                                        Jozef Misutka,
 *                                        Martin Petricek
 *                   Former team members: Miroslav Jahoda
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program (in doc/LICENSE.GPL); if not, write to the 
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, 
 * MA  02111-1307  USA
 *
 * Project is hosted on http://sourceforge.net/projects/pdfedit
 */
#ifndef __QTCOMPAT_H__
#define __QTCOMPAT_H__

/**
 @file
 Compatibility fixes to allow Qt3 code to work in Qt4
*/

#include <qglobal.h>

#if defined(QT_VERSION) && QT_VERSION >= 0x050000

 // QT5 or newer
#define QT5 1

#include <Qt>

/** qt3/qt4 compatibility typedef */
typedef Qt::WindowFlags WFlags;

/** Macro working in QT3 and QT4, allowing to send QString to debugging output */
#define Q_OUT(x) (x.toUtf8().data())

//Type aliases
#define Q_ButtonGroup		QButtonGroup
#define Q_ComboBox		QComboBox
#define Q_Dict			QHash
#define Q_DictIterator		Q3DictIterator
#define Q_GroupBox		Q3GroupBox
#define Q_List			QList
#define Q_ListBox		Q3ListBox
#define Q_ListBoxItem		Q3ListBoxItem
#define Q_ListBoxText		Q3ListBoxText
#define Q_ListView		Q3ListView
#define Q_ListViewItem		Q3ListViewItem
#define Q_ListViewItemIterator	Q3ListViewItemIterator
#define Q_Painter		QPainter
#define Q_PopupMenu		QMenu
#define Q_PtrCollection		Q3PtrCollection
#define Q_PtrDict		QHash
#define Q_PtrDictIterator	Q3PtrDictIterator
#define Q_PtrList		QList
#define Q_PtrListIterator	Q3PtrListIterator
#define Q_ScrollView		QScrollArea
#define Q_TextBrowser		QTextBrowser
#define Q_TextEdit		QTextEdit
#define QStrList		QList<QByteArray>
#define QMenuData		QMenu
#define QCString		QByteArray
#define QMemArray		QVector
#define IbeamCursor		IBeamCursor
#define TheWheelFocus		Qt::WheelFocus

//Include aliases
#define QBUTTONGROUP	<QtWidgets/QButtonGroup>
#define QBYTEARRAY	<QtCore/QByteArray>
#define QCOMBOBOX	<QtWidgets/QComboBox>
#define QDICT		<QtCore/QHash>
#define QGROUPBOX	<QtWidgets/QGroupBox>
#define QICON		<QtGui/QIcon>
#define QLISTBOX	<Q3ListBox>
#define QLISTVIEW	<Q3ListView>
#define QPOPUPMENU	<QMenu>
#define QPTRDICT	<QtCore/QHash>
#define QPTRLIST	<QtCore/QList>
#define QPTRCOLLECTION	<Q3PtrCollection>
#define QLIST		<QtCore/QList>
#define QSCROLLVIEW	<QtWidgets/QScrollArea>
#define QSTRLIST	<QtCore/QList>
#define QTEXTBROWSER	<QtWidgets/QTextBrowser>
#define QTEXTEDIT	<QtWidgets/QTextEdit>

#endif
