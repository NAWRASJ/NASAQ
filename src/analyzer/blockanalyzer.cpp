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

#include "blockanalyzer.h"
#include <algorithm>
#include <cmath>
#include <QPalette>

const char *BlockAnalyzer::kName = QT_TRANSLATE_NOOP("AnalyzerContainer", "NASAQ spectrum");

BlockAnalyzer::BlockAnalyzer(QWidget *parent) : AnalyzerBase(parent, 9) {
  setMinimumSize(159, 9);
}

void BlockAnalyzer::transform(Scope &s) {
  fht_->spectrum(s.data());
  fht_->scale(s.data(), 1.0F / 20);
  s.resize(static_cast<size_t>(fht_->size() / 2));
}

void BlockAnalyzer::demo(QPainter &p) {
  // Silence stays still; never generate a decorative fake signal while stopped.
  analyze(p, Scope{}, new_frame_);
}

void BlockAnalyzer::analyze(QPainter &p, const Scope &s, const bool new_frame) {
  if (new_frame) {
    const float dt = std::clamp(timeout() / 1000.0F, 0.001F, 0.2F);
    for (size_t band = 0; band < levels_.size(); ++band) {
      float target = 0;
      if (is_playing_ && s.size() > 1) {
        // Logarithmic frequency bands, excluding DC, from the actual PCM spectrum.
        const auto boundary = [&](size_t i) {
          return std::min(s.size(), static_cast<size_t>(std::pow(double(s.size()), i / 32.0)));
        };
        const size_t begin = std::min(s.size() - 1, boundary(band));
        const size_t end = std::min(s.size(), std::max(begin + 1, boundary(band + 1)));
        float amplitude = 0;
        for (size_t bin = begin; bin < end; ++bin)
          if (std::isfinite(s[bin])) amplitude = std::max(amplitude, s[bin]);
        target = std::clamp(std::log1p(amplitude * 9.0F) / std::log(10.0F), 0.0F, 1.0F);
      }
      const float time_constant = target > levels_[band] ? 0.045F : 0.24F;
      levels_[band] += (target - levels_[band]) * (1.0F - std::exp(-dt / time_constant));
      if (levels_[band] < 0.001F) levels_[band] = 0;
      peaks_[band] = std::max(levels_[band], peaks_[band] - dt * 0.55F);
    }
  }
  p.fillRect(rect(), palette().color(QPalette::Window));
  p.setRenderHint(QPainter::Antialiasing);
  p.setPen(Qt::NoPen);
  const qreal margin = std::min(6.0, width() * 0.025);
  const qreal cell = std::max(0.1, (width() - 2 * margin) / 32.0);
  const qreal bar_width = std::max(1.0, cell * 0.55);
  const qreal baseline = std::max(2.0, height() - 3.0);
  const qreal available = std::max(0.0, baseline - 6.0);
  for (size_t band = 0; band < levels_.size(); ++band) {
    const qreal x = margin + band * cell + (cell - bar_width) / 2;
    const qreal h = std::max(2.0, levels_[band] * available);
    p.setBrush(QColor("#153e34"));
    p.drawRoundedRect(QRectF(x, baseline - h, bar_width, h), bar_width / 2, bar_width / 2);
    if (peaks_[band] > 0.025F && available > 8) {
      p.setBrush(QColor("#69774e"));
      const qreal peak_y = std::max(0.0, baseline - std::max(h, peaks_[band] * available) - 4.0);
      p.drawRoundedRect(QRectF(x, peak_y, bar_width, 2), 1, 1);
    }
  }
}
