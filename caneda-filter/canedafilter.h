/***************************************************************************
                                canedafilter.h
                             -------------------
    begin                : Wed Mar 02 2005
    copyright            : (C) 2005 by Michael Margraf
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

#ifndef CANEDAFILTER_H
#define CANEDAFILTER_H

#include <QtGui/QWidget>

class QGridLayout;
class QComboBox;
class QLineEdit;
class QLabel;
class QIntValidator;
class QDoubleValidator;


struct tCanedaSettings {
  int x, y;      // position of main window
  QFont font;
  QString LangDir;
  QString BitmapDir;
  QString Language;
};

extern struct tCanedaSettings CanedaSettings;

class CanedaFilter : public QWidget
{
  Q_OBJECT
public:
  CanedaFilter();
 ~CanedaFilter();

private slots:
  void slotHelpIntro();
  void slotHelpAbout();
  void slotHelpAboutQt();
  void slotCalculate();
  void slotTypeChanged(int);
  void slotClassChanged(int);
  void slotShowResult();

private:
  void setError(const QString&);
  QString * calculateFilter(struct tFilter *);

  int ResultState;

  QComboBox *ComboType, *ComboClass, *ComboCorner, *ComboStop, *ComboBandStop;
  QLineEdit *EditOrder, *EditCorner, *EditStop, *EditRipple, *EditImpedance;
  QLineEdit *EditAtten, *EditBandStop;
  QLabel *LabelRipple, *LabelRipple_dB, *LabelStart, *LabelStop, *LabelResult;
  QLabel *LabelAtten, *LabelAtten_dB, *LabelBandStop, *LabelOrder;
};

#endif // CANEDAFILTER_H
