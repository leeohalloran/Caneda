/***************************************************************************
 * Copyright (C) 2010 by Pablo Daniel Pareja Obregon                       *
 *                                                                         *
 * This is free software; you can redistribute it and/or modify            *
 * it under the terms of the GNU General Public License as published by    *
 * the Free Software Foundation; either version 2, or (at your option)     *
 * any later version.                                                      *
 *                                                                         *
 * This software is distributed in the hope that it will be useful,        *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of          *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the           *
 * GNU General Public License for more details.                            *
 *                                                                         *
 * You should have received a copy of the GNU General Public License       *
 * along with this package; see the file COPYING.  If not, write to        *
 * the Free Software Foundation, Inc., 51 Franklin Street - Fifth Floor,   *
 * Boston, MA 02110-1301, USA.                                             *
 ***************************************************************************/

#include "projectfileopendialog.h"

#include "componentssidebar.h"
#include "global.h"

#include <QFileInfo>

namespace Caneda
{
    //! Constructor
    ProjectFileOpenDialog::ProjectFileOpenDialog(QString libraryFileName, QWidget *parent) :
        QDialog(parent)
    {
        ui.setupUi(this);

        //Add components browser
        m_projectsSidebar = new ComponentsSidebar(this);
        if(!libraryFileName.isEmpty()) {
            m_libraryFileName = libraryFileName;
            m_libraryName = QFileInfo(libraryFileName).baseName();
            m_libraryName.replace(0, 1, m_libraryName.left(1).toUpper()); // First letter in uppercase

            m_projectsSidebar->plugLibrary(m_libraryName, "root");
        }
        ui.layout->addWidget(m_projectsSidebar);

        connect(m_projectsSidebar, SIGNAL(itemDoubleClicked(const QString&, const QString&)), this,
                SLOT(accept()));

        m_fileName = "";
    }

    //! Destructor
    ProjectFileOpenDialog::~ProjectFileOpenDialog()
    {
    }

    void ProjectFileOpenDialog::done(int r)
    {
        if (r == QDialog::Accepted) {
            if(!m_projectsSidebar->currentComponent().isEmpty()) {
                m_fileName = QFileInfo(m_libraryFileName).absolutePath() + "/" + m_projectsSidebar->currentComponent() + ".xsch";
            }
            else {
                return;
            }
        }
        QDialog::done(r);
    }

} // namespace Caneda