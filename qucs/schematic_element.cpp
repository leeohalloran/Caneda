/***************************************************************************
                            schematic_element.cpp
                           -----------------------
    begin                : Sat Mar 3 2006
    copyright            : (C) 2006 by Michael Margraf
    email                : michael.margraf@alumni.tu-berlin.de
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <stdlib.h>
#include <limits.h>

#include "schematic.h"
#include "node.h"
#include "wire.h"
#include "diagrams/diagram.h"
#include "paintings/painting.h"
#include "components/component.h"



/* *******************************************************************
   *****                                                         *****
   *****              Actions handling the nodes                 *****
   *****                                                         *****
   ******************************************************************* */

// Inserts a port into the schematic and connects it to another node if
// the coordinates are identical. The node is returned.
Node* Schematic::insertNode(int x, int y, Element *e)
{
  Node *pn;
  // check if new node lies upon existing node
  for(pn = Nodes->first(); pn != 0; pn = Nodes->next())  // check every node
    if(pn->cx == x) if(pn->cy == y) {
      pn->Connections.append(e);
      break;
    }

  if(pn == 0) { // create new node, if no existing one lies at this position
    pn = new Node(x, y);
    Nodes->append(pn);
    pn->Connections.append(e);  // connect schematic node to component node
  }
  else return pn;   // return, if node is not new

  // check if the new node lies upon an existing wire
  for(Wire *pw = Wires->first(); pw != 0; pw = Wires->next()) {
    if(pw->x1 == x) {
      if(pw->y1 > y) continue;
      if(pw->y2 < y) continue;
    }
    else if(pw->y1 == y) {
      if(pw->x1 > x) continue;
      if(pw->x2 < x) continue;
    }
    else continue;

    // split the wire into two wires
    splitWire(pw, pn);
    return pn;
  }

  return pn;
}

// ---------------------------------------------------
Node* Schematic::selectedNode(int x, int y)
{
  for(Node *pn = Nodes->first(); pn != 0; pn = Nodes->next()) // test nodes
    if(pn->getSelected(x, y))
      return pn;

  return 0;
}


/* *******************************************************************
   *****                                                         *****
   *****              Actions handling the wires                 *****
   *****                                                         *****
   ******************************************************************* */

// Inserts a port into the schematic and connects it to another node if the
// coordinates are identical. If 0 is returned, no new wire is inserted.
// If 2 is returned, the wire line ended.
int Schematic::insertWireNode1(Wire *w)
{
  Node *pn;
  // check if new node lies upon an existing node
  for(pn = Nodes->first(); pn != 0; pn = Nodes->next()) // check every node
    if(pn->cx == w->x1) if(pn->cy == w->y1) break;

  if(pn != 0) {
    pn->Connections.append(w);
    w->Port1 = pn;
    return 2;   // node is not new
  }



  // check if the new node lies upon an existing wire
  for(Wire *ptr2 = Wires->first(); ptr2 != 0; ptr2 = Wires->next()) {
    if(ptr2->x1 == w->x1) {
      if(ptr2->y1 > w->y1) continue;
      if(ptr2->y2 < w->y1) continue;

      if(ptr2->isHorizontal() == w->isHorizontal()) // (ptr2-wire is vertical)
        if(ptr2->y2 >= w->y2) {
	  delete w;    // new wire lies within an existing wire
	  return 0; }
        else {
	  // one part of the wire lies within an existing wire
	  // the other part not
          if(ptr2->Port2->Connections.count() == 1) {
            w->y1 = ptr2->y1;
            w->Port1 = ptr2->Port1;
	    if(ptr2->Label) {
	      w->Label = ptr2->Label;
	      w->Label->pOwner = w;
	    }
            ptr2->Port1->Connections.removeRef(ptr2);  // two -> one wire
            ptr2->Port1->Connections.append(w);
            Nodes->removeRef(ptr2->Port2);
            Wires->removeRef(ptr2);
            return 2;
          }
          else {
            w->y1 = ptr2->y2;
            w->Port1 = ptr2->Port2;
            ptr2->Port2->Connections.append(w);   // shorten new wire
            return 2;
          }
        }
    }
    else if(ptr2->y1 == w->y1) {
      if(ptr2->x1 > w->x1) continue;
      if(ptr2->x2 < w->x1) continue;

      if(ptr2->isHorizontal() == w->isHorizontal()) // (ptr2-wire is horizontal)
        if(ptr2->x2 >= w->x2) {
          delete w;   // new wire lies within an existing wire
          return 0;
        }
        else {
	  // one part of the wire lies within an existing wire
	  // the other part not
          if(ptr2->Port2->Connections.count() == 1) {
            w->x1 = ptr2->x1;
            w->Port1 = ptr2->Port1;
	    if(ptr2->Label) {
	      w->Label = ptr2->Label;
	      w->Label->pOwner = w;
	    }
            ptr2->Port1->Connections.removeRef(ptr2); // two -> one wire
            ptr2->Port1->Connections.append(w);
            Nodes->removeRef(ptr2->Port2);
            Wires->removeRef(ptr2);
            return 2;
          }
          else {
            w->x1 = ptr2->x2;
            w->Port1 = ptr2->Port2;
            ptr2->Port2->Connections.append(w);   // shorten new wire
            return 2;
          }
        }
    }
    else continue;

    pn = new Node(w->x1, w->y1);   // create new node
    Nodes->append(pn);
    pn->Connections.append(w);  // connect schematic node to the new wire
    w->Port1 = pn;

    // split the wire into two wires
    splitWire(ptr2, pn);
    return 2;
  }

  pn = new Node(w->x1, w->y1);   // create new node
  Nodes->append(pn);
  pn->Connections.append(w);  // connect schematic node to the new wire
  w->Port1 = pn;
  return 1;
}

// ---------------------------------------------------
// if possible, connect two horizontal wires to one
bool Schematic::connectHWires1(Wire *w)
{
  Wire *pw;
  Node *n = w->Port1;

  pw = (Wire*)n->Connections.last();  // last connection is the new wire itself
  for(pw = (Wire*)n->Connections.prev(); pw!=0; pw = (Wire*)n->Connections.prev()) {
    if(pw->Type != isWire) continue;
    if(!pw->isHorizontal()) continue;
    if(pw->x1 < w->x1) {
      if(n->Connections.count() != 2) continue;
      if(pw->Label) {
        w->Label = pw->Label;
	w->Label->pOwner = w;
      }
      else if(n->Label) {
	     w->Label = n->Label;
	     w->Label->pOwner = w;
	     w->Label->Type = isHWireLabel;
	   }
      w->x1 = pw->x1;
      w->Port1 = pw->Port1;      // new wire lengthens an existing one
      Nodes->removeRef(n);
      w->Port1->Connections.removeRef(pw);
      w->Port1->Connections.append(w);
      Wires->removeRef(pw);
      return true;
    }
    if(pw->x2 >= w->x2) {  // new wire lies within an existing one ?
      w->Port1->Connections.removeRef(w); // second node not yet made
      delete w;
      return false;
    }
    if(pw->Port2->Connections.count() < 2) {
      // existing wire lies within the new one
      if(pw->Label) {
        w->Label = pw->Label;
	w->Label->pOwner = w;
      }
      pw->Port1->Connections.removeRef(pw);
      Nodes->removeRef(pw->Port2);
      Wires->removeRef(pw);
      return true;
    }
    w->x1 = pw->x2;    // shorten new wire according to an existing one
    w->Port1->Connections.removeRef(w);
    w->Port1 = pw->Port2;
    w->Port1->Connections.append(w);
    return true;
  }

  return true;
}

// ---------------------------------------------------
// if possible, connect two vertical wires to one
bool Schematic::connectVWires1(Wire *w)
{
  Wire *pw;
  Node *n = w->Port1;

  pw = (Wire*)n->Connections.last();  // last connection is the new wire itself
  for(pw = (Wire*)n->Connections.prev(); pw!=0; pw = (Wire*)n->Connections.prev()) {
    if(pw->Type != isWire) continue;
    if(pw->isHorizontal()) continue;
    if(pw->y1 < w->y1) {
      if(n->Connections.count() != 2) continue;
      if(pw->Label) {
        w->Label = pw->Label;
	w->Label->pOwner = w;
      }
      else if(n->Label) {
	     w->Label = n->Label;
	     w->Label->pOwner = w;
	     w->Label->Type = isVWireLabel;
	   }
     w->y1 = pw->y1;
      w->Port1 = pw->Port1;         // new wire lengthens an existing one
      Nodes->removeRef(n);
      w->Port1->Connections.removeRef(pw);
      w->Port1->Connections.append(w);
      Wires->removeRef(pw);
      return true;
    }
    if(pw->y2 >= w->y2) {  // new wire lies complete within an existing one ?
      w->Port1->Connections.removeRef(w); // second node not yet made
      delete w;
      return false;
    }
    if(pw->Port2->Connections.count() < 2) {
      // existing wire lies within the new one
      if(pw->Label) {
        w->Label = pw->Label;
	w->Label->pOwner = w;
      }
      pw->Port1->Connections.removeRef(pw);
      Nodes->removeRef(pw->Port2);
      Wires->removeRef(pw);
      return true;
    }
    w->y1 = pw->y2;    // shorten new wire according to an existing one
    w->Port1->Connections.removeRef(w);
    w->Port1 = pw->Port2;
    w->Port1->Connections.append(w);
    return true;
  }

  return true;
}

