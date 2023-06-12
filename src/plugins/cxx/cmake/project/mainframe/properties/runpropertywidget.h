/*
 * Copyright (C) 2022 Uniontech Software Technology Co., Ltd.
 *
 * Author:     huangyu<huangyub@uniontech.com>
 *
 * Maintainer: huangyu<huangyub@uniontech.com>
 *             zhouyi<zhouyi1@uniontech.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#ifndef RUNPROPERTYWIDGET_H
#define RUNPROPERTYWIDGET_H

#include "configutil.h"
#include "common/widget/pagewidget.h"

class QStandardItem;
class RunPropertyWidgetPrivate;
class RunPropertyWidget : public PageWidget
{
    Q_OBJECT
public:
    explicit RunPropertyWidget(const dpfservice::ProjectInfo &projectInfo, QStandardItem *item, QWidget *parent = nullptr);
    virtual ~RunPropertyWidget() override;

    void saveConfig() override;
    void readConfig() override;

private:
    void setupUi();
    void updateData();

    RunPropertyWidgetPrivate *const d;
};

#endif // RUNPROPERTYWIDGET_H