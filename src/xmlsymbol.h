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

// Forward declarations
class QString;

namespace Caneda
{
    // Forward declarations
    class CGraphicsScene;
    class SymbolDocument;

    class XmlSymbol
    {
    public:
        XmlSymbol(SymbolDocument *doc = 0);
        ~XmlSymbol() {}

        bool save();
        bool load();

        SymbolDocument* symbolDocument() const;
        CGraphicsScene* cGraphicsScene() const;
        QString fileName() const;

    private:
        QString saveText();

        SymbolDocument *m_symbolDocument;
    };

} // namespace Caneda

#endif //XML_SYMBOL_H