// ---------------------------------------------------
// Inserts a port into the schematic and connects it to another node if the
// coordinates are identical. If 0 is returned, no new wire is inserted.
// If 2 is returned, the wire line ended.
int Schematic::insertWireNode2(Wire *w)
{
  Node *pn;
  // check if new node lies upon an existing node
  for(pn = Nodes->first(); pn != 0; pn = Nodes->next())  // check every node
    if(pn->cx == w->x2) if(pn->cy == w->y2) break;

  if(pn != 0) {
    pn->Connections.append(w);
    w->Port2 = pn;
    return 2;   // node is not new
  }



  // check if the new node lies upon an existing wire
  for(Wire *ptr2 = Wires->first(); ptr2 != 0; ptr2 = Wires->next()) {
    if(ptr2->x1 == w->x2) {
      if(ptr2->y1 > w->y2) continue;
      if(ptr2->y2 < w->y2) continue;

    // (if new wire lies within an existing wire, was already check before)
      if(ptr2->isHorizontal() == w->isHorizontal()) // ptr2-wire is vertical
          // one part of the wire lies within an existing wire
          // the other part not
          if(ptr2->Port1->Connections.count() == 1) {
	    if(ptr2->Label) {
	      w->Label = ptr2->Label;
	      w->Label->pOwner = w;
	    }
            w->y2 = ptr2->y2;
            w->Port2 = ptr2->Port2;
            ptr2->Port2->Connections.removeRef(ptr2);  // two -> one wire
            ptr2->Port2->Connections.append(w);
            Nodes->removeRef(ptr2->Port1);
            Wires->removeRef(ptr2);
            return 2;
          }
          else {
            w->y2 = ptr2->y1;
            w->Port2 = ptr2->Port1;
            ptr2->Port1->Connections.append(w);   // shorten new wire
            return 2;
          }
    }
    else if(ptr2->y1 == w->y2) {
      if(ptr2->x1 > w->x2) continue;
      if(ptr2->x2 < w->x2) continue;

    // (if new wire lies within an existing wire, was already check before)
      if(ptr2->isHorizontal() == w->isHorizontal()) // ptr2-wire is horizontal
          // one part of the wire lies within an existing wire
          // the other part not
          if(ptr2->Port1->Connections.count() == 1) {
	    if(ptr2->Label) {
	      w->Label = ptr2->Label;
	      w->Label->pOwner = w;
	    }
            w->x2 = ptr2->x2;
            w->Port2 = ptr2->Port2;
            ptr2->Port2->Connections.removeRef(ptr2);  // two -> one wire
            ptr2->Port2->Connections.append(w);
            Nodes->removeRef(ptr2->Port1);
            Wires->removeRef(ptr2);
            return 2;
          }
          else {
            w->x2 = ptr2->x1;
            w->Port2 = ptr2->Port1;
            ptr2->Port1->Connections.append(w);   // shorten new wire
            return 2;
          }
    }
    else continue;

    pn = new Node(w->x2, w->y2);   // create new node
    Nodes->append(pn);
    pn->Connections.append(w);  // connect schematic node to the new wire
    w->Port2 = pn;

    // split the wire into two wires
    splitWire(ptr2, pn);
    return 2;
  }

  pn = new Node(w->x2, w->y2);   // create new node
  Nodes->append(pn);
  pn->Connections.append(w);  // connect schematic node to the new wire
  w->Port2 = pn;
  return 1;
}

// ---------------------------------------------------
// if possible, connect two horizontal wires to one
bool Schematic::connectHWires2(Wire *w)
{
  Wire *pw;
  Node *n = w->Port2;

  pw = (Wire*)n->Connections.last(); // last connection is the new wire itself
  for(pw = (Wire*)n->Connections.prev(); pw!=0; pw = (Wire*)n->Connections.prev()) {
    if(pw->Type != isWire) continue;
    if(!pw->isHorizontal()) continue;
    if(pw->x2 > w->x2) {
      if(n->Connections.count() != 2) continue;
      if(pw->Label) {
        w->Label = pw->Label;
	w->Label->pOwner = w;
      }
      w->x2 = pw->x2;
      w->Port2 = pw->Port2;      // new wire lengthens an existing one
      Nodes->removeRef(n);
      w->Port2->Connections.removeRef(pw);
      w->Port2->Connections.append(w);
      Wires->removeRef(pw);
      return true;
    }
    // (if new wire lies complete within an existing one, was already
    // checked before)

    if(pw->Port1->Connections.count() < 2) {
      // existing wire lies within the new one
      if(pw->Label) {
        w->Label = pw->Label;
	w->Label->pOwner = w;
      }
      pw->Port2->Connections.removeRef(pw);
      Nodes->removeRef(pw->Port1);
      Wires->removeRef(pw);
      return true;
    }
    w->x2 = pw->x1;    // shorten new wire according to an existing one
    w->Port2->Connections.removeRef(w);
    w->Port2 = pw->Port1;
    w->Port2->Connections.append(w);
    return true;
  }

  return true;
}

// ---------------------------------------------------
// if possible, connect two vertical wires to one
bool Schematic::connectVWires2(Wire *w)
{
  Wire *pw;
  Node *n = w->Port2;

  pw = (Wire*)n->Connections.last(); // last connection is the new wire itself
  for(pw = (Wire*)n->Connections.prev(); pw!=0; pw = (Wire*)n->Connections.prev()) {
    if(pw->Type != isWire) continue;
    if(pw->isHorizontal()) continue;
    if(pw->y2 > w->y2) {
      if(n->Connections.count() != 2) continue;
      if(pw->Label) {
        w->Label = pw->Label;
	w->Label->pOwner = w;
      }
      w->y2 = pw->y2;
      w->Port2 = pw->Port2;     // new wire lengthens an existing one
      Nodes->removeRef(n);
      w->Port2->Connections.removeRef(pw);
      w->Port2->Connections.append(w);
      Wires->removeRef(pw);
      return true;
    }
    // (if new wire lies complete within an existing one, was already
    // checked before)

    if(pw->Port1->Connections.count() < 2) {
      // existing wire lies within the new one
      if(pw->Label) {
        w->Label = pw->Label;
	w->Label->pOwner = w;
      }
      pw->Port2->Connections.removeRef(pw);
      Nodes->removeRef(pw->Port1);
      Wires->removeRef(pw);
      return true;
    }
    w->y2 = pw->y1;    // shorten new wire according to an existing one
    w->Port2->Connections.removeRef(w);
    w->Port2 = pw->Port1;
    w->Port2->Connections.append(w);
    return true;
  }

  return true;
}

// ---------------------------------------------------
// Inserts a vertical or horizontal wire into the schematic and connects
// the ports that hit together. Returns whether the beginning and ending
// (the ports of the wire) are connected or not.
int Schematic::insertWire(Wire *w)
{
  int  tmp, con = 0;
  bool ok;

  // change coordinates if necessary (port 1 coordinates must be less
  // port 2 coordinates)
  if(w->x1 > w->x2) { tmp = w->x1; w->x1 = w->x2; w->x2 = tmp; }
  else
  if(w->y1 > w->y2) { tmp = w->y1; w->y1 = w->y2; w->y2 = tmp; }
  else con = 0x100;



  tmp = insertWireNode1(w);
  if(tmp == 0) return 3;  // no new wire and no open connection
  if(tmp > 1) con |= 2;   // insert node and remember if it remains open

  if(w->isHorizontal()) ok = connectHWires1(w);
  else ok = connectVWires1(w);
  if(!ok) return 3;



  
  tmp = insertWireNode2(w);
  if(tmp == 0) return 3;  // no new wire and no open connection
  if(tmp > 1) con |= 1;   // insert node and remember if it remains open

  if(w->isHorizontal()) ok = connectHWires2(w);
  else ok = connectVWires2(w);
  if(!ok) return 3;

  

  // change node 1 and 2
  if(con > 255) con = ((con >> 1) & 1) | ((con << 1) & 2);

  Wires->append(w);    // add wire to the schematic




  int  n1, n2;
  Wire *pw, *nw;
  Node *pn, *pn2;
  Element *pe;
  // ................................................................
  // Check if the new line covers existing nodes.
  // In order to also check new appearing wires -> use "for"-loop
  for(pw = Wires->current(); pw != 0; pw = Wires->next())
    for(pn = Nodes->first(); pn != 0; ) {  // check every node
      if(pn->cx == pw->x1) {
        if(pn->cy <= pw->y1) { pn = Nodes->next(); continue; }
        if(pn->cy >= pw->y2) { pn = Nodes->next(); continue; }
      }
      else if(pn->cy == pw->y1) {
        if(pn->cx <= pw->x1) { pn = Nodes->next(); continue; }
        if(pn->cx >= pw->x2) { pn = Nodes->next(); continue; }
      }
      else { pn = Nodes->next(); continue; }

      n1 = 2; n2 = 3;
      pn2 = pn;
      // check all connections of the current node
      for(pe=pn->Connections.first(); pe!=0; pe=pn->Connections.next()) {
        if(pe->Type != isWire) continue;
        nw = (Wire*)pe;
	// wire lies within the new ?
	if(pw->isHorizontal() != nw->isHorizontal()) continue;

        pn  = nw->Port1;
        pn2 = nw->Port2;
        n1  = pn->Connections.count();
        n2  = pn2->Connections.count();
        if(n1 == 1) {
          Nodes->removeRef(pn);     // delete node 1 if open
          pn2->Connections.removeRef(nw);   // remove connection
          pn = pn2;
        }

        if(n2 == 1) {
          pn->Connections.removeRef(nw);   // remove connection
          Nodes->removeRef(pn2);     // delete node 2 if open
          pn2 = pn;
        }

        if(pn == pn2) {
	  if(nw->Label) {
	    pw->Label = nw->Label;
	    pw->Label->pOwner = pw;
	  }
          Wires->removeRef(nw);    // delete wire
          Wires->findRef(pw);      // set back to current wire
        }
        break;
      }
      if(n1 == 1) if(n2 == 1) continue;

      // split wire into two wires
      if((pw->x1 != pn->cx) || (pw->y1 != pn->cy)) {
        nw = new Wire(pw->x1, pw->y1, pn->cx, pn->cy, pw->Port1, pn);
        pn->Connections.append(nw);
        Wires->append(nw);
        Wires->findRef(pw);
        pw->Port1->Connections.append(nw);
      }
      pw->Port1->Connections.removeRef(pw);
      pw->x1 = pn2->cx;
      pw->y1 = pn2->cy;
      pw->Port1 = pn2;
      pn2->Connections.append(pw);

      pn = Nodes->next();
    }

  if (Wires->containsRef (w))  // if two wire lines with different labels ...
    oneLabel(w->Port1);       // ... are connected, delete one label
  return con | 0x0200;   // sent also end flag
}

