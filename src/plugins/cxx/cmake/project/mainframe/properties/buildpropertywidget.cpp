// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "buildpropertywidget.h"

#include "environmentwidget.h"
#include "stepspane.h"
#include "targetsmanager.h"

#include <DTabWidget>
#include <DPushButton>
#include <DFileDialog>
#include <DStackedWidget>
#include <DComboBox>
#include <DButtonBox>
#include <DFrame>

#include <QVBoxLayout>
#include <QFormLayout>

#include "common/common.h"

const int widthPerBtn = 80;

DWIDGET_USE_NAMESPACE
using namespace config;

class DetailPropertyWidgetPrivate
{
    friend class DetailPropertyWidget;

    StepsPane *buildStepsPane{nullptr};
    StepsPane *cleanStepsPane{nullptr};
    EnvironmentWidget *envWidget{nullptr};
};

DetailPropertyWidget::DetailPropertyWidget(QWidget *parent)
    : ConfigureWidget(parent)
    , d(new DetailPropertyWidgetPrivate())
{   
    setBackgroundRole(QPalette::Window);
    setFrameShape(QFrame::Shape::NoFrame);

    d->buildStepsPane = new StepsPane(this);
    d->cleanStepsPane = new StepsPane(this);
    d->envWidget = new EnvironmentWidget(this);

    DStackedWidget *stackWidget = new DStackedWidget(this);
    stackWidget->insertWidget(0, d->buildStepsPane);
    stackWidget->insertWidget(1, d->cleanStepsPane);
    stackWidget->insertWidget(2, d->envWidget);

    DButtonBoxButton *btnBuild = new DButtonBoxButton(QObject::tr("Build Steps"), this);
    btnBuild->setCheckable(true);
    btnBuild->setChecked(true);
    DButtonBoxButton *btnClean = new DButtonBoxButton(QObject::tr("Clean Steps"), this);
    DButtonBoxButton *btnEnv = new DButtonBoxButton(QObject::tr("Runtime Env"), this);

    DButtonBox *btnbox = new DButtonBox(this);
    QList<DButtonBoxButton *> list { btnBuild, btnClean, btnEnv };
    btnbox->setButtonList(list, true);

    auto frame = new DWidget(this);
    auto layout = new QVBoxLayout(this);
    layout->setAlignment(Qt::AlignHCenter);
    layout->addWidget(btnbox);
    frame->setLayout(layout);

    connect(btnbox, &DButtonBox::buttonClicked, this, [=](QAbstractButton *button) {
        if (button == btnBuild)
            stackWidget->setCurrentIndex(0);
        else if (button == btnClean)
            stackWidget->setCurrentIndex(1);
        else if (button == btnEnv)
            stackWidget->setCurrentIndex(2);
    });

    addWidget(frame);
    addWidget(stackWidget);
}

DetailPropertyWidget::~DetailPropertyWidget()
{
    if (d)
        delete d;
}

void DetailPropertyWidget::setValues(const BuildConfigure &configure)
{
    for(auto iter = configure.steps.begin(); iter != configure.steps.end(); ++iter) {
        if (iter->type == Build)
            d->buildStepsPane->setValues(*iter);
        else if (iter->type == Clean)
            d->cleanStepsPane->setValues(*iter);
    }

    d->envWidget->setValues(configure.env);
}

void DetailPropertyWidget::getValues(BuildConfigure &configure)
{
    for(auto iter = configure.steps.begin(); iter != configure.steps.end(); ++iter) {
        if (iter->type == Build)
            d->buildStepsPane->getValues(*iter);
        else if (iter->type == Clean)
            d->cleanStepsPane->getValues(*iter);
    }

    d->envWidget->getValues(configure.env);
}

class BuildPropertyWidgetPrivate
{
    friend class BuildPropertyWidget;

    DComboBox *configureComboBox{nullptr};
    DLineEdit *outputDirEdit{nullptr};

    DStackedWidget* stackWidget{nullptr};
    dpfservice::ProjectInfo projectInfo;

    QMap<StepType, dpfservice::TargetType> typeMap = {{StepType::Build, dpfservice::TargetType::kBuildTarget},
                                                      {StepType::Build, dpfservice::TargetType::kBuildTarget}};
};

