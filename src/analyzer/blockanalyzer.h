/*
   Strawberry Music Player
   This file was part of Amarok.
   Copyright 2003-2005, Max Howell <max.howell@methylblue.com>
   Copyright 2005, Mark Kretschmann <markey@web.de>
   Copyright 2009-2010, David Sansome <davidsansome@gmail.com>
   Copyright 2010, 2014, John Maguire <john.maguire@gmail.com>

   Strawberry is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   Strawberry is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with Strawberry.  If not, see <http://www.gnu.org/licenses/>.

 */

#ifndef BLOCKANALYZER_H
#define BLOCKANALYZER_H
#include <array>
#include "analyzerbase.h"
class BlockAnalyzer : public AnalyzerBase {
  Q_OBJECT
 public:
  Q_INVOKABLE explicit BlockAnalyzer(QWidget *parent);
  static const char *kName;
 protected:
  void transform(Scope &s) override;
  void analyze(QPainter &p, const Scope &s, bool new_frame) override;
  void demo(QPainter &p) override;
 private:
  std::array<float, 32> levels_{};
  std::array<float, 32> peaks_{};
};
#endif