// ---------------------------------------------------
// Follows a wire line and selects it.
void Schematic::selectWireLine(Element *pe, Node *pn, bool ctrl)
{
  Node *pn_1st = pn;
  while(pn->Connections.count() == 2) {
    if(pn->Connections.first() == pe)  pe = pn->Connections.last();
    else  pe = pn->Connections.first();

    if(pe->Type != isWire) break;
    if(ctrl) pe->isSelected ^= ctrl;
    else pe->isSelected = true;

    if(((Wire*)pe)->Port1 == pn)  pn = ((Wire*)pe)->Port2;
    else  pn = ((Wire*)pe)->Port1;
    if(pn == pn_1st) break;  // avoid endless loop in wire loops
  }
}

// ---------------------------------------------------
Wire* Schematic::selectedWire(int x, int y)
{
  for(Wire *pw = Wires->first(); pw != 0; pw = Wires->next())
    if(pw->getSelected(x, y))
      return pw;

  return 0;
}

// ---------------------------------------------------
// Splits the wire "*pw" into two pieces by the node "*pn".
Wire* Schematic::splitWire(Wire *pw, Node *pn)
{
  Wire *newWire = new Wire(pn->cx, pn->cy, pw->x2, pw->y2, pn, pw->Port2);
  newWire->isSelected = pw->isSelected;

  pw->x2 = pn->cx;
  pw->y2 = pn->cy;
  pw->Port2 = pn;

  newWire->Port2->Connections.prepend(newWire);
  pn->Connections.prepend(pw);
  pn->Connections.prepend(newWire);
  newWire->Port2->Connections.removeRef(pw);
  Wires->append(newWire);

  if(pw->Label)
    if((pw->Label->cx > pn->cx) || (pw->Label->cy > pn->cy)) {
      newWire->Label = pw->Label;   // label goes to the new wire
      pw->Label = 0;
      newWire->Label->pOwner = newWire;
    }

  return newWire;
}

// ---------------------------------------------------
// If possible, make one wire out of two wires.
bool Schematic::oneTwoWires(Node *n)
{
  Wire *e3;
  Wire *e1 = (Wire*)n->Connections.getFirst();  // two wires -> one wire
  Wire *e2 = (Wire*)n->Connections.getLast();

  if(e1->Type == isWire) if(e2->Type == isWire)
    if(e1->isHorizontal() == e2->isHorizontal()) {
      if(e1->x1 == e2->x2) if(e1->y1 == e2->y2) {
        e3 = e1; e1 = e2; e2 = e3;    // e1 must have lesser coordinates
      }
      if(e2->Label) {   // take over the node name label ?
        e1->Label = e2->Label;
	e1->Label->pOwner = e1;
      }
      else if(n->Label) {
             e1->Label = n->Label;
	     e1->Label->pOwner = e1;
	     if(e1->isHorizontal())
	       e1->Label->Type = isHWireLabel;
	     else
	       e1->Label->Type = isVWireLabel;
	   }

      e1->x2 = e2->x2;
      e1->y2 = e2->y2;
      e1->Port2 = e2->Port2;
      Nodes->removeRef(n);    // delete node (is auto delete)
      e1->Port2->Connections.removeRef(e2);
      e1->Port2->Connections.append(e1);
      Wires->removeRef(e2);
      return true;
    }
  return false;
}

// ---------------------------------------------------
// Deletes the wire 'w'.
void Schematic::deleteWire(Wire *w)
{
  if(w->Port1->Connections.count() == 1) {
    if(w->Port1->Label) delete w->Port1->Label;
    Nodes->removeRef(w->Port1);     // delete node 1 if open
  }
  else {
    w->Port1->Connections.removeRef(w);   // remove connection
    if(w->Port1->Connections.count() == 2)
      oneTwoWires(w->Port1);  // two wires -> one wire
  }

  if(w->Port2->Connections.count() == 1) {
    if(w->Port2->Label) delete w->Port2->Label;
    Nodes->removeRef(w->Port2);     // delete node 2 if open
  }
  else {
    w->Port2->Connections.removeRef(w);   // remove connection
    if(w->Port2->Connections.count() == 2)
      oneTwoWires(w->Port2);  // two wires -> one wire
  }

  if(w->Label) delete w->Label;
  Wires->removeRef(w);
}

// ---------------------------------------------------
void Schematic::copyWires(int& x1, int& y1, int& x2, int& y2,
			QPtrList<Element> *ElementCache)
{
  Wire *pw;
  WireLabel *pl;
  for(pw = Wires->first(); pw != 0; )  // find bounds of all selected wires
    if(pw->isSelected) {
      if(pw->x1 < x1) x1 = pw->x1;
      if(pw->x2 > x2) x2 = pw->x2;
      if(pw->y1 < y1) y1 = pw->y1;
      if(pw->y2 > y2) y2 = pw->y2;

      ElementCache->append(pw);

      Node *pn;   // rescue non-selected node labels
      pn = pw->Port1;
      if(pn->Label)
        if(pn->Connections.count() < 2) {
          ElementCache->append(pn->Label);
          pn->Label = 0;
          // Don't set Label->pOwner=0 , so text position stays unchanged.
        }
      pn = pw->Port2;
      if(pn->Label)
        if(pn->Connections.count() < 2) {
          ElementCache->append(pn->Label);
          pn->Label = 0;
          // Don't set pn->Label->pOwner=0 , so text position stays unchanged.
        }

      pl = pw->Label;
      pw->Label = 0;
      deleteWire(pw);
      pw->Label = pl;    // restore wire label
      pw = Wires->current();
    }
    else pw = Wires->next();
}


/* *******************************************************************
   *****                                                         *****
   *****                  Actions with markers                   *****
   *****                                                         *****
   ******************************************************************* */

Marker* Schematic::setMarker(int x, int y)
{
  int n;
  // test all diagrams
  for(Diagram *pd = Diagrams->last(); pd != 0; pd = Diagrams->prev())
    if(pd->getSelected(x, y)) {

      // test all graphs of the diagram
      for(Graph *pg = pd->Graphs.first(); pg != 0; pg = pd->Graphs.next()) {
	n  = pg->getSelected(x-pd->cx, pd->cy-y);
	if(n >= 0) {
	  Marker *pm = new Marker(pd, pg, n, x-pd->cx, y-pd->cy);
	  pg->Markers.append(pm);
	  setChanged(true, true);
	  return pm;
	}
      }
    }

  return 0;
}

// ---------------------------------------------------
// Moves the marker pointer left/right on the graph.
void Schematic::markerLeftRight(bool left, QPtrList<Element> *Elements)
{
  Marker *pm;
  bool acted = false;
  for(pm = (Marker*)Elements->first(); pm!=0; pm = (Marker*)Elements->next()) {
    pm->pGraph->Markers.append(pm);
    if(pm->moveLeftRight(left))
      acted = true;
  }

  if(acted)  setChanged(true, true, 'm');
}

// ---------------------------------------------------
// Moves the marker pointer up/down on the more-dimensional graph.
void Schematic::markerUpDown(bool up, QPtrList<Element> *Elements)
{
  Marker *pm;
  bool acted = false;
  for(pm = (Marker*)Elements->first(); pm!=0; pm = (Marker*)Elements->next()) {
    pm->pGraph->Markers.append(pm);
    if(pm->moveUpDown(up))
      acted = true;
  }

  if(acted)  setChanged(true, true, 'm');
}


/* *******************************************************************
   *****                                                         *****
   *****               Actions with all elements                 *****
   *****                                                         *****
   ******************************************************************* */

