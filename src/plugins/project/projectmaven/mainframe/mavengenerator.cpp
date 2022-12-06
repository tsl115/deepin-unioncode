﻿/*
 * Copyright (C) 2022 Uniontech Software Technology Co., Ltd.
 *
 * Author:     huangyu<huangyub@uniontech.com>
 *
 * Maintainer: huangyu<huangyub@uniontech.com>
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
#include "mavengenerator.h"
#include "mavenasynparse.h"

#include "properties/configpropertywidget.h"
#include "services/window/windowservice.h"
#include "services/builder/builderservice.h"

#include "common/dialog/propertiesdialog.h"

#include <QtXml>
#include <QFileIconProvider>

class MavenGeneratorPrivate
{
    friend class MavenGenerator;
    QStandardItem* configureRootItem {nullptr};
    QMenu *mavenMenu {nullptr};
    QHash<QStandardItem*, MavenAsynParse*> projectParses {};
};

MavenGenerator::MavenGenerator()
    : d(new MavenGeneratorPrivate())
{
    using namespace dpfservice;
    auto &ctx = dpfInstance.serviceContext();
    BuilderService *builderService = ctx.service<BuilderService>(BuilderService::name());
    if (!builderService) {
        qCritical() << "Failed, not found service : builderService";
        abort();
    }
}

MavenGenerator::~MavenGenerator()
{
    qInfo() << __FUNCTION__;

    if (d)
        delete d;
}

QStringList MavenGenerator::supportLanguages()
{
    return {dpfservice::MWMFA_JAVA};
}

QStringList MavenGenerator::supportFileNames()
{
    return {"pom.xml"};
}

QDialog *MavenGenerator::configureWidget(const QString &language,
                                         const QString &workspace)
{
    using namespace dpfservice;

    // get config result.
    ProjectInfo info;;
    info.setLanguage(language);
    info.setKitName(MavenGenerator::toolKitName());
    info.setWorkspaceFolder(workspace);

    configure(info);

    return nullptr;
}

bool MavenGenerator::configure(const dpfservice::ProjectInfo &info)
{
    dpfservice::ProjectGenerator::configure(info);

    auto root = createRootItem(info);
    using namespace dpfservice;
    auto &ctx = dpfInstance.serviceContext();
    ProjectService *projectService = ctx.service<ProjectService>(ProjectService::name());
    WindowService *windowService = ctx.service<WindowService>(WindowService::name());
    if (projectService && windowService && root) {
        projectService->projectView.addRootItem(root);
        projectService->projectView.expandedDepth(root, 1);
        windowService->switchWidgetNavigation(MWNA_EDIT);
        windowService->switchWidgetWorkspace(MWCWT_PROJECTS);
    }

    return true;
}

QStandardItem *MavenGenerator::createRootItem(const dpfservice::ProjectInfo &info)
{
    using namespace dpfservice;
    QStandardItem *rootItem = ProjectGenerator::createRootItem(info);
    dpfservice::ProjectInfo::set(rootItem, info);
    d->projectParses[rootItem] = new MavenAsynParse();
    QObject::connect(d->projectParses[rootItem], &MavenAsynParse::itemsModified,
                     this, &MavenGenerator::itemModified);
    QMetaObject::invokeMethod(d->projectParses[rootItem], "parseProject",
                              Q_ARG(const dpfservice::ProjectInfo &, info));
    return rootItem;
}

void MavenGenerator::removeRootItem(QStandardItem *root)
{
    if (!root)
        return;
    auto parser = d->projectParses[root];

    while (root->hasChildren()) {
        root->takeRow(0);
    }
    d->projectParses.remove(root);

    delete root;

    if (parser)
        delete parser;
}

QMenu *MavenGenerator::createItemMenu(const QStandardItem *item)
{
    if (item->parent())
        return nullptr;

    QMenu *menu = new QMenu();
    if (!d->mavenMenu) {
        d->mavenMenu = new QMenu("Maven", menu);
        QObject::connect(d->mavenMenu, &QMenu::destroyed,
                         [=](){ d->mavenMenu = nullptr; });
    }

    menu->addMenu(d->mavenMenu);

    // asyn parse
    using namespace dpfservice;

    QStandardItem *itemTemp = const_cast<QStandardItem *>(item);
    if (!itemTemp)
        return d->mavenMenu;

    auto parse = d->projectParses[itemTemp];
    if (!parse)
        return d->mavenMenu;

    ProjectInfo info = ProjectInfo::get(item);
    if (info.isEmpty())
        return d->mavenMenu;

    // add menu generat call back
    QObject::connect(parse, &MavenAsynParse::parsedActions,
                     this, &MavenGenerator::doAddMavenMeue,
                     Qt::QueuedConnection);
    // execute logic
    parse->parseActions(info);

    QAction *action = new QAction("Properties");
    menu->addAction(action);
    QObject::connect(action, &QAction::triggered, [=](){
        actionProperties(info, itemTemp);
    });

    return menu;
}

void MavenGenerator::itemModified(const QList<QStandardItem *> &items)
{
    MavenAsynParse *parse = qobject_cast<MavenAsynParse*>(sender());
    if (parse) {
        auto root = d->projectParses.key(parse);
        emit itemChanged(root, items);
    }
}

void MavenGenerator::doAddMavenMeue(const dpfservice::ProjectActionInfos &infos)
{
    if (d->mavenMenu) {
        for (auto actionInfo : infos) {
            QAction *action = new QAction(actionInfo.displyText, d->mavenMenu);
            dpfservice::ProjectMenuActionInfo::set(action, actionInfo);
            d->mavenMenu->addAction(action);
            QObject::connect(action, &QAction::triggered,
                             this, &MavenGenerator::doActionTriggered,
                             Qt::UniqueConnection);
        }
    }
}

void MavenGenerator::doActionTriggered()
{
    QAction *action = qobject_cast<QAction*>(sender());
    if (action) {
        auto value = dpfservice::ProjectMenuActionInfo::get(action);

        auto &ctx = dpfInstance.serviceContext();
        auto builderService = ctx.service<dpfservice::BuilderService>(dpfservice::BuilderService::name());
        if (builderService) {
            BuildCommandInfo commandInfo;
            commandInfo.kitName = toolKitName();
            commandInfo.program = value.buildProgram;
            commandInfo.arguments = value.buildArguments;
            commandInfo.workingDir = QFileInfo(value.workingDirectory).path();
            builderService->interface.builderCommand(commandInfo);
        }
    }
}

void MavenGenerator::actionProperties(const dpfservice::ProjectInfo &info, QStandardItem *item)
{
    PropertiesDialog dlg;
    ConfigPropertyWidget *property = new ConfigPropertyWidget(info, item);
    dlg.insertPropertyPanel("Config", property);
    dlg.exec();
}