BuildPropertyWidget::BuildPropertyWidget(const dpfservice::ProjectInfo &projectInfo, QWidget *parent)
    : PageWidget(parent)
    , d(new BuildPropertyWidgetPrivate())
{
    d->projectInfo = projectInfo;
    setupOverviewUI();
    initData(projectInfo);

    QObject::connect(TargetsManager::instance(), &TargetsManager::initialized,
                     this, &BuildPropertyWidget::updateDetail);
}

BuildPropertyWidget::~BuildPropertyWidget()
{
    if (d)
        delete d;
}

void BuildPropertyWidget::setupOverviewUI()
{
    QVBoxLayout *vLayout = new QVBoxLayout();
    ConfigureWidget *buildCfgWidget = new ConfigureWidget(this);
    buildCfgWidget->setFrameShape(QFrame::Shape::NoFrame);
    vLayout->addWidget(buildCfgWidget);
    setLayout(vLayout);

    QVBoxLayout *overviewLayout = new QVBoxLayout();
    DWidget *overviewWidget = new DWidget();
    overviewWidget->setLayout(overviewLayout);

    QHBoxLayout *configureLayout = new QHBoxLayout();
    d->configureComboBox = new DComboBox(this);
    configureLayout->addWidget(d->configureComboBox);
    configureLayout->setSpacing(10);
    configureLayout->addStretch();
    QObject::connect(d->configureComboBox, QOverload<int>::of(&DComboBox::currentIndexChanged), [=](int index){
        QVariant var = d->configureComboBox->itemData(index, Qt::UserRole + 1);
        if (var.isValid()) {
            QString directory = var.value<QString>();
            if (d->outputDirEdit) {
                d->outputDirEdit->setText(directory);
            }
        }

        var = d->configureComboBox->itemData(index, Qt::UserRole + 2);
        if (var.isValid()) {
            DetailPropertyWidget *detail = var.value<DetailPropertyWidget *>();
            if (detail && d->stackWidget) {
                d->stackWidget->setCurrentWidget(detail);
            }
        }

        ConfigureParam *param = ConfigUtil::instance()->getConfigureParamPointer();
        param->tempSelType = ConfigUtil::instance()->getTypeFromName(d->configureComboBox->currentText());
        ConfigUtil::instance()->checkConfigInfo(d->configureComboBox->currentText(), d->outputDirEdit->text());
    });

    QHBoxLayout *hLayout = new QHBoxLayout();
    d->outputDirEdit = new DLineEdit(this);
    d->outputDirEdit->lineEdit()->setReadOnly(true);
    auto button = new QPushButton(this);
    button->setText(tr("Browse"));
    connect(button, &QPushButton::clicked, [this](){
        QString outputDirectory = QFileDialog::getExistingDirectory(this, "Output directory", d->outputDirEdit->text());
        if (!outputDirectory.isEmpty()) {
            QString oldDir = d->outputDirEdit->text();
            d->outputDirEdit->setText(outputDirectory.toUtf8());
            int index = d->configureComboBox->currentIndex();
            d->configureComboBox->setItemData(index, QVariant::fromValue(outputDirectory.toUtf8()), Qt::UserRole + 1);

            if (outputDirectory != oldDir) {
                ConfigUtil::instance()->checkConfigInfo(d->configureComboBox->currentText(), d->outputDirEdit->text());
            }
        }
    });

    hLayout->addWidget(d->outputDirEdit);
    hLayout->addWidget(button);
    hLayout->setSpacing(10);

    overviewLayout->setSpacing(0);
    overviewLayout->setMargin(0);
    overviewLayout->setSpacing(5);

    auto formlayout = new QFormLayout(this);
    formlayout->addRow(QLabel::tr("Build configuration:"), configureLayout);
    formlayout->addRow(tr("Output direcotry:"), hLayout);

    overviewLayout->addLayout(formlayout);
    buildCfgWidget->addWidget(overviewWidget);
    d->stackWidget = new DStackedWidget(this);
    buildCfgWidget->addWidget(d->stackWidget);
}