// Selects the element that contains the coordinates x/y.
// Returns the pointer to the element.
// If 'flag' is true, the element can be deselected.
Element* Schematic::selectElement(int x, int y, bool flag, int *index)
{
  int n;
  Element *pe_1st=0, *pe_sel=0;
  float Corr = textCorr(); // for selecting text

  // test all components
  for(Component *pc = Components->last(); pc != 0; pc = Components->prev())
    if(pc->getSelected(x, y)) {
      if(flag) { pc->isSelected ^= flag; return pc; }
      if(pe_sel) {
	pe_sel->isSelected = false;
	return pc;
      }
      if(pe_1st == 0) pe_1st = pc;  // give access to elements lying beneath
      if(pc->isSelected) pe_sel = pc;
    }
    else {
      n = pc->getTextSelected(x, y, Corr);
      if(n >= 0) {   // was property text clicked ?
        pc->Type = isComponentText;
        if(index)  *index = n;
        return pc;
      }
    }

  WireLabel *pl;
  // test all wires and wire labels
  for(Wire *pw = Wires->last(); pw != 0; pw = Wires->prev()) {
    if(pw->getSelected(x, y)) {
      if(flag) { pw->isSelected ^= flag; return pw; }
      if(pe_sel) {
	pe_sel->isSelected = false;
	return pw;
      }
      if(pe_1st == 0) pe_1st = pw;  // give access to elements lying beneath
      if(pw->isSelected) pe_sel = pw;
    }
    pl = pw->Label;
    if(pl) if(pl->getSelected(x, y)) {
      if(flag) { pl->isSelected ^= flag; return pl; }
      if(pe_sel) {
	pe_sel->isSelected = false;
	return pl;
      }
      if(pe_1st == 0) pe_1st = pl;  // give access to elements lying beneath
      if(pl->isSelected) pe_sel = pl;
    }
  }

  // test all node labels
  for(Node *pn = Nodes->last(); pn != 0; pn = Nodes->prev()) {
    pl = pn->Label;
    if(pl) if(pl->getSelected(x, y)) {
      if(flag) { pl->isSelected ^= flag; return pl; }
      if(pe_sel) {
	pe_sel->isSelected = false;
	return pl;
      }
      if(pe_1st == 0) pe_1st = pl;  // give access to elements lying beneath
      if(pl->isSelected) pe_sel = pl;
    }
  }

  Graph *pg;
  // test all diagrams
  for(Diagram *pd = Diagrams->last(); pd != 0; pd = Diagrams->prev()) {

    for(pg = pd->Graphs.first(); pg != 0; pg = pd->Graphs.next())
      // test markers of graphs
      for(Marker *pm = pg->Markers.first(); pm != 0; pm = pg->Markers.next())
        if(pm->getSelected(x-pd->cx, y-pd->cy)) {
          if(flag) { pm->isSelected ^= flag; return pm; }
          if(pe_sel) {
	    pe_sel->isSelected = false;
	    return pm;
	  }
          if(pe_1st == 0) pe_1st = pm; // give access to elements beneath
          if(pm->isSelected) pe_sel = pm;
        }

    // resize area clicked ?
    if(pd->isSelected)
      if(pd->ResizeTouched(x, y))
	if(pe_1st == 0) {
	  pd->Type = isDiagramResize;
	  return pd;
        }

    if(pd->getSelected(x, y)) {
      if(pd->Name[0] == 'T')    // tabular, timing diagram or truth table ?
        if(pd->Name[1] == 'i') {
          if(y > pd->cy) {
            if(x < pd->cx+pd->xAxis.numGraphs) continue;
            pd->Type = isDiagramScroll;
            return pd;
          }
        }
        else {
          if(x < pd->cx) {      // clicked on scroll bar ?
            pd->Type = isDiagramScroll;
            return pd;
          }
        }

      // test graphs of diagram
      for(pg = pd->Graphs.first(); pg != 0; pg = pd->Graphs.next())
        if(pg->getSelected(x-pd->cx, pd->cy-y) >= 0) {
          if(flag) { pg->isSelected ^= flag; return pg; }
          if(pe_sel) {
	    pe_sel->isSelected = false;
	    return pg;
	  }
          if(pe_1st == 0) pe_1st = pg;  // access to elements lying beneath
          if(pg->isSelected) pe_sel = pg;
        }


      if(flag) { pd->isSelected ^= flag; return pd; }
      if(pe_sel) {
	pe_sel->isSelected = false;
	return pd;
      }
      if(pe_1st == 0) pe_1st = pd;  // give access to elements lying beneath
      if(pd->isSelected) pe_sel = pd;
    }
  }

  // test all paintings
  for(Painting *pp = Paintings->last(); pp != 0; pp = Paintings->prev()) {
    if(pp->isSelected)
      if(pp->ResizeTouched(x, y))
	if(pe_1st == 0) {
	  pp->Type = isPaintingResize;
	  return pp;
        }

    if(pp->getSelected(x, y)) {
      if(flag) { pp->isSelected ^= flag; return pp; }
      if(pe_sel) {
	pe_sel->isSelected = false;
	return pp;
      }
      if(pe_1st == 0) pe_1st = pp;  // give access to elements lying beneath
      if(pp->isSelected) pe_sel = pp;
    }
  }

  return pe_1st;
}

// ---------------------------------------------------
// Deselects all elements except 'e'.
void Schematic::deselectElements(Element *e)
{
  // test all components
  for(Component *pc = Components->first(); pc != 0; pc = Components->next())
    if(e != pc)  pc->isSelected = false;

  // test all wires
  for(Wire *pw = Wires->first(); pw != 0; pw = Wires->next()) {
    if(e != pw)  pw->isSelected = false;
    if(pw->Label) if(pw->Label != e)  pw->Label->isSelected = false;
  }

  // test all node labels
  for(Node *pn = Nodes->first(); pn != 0; pn = Nodes->next())
    if(pn->Label) if(pn->Label != e)  pn->Label->isSelected = false;

  // test all diagrams
  for(Diagram *pd = Diagrams->first(); pd != 0; pd = Diagrams->next()) {
    if(e != pd)  pd->isSelected = false;

    // test graphs of diagram
    for(Graph *pg = pd->Graphs.first(); pg != 0; pg = pd->Graphs.next()) {
      if(e != pg) pg->isSelected = false;

      // test markers of graph
      for(Marker *pm = pg->Markers.first(); pm != 0; pm = pg->Markers.next())
        if(e != pm) pm->isSelected = false;
    }

  }

  // test all paintings
  for(Painting *pp = Paintings->first(); pp != 0; pp = Paintings->next())
    if(e != pp)  pp->isSelected = false;
}

// ---------------------------------------------------
// Selects elements that lie within the rectangle x1/y1, x2/y2.
int Schematic::selectElements(int x1, int y1, int x2, int y2, bool flag)
{
  int  z=0;   // counts selected elements
  int  cx1, cy1, cx2, cy2;

  // exchange rectangle coordinates to obtain x1 < x2 and y1 < y2
  cx1 = (x1 < x2) ? x1 : x2; cx2 = (x1 > x2) ? x1 : x2;
  cy1 = (y1 < y2) ? y1 : y2; cy2 = (y1 > y2) ? y1 : y2;
  x1 = cx1; x2 = cx2;
  y1 = cy1; y2 = cy2;

  // test all components
  for(Component *pc = Components->first(); pc != 0; pc = Components->next()) {
    pc->Bounding(cx1, cy1, cx2, cy2);
    if(cx1 >= x1) if(cx2 <= x2) if(cy1 >= y1) if(cy2 <= y2) {
      pc->isSelected = true;  z++;
      continue;
    }
    if(pc->isSelected &= flag) z++;
  }


  Wire *pw;
  for(pw = Wires->first(); pw != 0; pw = Wires->next()) { // test all wires
    if(pw->x1 >= x1) if(pw->x2 <= x2) if(pw->y1 >= y1) if(pw->y2 <= y2) {
      pw->isSelected = true;  z++;
      continue;
    }
    if(pw->isSelected &= flag) z++;
  }


  // test all wire labels *********************************
  WireLabel *pl=0;
  for(pw = Wires->first(); pw != 0; pw = Wires->next()) {
    if(pw->Label) {
      pl = pw->Label;
      if(pl->x1 >= x1) if((pl->x1+pl->x2) <= x2)
        if(pl->y1 >= y1) if((pl->y1+pl->y2) <= y2) {
          pl->isSelected = true;  z++;
          continue;
        }
      if(pl->isSelected &= flag) z++;
    }
  }


  // test all node labels *************************************
  for(Node *pn = Nodes->first(); pn != 0; pn = Nodes->next()) {
    pl = pn->Label;
    if(pl) {
      if(pl->x1 >= x1) if((pl->x1+pl->x2) <= x2)
        if((pl->y1-pl->y2) >= y1) if(pl->y1 <= y2) {
          pl->isSelected = true;  z++;
          continue;
        }
      if(pl->isSelected &= flag) z++;
    }
  }


  // test all diagrams *******************************************
  for(Diagram *pd = Diagrams->first(); pd != 0; pd = Diagrams->next()) {
    // test graphs of diagram
    for(Graph *pg = pd->Graphs.first(); pg != 0; pg = pd->Graphs.next()) {
      if(pg->isSelected &= flag) z++;

      // test markers of graph
      for(Marker *pm = pg->Markers.first(); pm!=0; pm = pg->Markers.next()) {
	pm->Bounding(cx1, cy1, cx2, cy2);
	if(cx1 >= x1) if(cx2 <= x2) if(cy1 >= y1) if(cy2 <= y2) {
	    pm->isSelected = true;  z++;
	    continue;
          }
        if(pm->isSelected &= flag) z++;
      }
    }

    // test diagram itself
    pd->Bounding(cx1, cy1, cx2, cy2);
    if(cx1 >= x1) if(cx2 <= x2) if(cy1 >= y1) if(cy2 <= y2) {
      pd->isSelected = true;  z++;
      continue;
    }
    if(pd->isSelected &= flag) z++;
  }

  // test all paintings *******************************************
  for(Painting *pp = Paintings->first(); pp != 0; pp = Paintings->next()) {
    pp->Bounding(cx1, cy1, cx2, cy2);
    if(cx1 >= x1) if(cx2 <= x2) if(cy1 >= y1) if(cy2 <= y2) {
      pp->isSelected = true;  z++;
      continue;
    }
    if(pp->isSelected &= flag) z++;
  }

  return z;
}

// ---------------------------------------------------
// Selects all markers.
void Schematic::selectMarkers()
{
  for(Diagram *pd = Diagrams->first(); pd != 0; pd = Diagrams->next())
    for(Graph *pg = pd->Graphs.first(); pg != 0; pg = pd->Graphs.next())
      for(Marker *pm = pg->Markers.first(); pm!=0; pm = pg->Markers.next())
         pm->isSelected = true;
}

