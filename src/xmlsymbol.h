/***************************************************************************
 * Copyright (C) 2009-2012 by Pablo Daniel Pareja Obregon                  *
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

#ifndef XML_SYMBOL_H
#define XML_SYMBOL_H

#include "component.h"

// Forward declarations
class QString;

namespace Caneda
{
    // Forward declarations
    class CGraphicsScene;
    class SymbolDocument;
    class XmlReader;

    class XmlSymbol
    {
    public:
        XmlSymbol(SymbolDocument *doc = 0);
        XmlSymbol(ComponentData *component);
        ~XmlSymbol() {}

        bool save();
        bool loadSymbol();
        bool loadComponent();

        SymbolDocument* symbolDocument() const;
        CGraphicsScene* cGraphicsScene() const;
        ComponentData* component() const;

        QString fileName() const;

    private:
        QString saveText();
        bool loadFromText(const QString& text);

        bool readComponentData(Caneda::XmlReader *reader, const QString& path);
        bool readSchematics(Caneda::XmlReader *reader, const QString& svgPath);
        void readComponentProperties(Caneda::XmlReader *reader);
        bool readSchematic(Caneda::XmlReader *reader, const QString& svgPath);
        void readSchematicPort(Caneda::XmlReader *reader, const QString & schName);
        bool readSchematicSvg(const QByteArray &svgContent, const QString &schName);

        SymbolDocument *m_symbolDocument;
        ComponentData *m_component;
        QString m_fileName;
    };

} // namespace Caneda

#endif //XML_SYMBOL_H
