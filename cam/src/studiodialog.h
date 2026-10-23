// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include "studio.h"
#include <QDialog>
class StudioDialog:public QDialog {
 public: StudioDialog(Studio &studio,Audio &audio,QWidget *parent=nullptr);
};