void BuildPropertyWidget::initData(const dpfservice::ProjectInfo &projectInfo)
{
    d->configureComboBox->clear();

    ConfigureParam *param = ConfigUtil::instance()->getConfigureParamPointer();
    ConfigUtil::instance()->readConfig(ConfigUtil::instance()->getConfigPath(projectInfo.workspaceFolder()), *param);

    auto iter = param->buildConfigures.begin();
    int index = 0;
    for (; iter != param->buildConfigures.end(); ++iter, index++) {
        for (auto iterStep = iter->steps.begin(); iterStep != iter->steps.end(); ++iterStep) {
            if (iterStep->targetList.isEmpty()) {
                iterStep->targetList = TargetsManager::instance()->getTargetNamesList();
                dpfservice::TargetType targetType = dpfservice::TargetType::kUnknown;
                if (iterStep->type == Build) {
                    targetType = dpfservice::TargetType::kBuildTarget;
                } else if (iterStep->type == Clean) {
                    targetType = dpfservice::TargetType::kCleanTarget;
                }
                dpfservice::Target target = TargetsManager::instance()->getActivedTargetByTargetType(targetType);
                iterStep->targetName = target.buildTarget;
            }
        }

        DetailPropertyWidget *detailWidget = new DetailPropertyWidget();
        detailWidget->setValues(*iter);
        d->stackWidget->insertWidget(index, detailWidget);

        d->configureComboBox->insertItem(index, ConfigUtil::instance()->getNameFromType(iter->type));
        d->configureComboBox->setItemData(index, QVariant::fromValue(iter->directory), Qt::UserRole + 1);
        d->configureComboBox->setItemData(index, QVariant::fromValue(detailWidget), Qt::UserRole + 2);
        if (param->defaultType == iter->type) {
            d->configureComboBox->setCurrentIndex(index);
            d->outputDirEdit->setText(iter->directory);
            d->stackWidget->setCurrentWidget(detailWidget);
        }

        initRunConfig(iter->directory, iter->runConfigure);
    }

}

void BuildPropertyWidget::updateDetail()
{
    ConfigureParam *param = ConfigUtil::instance()->getConfigureParamPointer();

    QString currentTypeString = d->configureComboBox->currentText();
    auto iter = param->buildConfigures.begin();
    for (; iter != param->buildConfigures.end(); ++iter) {
        if (iter->type == ConfigUtil::instance()->getTypeFromName(currentTypeString)) {
            for (auto iterStep = iter->steps.begin(); iterStep != iter->steps.end(); ++iterStep) {
                iterStep->targetList = TargetsManager::instance()->getTargetNamesList();
                dpfservice::Target target = TargetsManager::instance()->getActivedTargetByTargetType(d->typeMap.value(iterStep->type));
                iterStep->targetName = target.buildTarget;
            }

            DetailPropertyWidget *detail = dynamic_cast<DetailPropertyWidget *>(d->stackWidget->currentWidget());
            if (detail) {
                detail->setValues(*iter);
            }

            break;
        }

        initRunConfig(iter->directory, iter->runConfigure);
    }
}

void BuildPropertyWidget::initRunConfig(const QString &workDirectory, RunConfigure &runConfigure)
{
    if (runConfigure.params.isEmpty()) {
        QStringList exeTargetList = TargetsManager::instance()->getExeTargetNamesList();
        foreach (auto targetName, exeTargetList) {
            RunParam param;
            QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
            foreach (auto key, env.keys()) {
                param.env.environments.insert(key, env.value(key));
            }
            param.targetName = targetName;
            dpfservice::Target target = TargetsManager::instance()->getTargetByName(targetName);
            param.targetPath = target.output;

            runConfigure.params.push_back(param);
        }

        dpfservice::Target target = TargetsManager::instance()->getActivedTargetByTargetType(dpfservice::TargetType::kActiveExecTarget);
        runConfigure.defaultTargetName = target.buildTarget;
    }
}

void BuildPropertyWidget::readConfig()
{

}

void BuildPropertyWidget::saveConfig()
{
    ConfigureParam *param = ConfigUtil::instance()->getConfigureParamPointer();
    auto iter = param->buildConfigures.begin();
    int index = 0;
    for (; iter != param->buildConfigures.end(); ++iter) {
        DetailPropertyWidget *detailWidget = dynamic_cast<DetailPropertyWidget *>(d->stackWidget->widget(index));
        if (detailWidget) {
            detailWidget->getValues(*iter);
        }

        for (int i = 0; i < d->configureComboBox->count(); i++) {
            ConfigType type = ConfigUtil::instance()->getTypeFromName(d->configureComboBox->itemText(i));
            if (type == iter->type) {
                QVariant var = d->configureComboBox->itemData(index, Qt::UserRole + 1);
                if (var.isValid()) {
                    iter->directory = var.value<QString>();
                }

                if (d->configureComboBox->currentIndex() == i)
                    param->defaultType = type;

                break;
            }
        }

        index++;
    }
}
