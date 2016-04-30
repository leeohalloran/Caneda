/***************************************************************************
 * Copyright (C) 2015-2016 by Pablo Daniel Pareja Obregon                  *
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

#ifndef SIMULATIONDIALOG_H
#define SIMULATIONDIALOG_H

#include "ui_simulationdialog.h"

namespace Caneda
{
    // Forward declations
    class ChartView;

    /*!
     * \brief Dialog to select simulation properties in a ChartView plot.
     *
     * This dialog presents to the user the properties of the selected
     * simulation plot (ChartView) and the visible waveforms.
     *
     * This class handles the user interface part of the dialog, and
     * presentation part to the user, while SimulationModel class handles
     * the data interaction itself.
     *
     * \sa ChartView, WaveformsMap, SimulationModel, QSortFilterProxyModel
     */
    class SimulationDialog : public QDialog
    {
        Q_OBJECT

    public:
        SimulationDialog(ChartView *parent = 0);

    public Q_SLOTS:
        void accept();

    private:
        Ui::SimulationDialog ui;
    };

} // namespace Caneda

#endif //SIMULATIONDIALOG_H