// ---------------------------------------------------
// For moving elements: If the moving element is connected to a not
// moving element, insert two wires. If the connected element is already
// a wire, use this wire. Otherwise create new wire.
void Schematic::newMovingWires(QPtrList<Element> *p, Node *pn)
{
  if(pn->Connections.count() < 1) return; // return, if connected node moves
  Element *pe = pn->Connections.getFirst();
  if(pe == (Element*)1) return; // return, if it was already treated this way
  pn->Connections.prepend((Element*)1);  // to avoid doubling

  if(pn->Connections.count() == 2)    // 2, because of prepend (Element*)1
    if(pe->Type == isWire) {    // is it connected to exactly one wire ?

      // .................................................
      Wire *pw2=0, *pw = (Wire*)pe;

      Node *pn2 = pw->Port1;
      if(pn2 == pn) pn2 = pw->Port2;

      if(pn2->Connections.count() == 2) { // two existing wires connected ?
        Element *pe2 = pn2->Connections.getFirst();
        if(pe2 == pe) pe2 = pn2->Connections.getLast();
        if(pe2 != (Element*)1)
          if(pe2->Type == isWire)  // connected wire connected to ...
            pw2  = (Wire*)pe2;     // ... exactly one wire ?
      }

      // .................................................
      // reuse one wire
      p->append(pw);
      pw->Port1->Connections.removeRef(pw);   // remove connection 1
      pw->Port2->Connections.removeRef(pw);   // remove connection 2
      if(pw->Port1->Connections.getFirst() !=  (Element*)1)
        pw->Port1->Connections.prepend((Element*)1);  // remember this action
      if(pw->Port2->Connections.getFirst() !=  (Element*)1)
        pw->Port2->Connections.prepend((Element*)1);  // remember this action
      Wires->take(Wires->findRef(pw));
      long mask = 1;
      if(pw->isHorizontal()) mask = 2;

      if(pw->Port1 != pn) {
	pw->Port1->State = mask;
	pw->Port1 = (Node*)mask;
	pw->Port2->State = 3;
	pw->Port2 = (Node*)3;    // move port 2 completely
      }
      else {
	pw->Port1->State = 3;
	pw->Port1 = (Node*)3;
	pw->Port2->State = mask;
	pw->Port2 = (Node*)mask;
      }

      // .................................................
      // create new wire ?
      if(pw2 == 0) {
        if(pw->Port1 == (Node*)3)
          p->append(new Wire(pw->x2, pw->y2, pw->x2, pw->y2,
			     (Node*)mask, (Node*)0));
        else
          p->append(new Wire(pw->x1, pw->y1, pw->x1, pw->y1,
			     (Node*)mask, (Node*)0));
        return;
      }


      // .................................................
      // reuse a second wire
      p->append(pw2);
      pw2->Port1->Connections.removeRef(pw2);   // remove connection 1
      pw2->Port2->Connections.removeRef(pw2);   // remove connection 2
      if(pw2->Port1->Connections.getFirst() !=  (Element*)1)
        pw2->Port1->Connections.prepend((Element*)1); // remember this action
      if(pw2->Port2->Connections.getFirst() !=  (Element*)1)
        pw2->Port2->Connections.prepend((Element*)1); // remember this action
      Wires->take(Wires->findRef(pw2));

      if(pw2->Port1 != pn2) {
	pw2->Port1->State = 0;
	pw2->Port1 = (Node*)0;
	pw2->Port2->State = mask;
	pw2->Port2 = (Node*)mask;
      }
      else {
	pw2->Port1->State = mask;
	pw2->Port1 = (Node*)mask;
	pw2->Port2->State = 0;
	pw2->Port2 = (Node*)0;
      }
      return;
    }

  // only x2 moving
  p->append(new Wire(pn->cx, pn->cy, pn->cx, pn->cy, (Node*)0, (Node*)1));
  // x1, x2, y2 moving
  p->append(new Wire(pn->cx, pn->cy, pn->cx, pn->cy, (Node*)1, (Node*)3));
}

// ---------------------------------------------------
// For moving of elements: Copies all selected elements into the
// list 'p' and deletes them from the document.
int Schematic::copySelectedElements(QPtrList<Element> *p)
{
  int markerCount = 0;
  Port      *pp;
  Component *pc;
  Wire      *pw;
  Diagram   *pd;
  Element   *pe;
  Node      *pn;


  // test all components *********************************
  for(pc = Components->first(); pc != 0; )
    if(pc->isSelected) {
      p->append(pc);

      // delete all port connections
      for(pp = pc->Ports.first(); pp!=0; pp = pc->Ports.next())
        pp->Connection->Connections.removeRef((Element*)pc);

      Components->take();   // take component out of the document
      pc = Components->current();
    }
    else pc = Components->next();

  // test all wires and wire labels ***********************
  for(pw = Wires->first(); pw != 0; ) {
    if(pw->Label) if(pw->Label->isSelected)
      p->append(pw->Label);

    if(pw->isSelected) {
      p->append(pw);

      pw->Port1->Connections.removeRef(pw);   // remove connection 1
      pw->Port2->Connections.removeRef(pw);   // remove connection 2
      Wires->take();
      pw = Wires->current();
    }
    else pw = Wires->next();
  }

  // test all diagrams **********************************
  for(pd = Diagrams->first(); pd != 0; )
    if(pd->isSelected) {
      p->append(pd);
      Diagrams->take();
      pd = Diagrams->current();
    }
    else {
      for(Graph *pg = pd->Graphs.first(); pg!=0; pg = pd->Graphs.next())
        for(Marker *pm = pg->Markers.first(); pm != 0; )
          if(pm->isSelected) {
            markerCount++;
            p->append(pm);
            pg->Markers.take();
            pm = pg->Markers.current();
          }
          else pm = pg->Markers.next();

      pd = Diagrams->next();
    }

  // test all paintings **********************************
  for(Painting *ppa = Paintings->first(); ppa != 0; )
    if(ppa->isSelected) {
      p->append(ppa);
      Paintings->take();
      ppa = Paintings->current();
    }
    else ppa = Paintings->next();

  // ..............................................
  // Inserts wires, if a connection to a not moving element is found.
  // Go backwards in order not to test the new insertions.
  for(pe = p->last(); pe!=0; pe = p->prev())
    if(pe->Type & isComponent) {
      pc = (Component*)pe;
      for(pp = pc->Ports.first(); pp!=0; pp = pc->Ports.next())
         newMovingWires(p, pp->Connection);

      p->findRef(pe);   // back to the real current pointer
    }
    else if(pe->Type == isWire) {
      pw = (Wire*)pe;
      newMovingWires(p, pw->Port1);
      newMovingWires(p, pw->Port2);
      p->findRef(pe);   // back to the real current pointer
    }


  // ..............................................
  // delete the unused nodes
  for(pn = Nodes->first(); pn!=0; ) {
    if(pn->Connections.getFirst() == (Element*)1) {
      pn->Connections.removeFirst();  // delete tag
      if(pn->Connections.count() == 2)
        if(oneTwoWires(pn)) {  // if possible, connect two wires to one
          pn = Nodes->current();
          continue;
        }
    }

    if(pn->Connections.count() == 0) {
      if(pn->Label) {
	pn->Label->Type = isMovingLabel;
	if(pn->State & 1) {
	  if(!(pn->State & 2)) pn->Label->Type = isHMovingLabel;
	}
	else if(pn->State & 2) pn->Label->Type = isVMovingLabel;
	pn->State = 0;
	p->append(pn->Label);    // do not forget the node labels
      }
      Nodes->remove();
      pn = Nodes->current();
      continue;
    }

    pn = Nodes->next();
  }

  // test all node labels
  // do this last to avoid double copying
  for(pn = Nodes->first(); pn != 0; pn = Nodes->next())
    if(pn->Label) if(pn->Label->isSelected)
      p->append(pn->Label);

  return markerCount;
}

// ---------------------------------------------------
bool Schematic::copyComps2WiresPaints(int& x1, int& y1, int& x2, int& y2,
			QPtrList<Element> *ElementCache)
{
  x1=INT_MAX;
  y1=INT_MAX;
  x2=INT_MIN;
  y2=INT_MIN;
  copyLabels(x1, y1, x2, y2, ElementCache);   // must be first of all !
  copyComponents2(x1, y1, x2, y2, ElementCache);
  copyWires(x1, y1, x2, y2, ElementCache);
  copyPaintings(x1, y1, x2, y2, ElementCache);

  if(y1 == INT_MAX) return false;  // no element selected
  return true;
}

// ---------------------------------------------------
// Used in "aligning()", "distribHoriz()" and "distribVert()".
int Schematic::copyElements(int& x1, int& y1, int& x2, int& y2,
			QPtrList<Element> *ElementCache)
{
  int bx1, by1, bx2, by2;
  Wires->setAutoDelete(false);
  Components->setAutoDelete(false);

  x1=INT_MAX;
  y1=INT_MAX;
  x2=INT_MIN;
  y2=INT_MIN;
  // take components and wires out of list, check their boundings
  copyComponents(x1, y1, x2, y2, ElementCache);
  copyWires(x1, y1, x2, y2, ElementCache);
  int number = ElementCache->count();

  Wires->setAutoDelete(true);
  Components->setAutoDelete(true);

  // find upper most selected diagram
  for(Diagram *pd = Diagrams->last(); pd != 0; pd = Diagrams->prev())
    if(pd->isSelected) {
      pd->Bounding(bx1, by1, bx2, by2);
      if(bx1 < x1) x1 = bx1;
      if(bx2 > x2) x2 = bx2;
      if(by1 < y1) y1 = by1;
      if(by2 > y2) y2 = by2;
      ElementCache->append(pd);
      number++;
    }
  // find upper most selected painting
  for(Painting *pp = Paintings->last(); pp != 0; pp = Paintings->prev())
    if(pp->isSelected) {
      pp->Bounding(bx1, by1, bx2, by2);
      if(bx1 < x1) x1 = bx1;
      if(bx2 > x2) x2 = bx2;
      if(by1 < y1) y1 = by1;
      if(by2 > y2) y2 = by2;
      ElementCache->append(pp);
      number++;
    }

  return number;
}

