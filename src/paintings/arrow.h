/***************************************************************************
 * Copyright (C) 2008 by Gopala Krishna A <krishna.ggk@gmail.com>          *
 * Copyright (C) 2012 by Pablo Daniel Pareja Obregon                       *
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

#ifndef ARROW_H
#define ARROW_H

#include "painting.h"

namespace Caneda
{
    /*!
     * \brief This class is used to represent resizable arrow item on a graphics scene.
     *
     * This class supports two different styles of arrow.
     *
     * \sa HeadStyle
     */
    class Arrow : public Painting
    {
    public:
        //! \brief Represents the arrow head style.
        enum HeadStyle {
            //! This represents an ordinary arrow head style (two lines pointing in one direction)
            TwoLineArrow,
            //! This represents a filled arrow head style (filled triangle pointing in one direction)
            FilledArrow
        };

        Arrow(const QLineF &line = QLineF(), HeadStyle style = FilledArrow,
                qreal headWidth = 12, qreal headHeight = 20,
                CGraphicsScene *scene = 0);

        //! \copydoc CGraphicsItem::Type
        enum { Type = Painting::ArrowType };
        //! \copydoc CGraphicsItem::type()
        int type() const { return Type; }

        QPainterPath shapeForRect(const QRectF &rect) const;

        void paint(QPainter *, const QStyleOptionGraphicsItem*, QWidget *);

        Arrow* copy(CGraphicsScene *scene = 0) const;

        void saveData(Caneda::XmlWriter *writer) const;
        void loadData(Caneda::XmlReader *reader);

        HeadStyle headStyle() const { return m_headStyle; }
        void setHeadStyle(HeadStyle style);

        //! Returns the base triangle width of arrow head.
        qreal headWidth() const { return m_headWidth; }
        void setHeadWidth(qreal width);

        //! Returns the triangle height of arrow head.
        qreal headHeight() const { return m_headHeight; }
        void setHeadHeight(qreal width);

        //! Returns the line of the arrow.
        QLineF line() const { return lineFromRect(paintingRect()); }
        void setLine(const QLineF &line);

        int launchPropertyDialog(Caneda::UndoOption opt);

    protected:
        void geometryChange();

    private:
        void calcHeadPoints();
        QLineF lineFromRect(const QRectF &rect) const;
        void drawHead(QPainter *painter);

        HeadStyle m_headStyle;
        qreal m_headWidth;
        qreal m_headHeight;

        //the head's tip is always at index 1
        QPolygonF m_head;
    };

} // namespace Caneda

#endif //ARROW_H
