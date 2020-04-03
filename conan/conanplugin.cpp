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
#include <coreplugin/messagemanager.h>
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

      if (!_config.installFlags().isEmpty())
      {
        conanInstall.push_back(_config.installFlags());
      }

      write(tr("Run conan %1 for >%2< in >%3<")
                .arg(conanInstall.join(" "))
                .arg(pathToConanFile)
                .arg(directory.path()));

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
        write(tr("Error, could not open buildinfo: %1")
                  .arg(buildInfo.fileName()));
        return {};
      }

      QJsonParseError err;
      auto doc = QJsonDocument::fromJson(buildInfo.readAll(), &err);

      if (err.error != QJsonParseError::NoError)
      {
        write(tr("Error, JSON file could not be parsed: %1")
                  .arg(err.errorString()));
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
          this, &conanPlugin::setNewProject);

      connect(&_conanFileWatcher, &QFileSystemWatcher::fileChanged, this,
          &conanPlugin::evaluateDependencies);

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

    void conanPlugin::setNewProject(ProjectExplorer::Project* project)
    {
      if (project)
      {
        const QString settingsPath =
            QDir(project->projectDirectory().toString())
                .filePath(QStringLiteral("qconan.ini"));
        QSettings settings(settingsPath, QSettings::Format::IniFormat);

        _config = PluginConfig(
            settings.value(QStringLiteral("global/path")).toString(),
            settings.value(QStringLiteral("global/installFlags")).toString());
      }
      if (const auto path = conanFilePath(); !path.isEmpty())
      {
        _conanFileWatcher.removePaths(_conanFileWatcher.files());
        _conanFileWatcher.addPath(path);
      }
      updateDepConnections();
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

      if (const QString conanPath = conanFilePath(); !conanPath.isEmpty())
      {
        const QVariantMap buildInfo = conanInstall(conanPath, buildPath);
      }
      return;
    }

    QString conanPlugin::conanFilePath() const
    {
      if (!_config.isAutoDetect())
      {
        if (QFileInfo(_config.conanFile()).exists())
          return _config.conanFile();
        return {};
      }

      if (auto project =
              ProjectExplorer::ProjectTree::instance()->currentProject();
          project)
      {
        QDir rootDir(project->rootProjectDirectory().toString());
        for (const auto& relPath :
            {QStringLiteral("conanfile.py"), QStringLiteral("../conanfile.py")})
        {
          if (rootDir.exists(relPath))
          {
            qDebug() << "Found conanfile" << relPath;
            return rootDir.filePath(relPath);
          }
        }
      }
      return {};
    }

    void conanPlugin::write(const QString& text) const
    {
      auto messenger = Core::MessageManager::instance();
      messenger->write(tr("conan plugin: %1").arg(text));
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