// ---------------------------------------------------
// Deletes all selected elements.
bool Schematic::deleteElements()
{
  bool sel = false;

  Component *pc = Components->first();
  while(pc != 0)      // all selected component
    if(pc->isSelected) {
      deleteComp(pc);
      pc = Components->current();
      sel = true;
    }
    else pc = Components->next();

  Wire *pw = Wires->first();
  while(pw != 0) {      // all selected wires and their labels
    if(pw->Label)
      if(pw->Label->isSelected) {
        delete pw->Label;
        pw->Label = 0;
        sel = true;
      }

    if(pw->isSelected) {
      deleteWire(pw);
      pw = Wires->current();
      sel = true;
    }
    else pw = Wires->next();
  }

  // all selected labels on nodes ***************************
  for(Node *pn = Nodes->first(); pn != 0; pn = Nodes->next())
    if(pn->Label)
      if(pn->Label->isSelected) {
        delete pn->Label;
        pn->Label = 0;
        sel = true;
      }

  Diagram *pd = Diagrams->first();
  while(pd != 0)      // test all diagrams
    if(pd->isSelected) {
      Diagrams->remove();
      pd = Diagrams->current();
      sel = true;
    }
    else {
      bool wasGraphDeleted = false;
      // all graphs of diagram
      for(Graph *pg = pd->Graphs.first(); pg != 0; ) {
        // all markers of diagram
        for(Marker *pm = pg->Markers.first(); pm != 0; )
          if(pm->isSelected) {
            pg->Markers.remove();
            pm = pg->Markers.current();
            sel = true;
          }
          else  pm = pg->Markers.next();

        if(pg->isSelected) {
          pd->Graphs.remove();
          pg = pd->Graphs.current();
          sel = wasGraphDeleted = true;
        }
        else  pg = pd->Graphs.next();
      }
      if(wasGraphDeleted)
        pd->recalcGraphData();  // update diagram (resize etc.)

      pd = Diagrams->next();
    }

  Painting *pp = Paintings->first();
  while(pp != 0) {    // test all paintings
    if(pp->isSelected)
      if(pp->Name.at(0) != '.') {  // do not delete "PortSym", "ID_text"
	sel = true;
	Paintings->remove();
	pp = Paintings->current();
	continue;
      }
    pp = Paintings->next();
  }

  if(sel) {
    sizeOfAll(UsedX1, UsedY1, UsedX2, UsedY2);   // set new document size
    setChanged(sel, true);
  }
  return sel;
}

// ---------------------------------------------------
bool Schematic::aligning(int Mode)
{
  int x1, y1, x2, y2;
  int bx1, by1, bx2, by2, *bx=0, *by=0;
  QPtrList<Element> ElementCache;
  int count = copyElements(x1, y1, x2, y2, &ElementCache);
  if(count < 1) return false;


  switch(Mode) {
    case 0:  // align top
	bx = &x1;
	by = &by1;
	break;
    case 1:  // align bottom
	bx = &x1;
	y1 = y2;
	by = &by2;
	break;
    case 2:  // align left
	by = &y1;
	bx = &bx1;
	break;
    case 3:  // align right
	by = &y1;
	x1 = x2;
	bx = &bx2;
	break;
  }


  Wire *pw;
  Component *pc;
  // re-insert elements
  for(Element *pe = ElementCache.first(); pe != 0; pe = ElementCache.next())
    switch(pe->Type) {
      case isComponent:
      case isAnalogComponent:
      case isDigitalComponent:
	pc = (Component*)pe;
	pc->Bounding(bx1, by1, bx2, by2);
	pc->setCenter(x1-(*bx), y1-(*by), true);
	insertRawComponent(pc);
	break;

      case isWire:
	pw = (Wire*)pe;
	bx1 = pw->x1;
	by1 = pw->y1;
	bx2 = pw->x2;
	by2 = pw->y2;
	pw->setCenter(x1-(*bx), y1-(*by), true);
//	if(pw->Label) {	}
	insertWire(pw);
	break;

      case isDiagram:
	((Diagram*)pe)->Bounding(bx1, by1, bx2, by2);
	((Diagram*)pe)->setCenter(x1-(*bx), y1-(*by), true);
	break;

      case isPainting:
	((Painting*)pe)->Bounding(bx1, by1, bx2, by2);
	((Painting*)pe)->setCenter(x1-(*bx), y1-(*by), true);
/*
      case isNodeLabel:
	bx1 = ((WireLabel*)pe)->cx;
	by1 = ((WireLabel*)pe)->cy;
	bx2 = ((WireLabel*)pe)->cx;
	by2 = ((WireLabel*)pe)->cy;
	((WireLabel*)pe)->cx = x1-(*bx);
	((WireLabel*)pe)->cy = y1-(*by);
	insertNodeLabel((WireLabel*)pe);
	break;
*/
      default: ;
    }

  ElementCache.clear();
  if(count < 2) return false;

  setChanged(true, true);
  return true;
}

// ---------------------------------------------------
bool Schematic::distribHoriz()
{
  int x1, y1, x2, y2;
  int bx1, by1, bx2, by2;
  QPtrList<Element> ElementCache;
  int count = copyElements(x1, y1, x2, y2, &ElementCache);
  if(count < 1) return false;

  // using bubble sort to get elements x ordered
  Element *pe;
if(count > 1)
  for(int i = count-1; i>0; i--) {
    pe = ElementCache.first();
    for(int j=0; j<i; j++) {
      pe->getCenter(bx1, by1);
      pe=ElementCache.next();
      pe->getCenter(bx2, by2);
      if(bx1 > bx2) {  // change two elements ?
	ElementCache.replace(j+1, ElementCache.prev());
	ElementCache.replace(j, pe);
	pe = ElementCache.at(j+1);
      }
    }
  }

  ElementCache.getLast()->getCenter(x2, y2);
  ElementCache.getFirst()->getCenter(x1, y1);
  // re-insert elements and put them at right position
  Wire *pw;
  int x  = x1;
  int dx=0;
  if(count > 1) dx = (x2-x1)/(count-1);
  for(pe = ElementCache.first(); pe!=0; pe = ElementCache.next()) {
    switch(pe->Type) {
      case isComponent:
      case isAnalogComponent:
      case isDigitalComponent:
	((Component*)pe)->cx = x;
	insertRawComponent((Component*)pe);
	break;

      case isWire:
	pw = (Wire*)pe;
	if(pw->isHorizontal()) {
	  x1 = pw->x2 - pw->x1;
	  pw->x1 = x - (x1 >> 1);
	  pw->x2 = pe->x1 + x1;
	}
	else  pw->x1 = pw->x2 = x;
//	if(pw->Label) {	}
	insertWire(pw);
	break;

      case isDiagram:
	((Diagram*)pe)->cx = x - (((Diagram*)pe)->x2 >> 1);
	break;

      case isPainting:
	pe->getCenter(bx1, by1);
	pe->setCenter(x, by1, false);
      default: ;
    }
    x += dx;
  }

  ElementCache.clear();
  if(count < 2) return false;

  setChanged(true, true);
  return true;
}

// ---------------------------------------------------
bool Schematic::distribVert()
{
  int x1, y1, x2, y2;
  int bx1, by1, bx2, by2;
  QPtrList<Element> ElementCache;
  int count = copyElements(x1, y1, x2, y2, &ElementCache);
  if(count < 1) return false;

  // using bubble sort to get elements x ordered
  Element *pe;
if(count > 1)
  for(int i = count-1; i>0; i--) {
    pe = ElementCache.first();
    for(int j=0; j<i; j++) {
      pe->getCenter(bx1, by1);
      pe=ElementCache.next();
      pe->getCenter(bx2, by2);
      if(by1 > by2) {  // change two elements ?
	ElementCache.replace(j+1, ElementCache.prev());
	ElementCache.replace(j, pe);
	pe = ElementCache.at(j+1);
      }
    }
  }

  ElementCache.getLast()->getCenter(x2, y2);
  ElementCache.getFirst()->getCenter(x1, y1);
  // re-insert elements and put them at right position
  Wire *pw;
  int y  = y1;
  int dy=0;
  if(count > 1) dy = (y2-y1)/(count-1);
  for(pe = ElementCache.first(); pe!=0; pe = ElementCache.next()) {
    switch(pe->Type) {
      case isComponent:
      case isAnalogComponent:
      case isDigitalComponent:
	pe->cy = y;
	insertRawComponent((Component*)pe);
	break;

      case isWire:
	pw = (Wire*)pe;
	if(pw->isHorizontal())  pw->y1 = pw->y2 = y;
	else {
	  y1 = pw->y2 - pw->y1;
	  pw->y1 = y - (y1 >> 1);
	  pw->y2 = pe->y1 + y1;
	}
//	if(pw->Label) {	}
	insertWire(pw);
	break;

      case isDiagram:
	pe->cy = y + (pe->y2 >> 1);
	break;

      case isPainting:
	pe->getCenter(bx1, by1);
	pe->setCenter(bx1, y, false);
      default: ;
    }
    y += dy;
  }

  ElementCache.clear();
  if(count < 2) return false;

  setChanged(true, true);
  return true;
}


/* *******************************************************************
   *****                                                         *****
   *****                Actions with components                  *****
   *****                                                         *****
   ******************************************************************* */

// Finds the correct number for power sources, subcircuit ports and
// digital sources and sets them accordingly.
void Schematic::setComponentNumber(Component *c)
{
  Property *pp = c->Props.getFirst();
  if(!pp) return;
  if(pp->Name != "Num") return;

  int n=1;
  QString s = pp->Value;
  QString cSign = c->Model;
  Component *pc;
  // First look, if the port number already exists.
  for(pc = Components->first(); pc != 0; pc = Components->next())
    if(pc->Model == cSign)
      if(pc->Props.getFirst()->Value == s) break;
  if(!pc) return;   // was port number not yet in use ?

  // Find the first free number.
  do {
    s  = QString::number(n);
    // look for existing ports and their numbers
    for(pc = Components->first(); pc != 0; pc = Components->next())
      if(pc->Model == cSign)
        if(pc->Props.getFirst()->Value == s) break;

    n++;
  } while(pc);   // found not used component number
  pp->Value = s; // set new number
}

