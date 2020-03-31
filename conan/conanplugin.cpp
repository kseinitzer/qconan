#include "conanplugin.h"
#include "conanconstants.h"

#include <QAction>
#include <QDebug>
#include <QJsonDocument>
#include <QMainWindow>
#include <QMenu>
#include <QMessageBox>
#include <QTemporaryDir>
#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/icontext.h>
#include <coreplugin/icore.h>
#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/kitmanager.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projecttree.h>
#include <projectexplorer/target.h>
#include <utils/environment.h>
#include <utils/synchronousprocess.h>

namespace {
  const QString kitName = QStringLiteral("conan kit");
};

namespace conan {
  namespace Internal {

    conanPlugin::conanPlugin()
    {
      _pathSeparator = Utils::OsSpecificAspects::pathListSeparator(
          Utils::HostOsInfo::hostOs());
    }

    conanPlugin::~conanPlugin()
    {
      // Unregister objects from the plugin manager's object pool
      // Delete members
    }

    QVariantMap conanPlugin::conanInstall(
        const QString& pathToConanFile, const QDir& directory) const
    {
      const QString conanPath = QStringLiteral("conan");
      QStringList conanInstall = {QStringLiteral("install"), pathToConanFile,
          QStringLiteral("-g"), QStringLiteral("json")};

      qInfo() << "Run conan install for " << pathToConanFile
              << directory.path();

      Utils::SynchronousProcess process;
      process.setWorkingDirectory(directory.path());
      process.setTimeoutS(5); // TODO: Must run completly asynchron because
                              // downloads may take a lot of time
      Utils::SynchronousProcessResponse response =
          process.runBlocking({conanPath, conanInstall});
      if (response.result != Utils::SynchronousProcessResponse::Finished)
        return {};

      QFile buildInfo(
          directory.filePath(QStringLiteral("conanbuildinfo.json")));
      if (!buildInfo.open(QIODevice::ReadOnly))
      {
        qDebug() << "could not open buildinfo " << buildInfo.fileName();
        return {};
      }

      QJsonParseError err;
      auto doc = QJsonDocument::fromJson(buildInfo.readAll(), &err);

      if (err.error != QJsonParseError::NoError)
      {
        qDebug() << "Json Parse Error: " << err.errorString();
        return {};
      }
      return doc.object().toVariantMap();
    }

    bool conanPlugin::initialize(
        const QStringList& arguments, QString* errorString)
    {
      // Register objects in the plugin manager's object pool
      // Load settings
      // Add actions to menus
      // Connect to other plugins' signals
      // In the initialize function, a plugin can be sure that the plugins it
      // depends on have initialized their members.

      Q_UNUSED(arguments)
      Q_UNUSED(errorString)

      auto prjTree = ProjectExplorer::ProjectTree::instance();
      connect(prjTree, &ProjectExplorer::ProjectTree::currentProjectChanged,
          this, &conanPlugin::updateDepConnections);

      return true;
    }

    void conanPlugin::extensionsInitialized()
    {
      // Retrieve objects from the plugin manager's object pool
      // In the extensionsInitialized function, a plugin can be sure that all
      // plugins that depend on it are completely initialized.
    }

    ExtensionSystem::IPlugin::ShutdownFlag conanPlugin::aboutToShutdown()
    {
      // Save settings
      // Disconnect from signals that are not needed during shutdown
      // Hide UI (if you add UI that is not in the main window directly)
      return SynchronousShutdown;
    }

    void conanPlugin::updateDepConnections()
    {
      qDebug() << "update depConnections";

      for (const auto& con : _depConnections)
        disconnect(con);
      _depConnections.clear();

      auto tree = ProjectExplorer::ProjectTree::instance();

      if (auto project = tree->currentProject(); project)
      {
        _depConnections.push_back(
            connect(project, &ProjectExplorer::Project::activeTargetChanged,
                this, &conanPlugin::updateDepConnections));
        if (auto target = project->activeTarget(); target)
        {
          _depConnections.push_back(connect(target,
              &ProjectExplorer::Target::activeBuildConfigurationChanged, this,
              &conanPlugin::updateDepConnections));
          if (auto buildConfig = target->activeBuildConfiguration();
              buildConfig)
          {
            _depConnections.push_back(connect(buildConfig,
                &ProjectExplorer::BuildConfiguration::buildDirectoryChanged,
                this, &conanPlugin::evaluateDependencies));
            evaluateDependencies();
          }
        }
      }
    }

    void conanPlugin::evaluateDependencies()
    {
      qDebug() << "evaluateDependency triggered!";

      const QString buildPath = currentBuildDir();
      if (buildPath.isEmpty())
      {
        qWarning() << "No valid build directory available. Abort conan "
                      "dependency discovery";
        return;
      }

      QDir buildDir;
      if (!buildDir.mkpath(buildPath))
      {
        qCritical() << "Unable to create directory. Please check access rights!"
                    << buildPath;
        return;
      }

      const QString conanFilePath = QStringLiteral(
          "/home/kseinitzer/prj/qconan/integrationTest/app/conanfile.py");

      const QVariantMap buildInfo = conanInstall(conanFilePath, buildPath);
    }

    QString conanPlugin::currentBuildDir() const
    {
      auto tree = ProjectExplorer::ProjectTree::instance();

      // ToDo: On Project, target and buildConfig level we need to check if
      // signals are available to detect if the specicific element was
      // changed! e.g. buildDirectoryChanged(),
      // activeBuildConfigurationChanged()
      if (auto project = tree->currentProject(); project)
      {
        if (auto target = project->activeTarget(); target)
        {
          if (auto buildConfig = target->activeBuildConfiguration();
              buildConfig)
            return buildConfig->buildDirectory().toString();
        }
      }

      return {};
    }

  } // namespace Internal
} // namespace conan