// ---------------------------------------------------
void Schematic::insertComponentNodes(Component *c, bool noOptimize)
{
  Port *pp;
  // connect every node of the component to corresponding schematic node
  for(pp = c->Ports.first(); pp != 0; pp = c->Ports.next())
    pp->Connection = insertNode(pp->x+c->cx, pp->y+c->cy, c);

  if(noOptimize)  return;

  Node    *pn;
  Element *pe, *pe1;
  QPtrList<Element> *pL;
  // if component over wire then delete this wire
  c->Ports.first();  // omit the first element
  for(pp = c->Ports.next(); pp != 0; pp = c->Ports.next()) {
    pn = pp->Connection;
    for(pe = pn->Connections.first(); pe!=0; pe = pn->Connections.next())
      if(pe->Type == isWire) {
	if(((Wire*)pe)->Port1 == pn)  pL = &(((Wire*)pe)->Port2->Connections);
	else  pL = &(((Wire*)pe)->Port1->Connections);

        for(pe1 = pL->first(); pe1!=0; pe1 = pL->next())
	  if(pe1 == c) {
	    deleteWire((Wire*)pe);
	    break;
	  }
      }
  }
}

// ---------------------------------------------------
// Used for example in moving components.
void Schematic::insertRawComponent(Component *c, bool noOptimize)
{
  // connect every node of component to corresponding schematic node
  insertComponentNodes(c, noOptimize);
  Components->append(c);

  // a ground symbol erases an existing label on the wire line
  if(c->Model == "GND") {
    c->Model = "x";    // prevent that this ground is found as label
    Element *pe = getWireLabel(c->Ports.getFirst()->Connection);
    if(pe) if((pe->Type & isComponent) == 0) {
      delete ((Conductor*)pe)->Label;
      ((Conductor*)pe)->Label = 0;
    }
    c->Model = "GND";    // rebuild component model
  }
}

// ---------------------------------------------------
void Schematic::recreateComponent(Component *Comp)
{
  Port *pp;
  WireLabel **plMem=0, **pl;
  int PortCount = Comp->Ports.count();

  if(PortCount > 0) {
    // Save the labels whose node is not connected to somewhere else.
    // Otherwise the label would be deleted.
    pl = plMem = (WireLabel**)malloc(PortCount * sizeof(WireLabel*));
    for(pp = Comp->Ports.first(); pp != 0; pp = Comp->Ports.next())
      if(pp->Connection->Connections.count() < 2) {
        *(pl++) = pp->Connection->Label;
        pp->Connection->Label = 0;
      }
      else  *(pl++) = 0;
  }


  int x = Comp->tx, y = Comp->ty;
  int x1 = Comp->x1, x2 = Comp->x2, y1 = Comp->y1, y2 = Comp->y2;
  QString tmp = Comp->Name;    // is sometimes changed by "recreate"
  Comp->recreate(this);   // to apply changes to the schematic symbol
  Comp->Name = tmp;
  if(x < x1)
    x += Comp->x1 - x1;
  else if(x > x2)
    x += Comp->x2 - x2;
  if(y < y1)
    y += Comp->y1 - y1;
  else if(y > y2)
    y += Comp->y2 - y2;
  Comp->tx = x;  Comp->ty = y;


  if(PortCount > 0) {
    // restore node labels
    pl = plMem;
    for(pp = Comp->Ports.first(); pp != 0; pp = Comp->Ports.next()) {
      if(*pl != 0) {
        (*pl)->cx = pp->Connection->cx;
        (*pl)->cy = pp->Connection->cy;
        placeNodeLabel(*pl);
      }
      pl++;
      if((--PortCount) < 1)  break;
    }
    for( ; PortCount > 0; PortCount--) {
      delete (*pl);  // delete not needed labels
      pl++;
    }
    free(plMem);
  }
}

// ---------------------------------------------------
void Schematic::insertComponent(Component *c)
{
  // connect every node of component to corresponding schematic node
  insertComponentNodes(c, false);

  bool ok;
  QString s;
  int  max=1, len = c->Name.length(), z;
  if(c->Name.isEmpty()) {
    // a ground symbol erases an existing label on the wire line
    if(c->Model == "GND") {
      c->Model = "x";    // prevent that this ground is found as label
      Element *pe = getWireLabel(c->Ports.getFirst()->Connection);
      if(pe) if((pe->Type & isComponent) == 0) {
        delete ((Conductor*)pe)->Label;
        ((Conductor*)pe)->Label = 0;
      }
      c->Model = "GND";    // rebuild component model
    }
  }
  else {
    // determines the name by looking for names with the same
    // prefix and increment the number
    for(Component *pc = Components->first(); pc != 0; pc = Components->next())
      if(pc->Name.left(len) == c->Name) {
        s = pc->Name.right(pc->Name.length()-len);
        z = s.toInt(&ok);
        if(ok) if(z >= max) max = z + 1;
      }
    c->Name += QString::number(max);  // create name with new number
  }

  setComponentNumber(c); // important for power sources and subcircuit ports
  Components->append(c);
}

// ---------------------------------------------------
void Schematic::activateCompsWithinRect(int x1, int y1, int x2, int y2)
{
  bool changed = false;
  int  cx1, cy1, cx2, cy2, a;
  // exchange rectangle coordinates to obtain x1 < x2 and y1 < y2
  cx1 = (x1 < x2) ? x1 : x2; cx2 = (x1 > x2) ? x1 : x2;
  cy1 = (y1 < y2) ? y1 : y2; cy2 = (y1 > y2) ? y1 : y2;
  x1 = cx1; x2 = cx2;
  y1 = cy1; y2 = cy2;


  for(Component *pc = Components->first(); pc != 0; pc = Components->next()) {
    pc->Bounding(cx1, cy1, cx2, cy2);
    if(cx1 >= x1) if(cx2 <= x2) if(cy1 >= y1) if(cy2 <= y2) {
      a = pc->isActive - 1;

      if(pc->Ports.count() > 1) {
        if(a < 0)  a = 2;
        pc->isActive = a;    // change "active status"
      }
      else {
        a &= 1;
        pc->isActive = a;    // change "active status"
        if(a == COMP_IS_ACTIVE)  // only for active (not shorten)
          if(pc->Model == "GND")  // if existing, delete label on wire line
            oneLabel(pc->Ports.getFirst()->Connection);
      }
      changed = true;
    }
  }

  if(changed)  setChanged(true, true);
}

// ---------------------------------------------------
bool Schematic::activateSpecifiedComponent(int x, int y)
{
  int x1, y1, x2, y2, a;
  for(Component *pc = Components->first(); pc != 0; pc = Components->next()) {
    pc->Bounding(x1, y1, x2, y2);
    if(x >= x1) if(x <= x2) if(y >= y1) if(y <= y2) {
      a = pc->isActive - 1;

      if(pc->Ports.count() > 1) {
        if(a < 0)  a = 2;
        pc->isActive = a;    // change "active status"
      }
      else {
        a &= 1;
        pc->isActive = a;    // change "active status"
        if(a == COMP_IS_ACTIVE)  // only for active (not shorten)
          if(pc->Model == "GND")  // if existing, delete label on wire line
            oneLabel(pc->Ports.getFirst()->Connection);
      }
      setChanged(true, true);
      return true;
    }
  }
  return false;
}

// ---------------------------------------------------
bool Schematic::activateSelectedComponents()
{
  int a;
  bool sel = false;
  for(Component *pc = Components->first(); pc != 0; pc = Components->next())
    if(pc->isSelected) {
      a = pc->isActive - 1;

      if(pc->Ports.count() > 1) {
        if(a < 0)  a = 2;
        pc->isActive = a;    // change "active status"
      }
      else {
        a &= 1;
        pc->isActive = a;    // change "active status"
        if(a == COMP_IS_ACTIVE)  // only for active (not shorten)
          if(pc->Model == "GND")  // if existing, delete label on wire line
            oneLabel(pc->Ports.getFirst()->Connection);
      }
      sel = true;
    }

  if(sel) setChanged(true, true);
  return sel;
}

// ---------------------------------------------------
// Sets the component ports anew. Used after rotate, mirror etc.
void Schematic::setCompPorts(Component *pc)
{
  WireLabel *pl=0;
  for(Port *pp = pc->Ports.first(); pp!=0; pp = pc->Ports.next()) {
    pp->Connection->Connections.removeRef((Element*)pc);// delete connections
    switch(pp->Connection->Connections.count()) {
      case 0: pl = pp->Connection->Label;
              Nodes->removeRef(pp->Connection);
              break;
      case 2: oneTwoWires(pp->Connection); // try to connect two wires to one
              pl = 0;
      default: ;
    }
    // connect component node to schematic node
    pp->Connection = insertNode(pp->x+pc->cx, pp->y+pc->cy, pc);
    if(pl) {
      if(!getWireLabel(pp->Connection)) {
        pl->cx = pp->Connection->cx;
        pl->cy = pp->Connection->cy;
        pp->Connection->Label = pl;   // restore node label if unlabeled
      }
      else delete pl;
    }
  }
}

// ---------------------------------------------------
// Returns a pointer of the component on whose text x/y points.
Component* Schematic::selectCompText(int x_, int y_, int& w, int& h)
{
  int a, b, dx, dy;
  for(Component *pc = Components->first(); pc != 0; pc = Components->next()) {
    a = pc->cx + pc->tx;
    if(x_ < a)  continue;
    b = pc->cy + pc->ty;
    if(y_ < b)  continue;

    pc->textSize(dx, dy);
    if(x_ > a+dx)  continue;
    if(y_ > b+dy)  continue;

    w = dx;
    h = dy;
    return pc;
  }

  return 0;
}

// ---------------------------------------------------
Component* Schematic::searchSelSubcircuit()
{
  Component *sub=0;
  // test all components
  for(Component *pc = Components->first(); pc != 0; pc = Components->next()) {
    if(!pc->isSelected) continue;
    if(pc->Model != "Sub")
      if(pc->Model != "VHDL") continue;

    if(sub != 0) return 0;    // more than one subcircuit selected
    sub = pc;
  }
  return sub;
}

// ---------------------------------------------------
Component* Schematic::selectedComponent(int x, int y)
{
  // test all components
  for(Component *pc = Components->first(); pc != 0; pc = Components->next())
    if(pc->getSelected(x, y))
      return pc;

  return 0;
}

// ---------------------------------------------------
// Deletes the component 'c'.
void Schematic::deleteComp(Component *c)
{
  Port *pn;

  // delete all port connections
  for(pn = c->Ports.first(); pn!=0; pn = c->Ports.next())
    switch(pn->Connection->Connections.count()) {
      case 1  : if(pn->Connection->Label) delete pn->Connection->Label;
                Nodes->removeRef(pn->Connection);  // delete open nodes
                pn->Connection = 0;		  //  (auto-delete)
                break;
      case 3  : pn->Connection->Connections.removeRef(c);// delete connection
                oneTwoWires(pn->Connection);  // two wires -> one wire
                break;
      default : pn->Connection->Connections.removeRef(c);// remove connection
                break;
    }

  Components->removeRef(c);   // delete component
}

// ---------------------------------------------------
void Schematic::copyComponents(int& x1, int& y1, int& x2, int& y2,
			QPtrList<Element> *ElementCache)
{
  Component *pc;
  int bx1, by1, bx2, by2;
  // find bounds of all selected components
  for(pc = Components->first(); pc != 0; ) {
    if(pc->isSelected) {
      pc->Bounding(bx1, by1, bx2, by2);  // is needed because of "distribute
      if(bx1 < x1) x1 = bx1;             // uniformly"
      if(bx2 > x2) x2 = bx2;
      if(by1 < y1) y1 = by1;
      if(by2 > y2) y2 = by2;

      ElementCache->append(pc);

      Port *pp;   // rescue non-selected node labels
      for(pp = pc->Ports.first(); pp != 0; pp = pc->Ports.next())
        if(pp->Connection->Label)
          if(pp->Connection->Connections.count() < 2) {
            ElementCache->append(pp->Connection->Label);
            pp->Connection->Label = 0;
            // Don't set pp->Connection->Label->pOwner=0,
            // so text position stays unchanged.
          }

      deleteComp(pc);
      pc = Components->current();
      continue;
    }
    pc = Components->next();
  }
}

// ---------------------------------------------------
void Schematic::copyComponents2(int& x1, int& y1, int& x2, int& y2,
			QPtrList<Element> *ElementCache)
{
  Component *pc;
  // find bounds of all selected components
  for(pc = Components->first(); pc != 0; ) {
    if(pc->isSelected) {
      // is better for unsymmetrical components
      if(pc->cx < x1)  x1 = pc->cx;
      if(pc->cx > x2)  x2 = pc->cx;
      if(pc->cy < y1)  y1 = pc->cy;
      if(pc->cy > y2)  y2 = pc->cy;

      ElementCache->append(pc);

      Port *pp;   // rescue non-selected node labels
      for(pp = pc->Ports.first(); pp != 0; pp = pc->Ports.next())
        if(pp->Connection->Label)
          if(pp->Connection->Connections.count() < 2) {
            ElementCache->append(pp->Connection->Label);
            pp->Connection->Label = 0;
            // Don't set pp->Connection->Label->pOwner=0,
            // so text position stays unchanged.
          }

      deleteComp(pc);
      pc = Components->current();
      continue;
    }
    pc = Components->next();
  }
}


/* *******************************************************************
   *****                                                         *****
   *****                  Actions with labels                    *****
   *****                                                         *****
   ******************************************************************* */

// Test, if wire connects wire line with more than one label and delete
// all further labels. Also delete all labels if wire line is grounded.
void Schematic::oneLabel(Node *n1)
{
  Wire *pw;
  Node *pn, *pNode;
  Element *pe;
  WireLabel *pl = 0;
  bool named=false;   // wire line already named ?
  QPtrList<Node> Cons;

  for(pn = Nodes->first(); pn!=0; pn = Nodes->next())
    pn->y1 = 0;   // mark all nodes as not checked

  Cons.append(n1);
  n1->y1 = 1;  // mark Node as already checked
  for(pn = Cons.first(); pn!=0; pn = Cons.next()) {
    if(pn->Label) {
      if(named) {
        delete pn->Label;
        pn->Label = 0;    // erase double names
      }
      else {
	named = true;
	pl = pn->Label;
      }
    }

    for(pe = pn->Connections.first(); pe!=0; pe = pn->Connections.next()) {
      if(pe->Type != isWire) {
        if(((Component*)pe)->isActive == COMP_IS_ACTIVE)
	  if(((Component*)pe)->Model == "GND") {
	    named = true;
	    if(pl) {
	      pl->pOwner->Label = 0;
	      delete pl;
	    }
	    pl = 0;
	  }
	continue;
      }
      pw = (Wire*)pe;

      if(pn != pw->Port1) pNode = pw->Port1;
      else pNode = pw->Port2;

      if(pNode->y1) continue;
      pNode->y1 = 1;  // mark Node as already checked
      Cons.append(pNode);
      Cons.findRef(pn);

      if(pw->Label) {
        if(named) {
          delete pw->Label;
          pw->Label = 0;    // erase double names
        }
        else {
	  named = true;
	  pl = pw->Label;
	}
      }
    }
  }
}

// ---------------------------------------------------
int Schematic::placeNodeLabel(WireLabel *pl)
{
  Node *pn;
  int x = pl->cx;
  int y = pl->cy;

  // check if new node lies upon an existing node
  for(pn = Nodes->first(); pn != 0; pn = Nodes->next())
    if(pn->cx == x) if(pn->cy == y) break;

  if(!pn)  return -1;

  Element *pe = getWireLabel(pn);
  if(pe) {    // name found ?
    if(pe->Type & isComponent)  return -2;  // ground potential

    delete ((Conductor*)pe)->Label;
    ((Conductor*)pe)->Label = 0;
  }

  pn->Label = pl;   // insert node label
  return 0;
}

// ---------------------------------------------------
// Test, if wire line is already labeled and returns a pointer to the
// labeled element.
Element* Schematic::getWireLabel(Node *pn_)
{
  Wire *pw;
  Node *pn, *pNode;
  Element *pe;
  QPtrList<Node> Cons;

  for(pn = Nodes->first(); pn!=0; pn = Nodes->next())
    pn->y1 = 0;   // mark all nodes as not checked

  Cons.append(pn_);
  pn_->y1 = 1;  // mark Node as already checked
  for(pn = Cons.first(); pn!=0; pn = Cons.next())
    if(pn->Label) return pn;
    else
      for(pe = pn->Connections.first(); pe!=0; pe = pn->Connections.next()) {
        if(pe->Type != isWire) {
	  if(((Component*)pe)->isActive == COMP_IS_ACTIVE)
	    if(((Component*)pe)->Model == "GND") return pe;
          continue;
        }

        pw = (Wire*)pe;
        if(pw->Label) return pw;

        if(pn != pw->Port1) pNode = pw->Port1;
        else pNode = pw->Port2;

        if(pNode->y1) continue;
        pNode->y1 = 1;  // mark Node as already checked
        Cons.append(pNode);
        Cons.findRef(pn);
      }
  return 0;   // no wire label found
}

// ---------------------------------------------------
// Inserts a node label.
void Schematic::insertNodeLabel(WireLabel *pl)
{
  Node *pn;
  // check if node lies upon existing node
  for(pn = Nodes->first(); pn != 0; pn = Nodes->next())  // check every node
    if(pn->cx == pl->cx) if(pn->cy == pl->cy) break;

  if(!pn) {   // create new node, if no existing one lies at this position
    pn = new Node(pl->cx, pl->cy);
    Nodes->append(pn);
  }

  if(pn->Label) delete pn->Label;
  pn->Label = pl;
  pl->Type  = isNodeLabel;
  pl->pOwner = pn;
}

// ---------------------------------------------------
void Schematic::copyLabels(int& x1, int& y1, int& x2, int& y2,
			QPtrList<Element> *ElementCache)
{
  WireLabel *pl;
  // find bounds of all selected wires
  for(Wire *pw = Wires->first(); pw != 0; pw = Wires->next()) {
    pl = pw->Label;
    if(pl) if(pl->isSelected) {
      if(pl->x1 < x1) x1 = pl->x1;
      if(pl->y1-pl->y2 < y1) y1 = pl->y1-pl->y2;
      if(pl->x1+pl->x2 > x2) x2 = pl->x1+pl->x2;
      if(pl->y1 > y2) y2 = pl->y1;
      ElementCache->append(pl);
    }
  }

  for(Node *pn = Nodes->first(); pn != 0; pn = Nodes->next()) {
    pl = pn->Label;
    if(pl) if(pl->isSelected) {
      if(pl->x1 < x1) x1 = pl->x1;
      if(pl->y1-pl->y2 < y1) y1 = pl->y1-pl->y2;
      if(pl->x1+pl->x2 > x2) x2 = pl->x1+pl->x2;
      if(pl->y1 > y2) y2 = pl->y1;
      ElementCache->append(pl);
      pl->pOwner->Label = 0;   // erase connection
      pl->pOwner = 0;
    }
  }
}


/* *******************************************************************
   *****                                                         *****
   *****                Actions with paintings                   *****
   *****                                                         *****
   ******************************************************************* */

Painting* Schematic::selectedPainting(int x, int y)
{
  // test all paintings
  for(Painting *pp = Paintings->first(); pp != 0; pp = Paintings->next())
    if(pp->getSelected(x,y))
      return pp;

  return 0;
}

// ---------------------------------------------------
void Schematic::copyPaintings(int& x1, int& y1, int& x2, int& y2,
			QPtrList<Element> *ElementCache)
{
  Painting *pp;
  int bx1, by1, bx2, by2;
  // find boundings of all selected paintings
  for(pp = Paintings->first(); pp != 0; )
    if(pp->isSelected) {
      pp->Bounding(bx1, by1, bx2, by2);
      if(bx1 < x1) x1 = bx1;
      if(bx2 > x2) x2 = bx2;
      if(by1 < y1) y1 = by1;
      if(by2 > y2) y2 = by2;

      ElementCache->append(pp);
      Paintings->take();
      pp = Paintings->current();
    }
    else pp = Paintings->next();
}