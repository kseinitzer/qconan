#include "conanplugin.h"
#include "BuildInfo.h"
#include "conanconstants.h"
#include <QAction>
#include <QDebug>
#include <QMainWindow>
#include <QMenu>
#include <QMessageBox>
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

    BuildInfo conanPlugin::conanInstall(
        const QString& pathToConanFile, const QDir& directory)
    {
      const QString conanBinPath = QStringLiteral("conan");
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
          process.runBlocking(conanBinPath, conanInstall);
      if (response.result != Utils::SynchronousProcessResponse::Finished)
        return {};

      _lastInstallDir = directory.canonicalPath();

      return BuildInfo::fromJsonFile(
          directory.filePath(QStringLiteral("conanbuildinfo.json")));
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

      connect(ProjectExplorer::ProjectTree::instance(),
          &ProjectExplorer::ProjectTree::currentProjectChanged, this,
          &conanPlugin::setNewProject);

      connect(&_conanFileWatcher, &QFileSystemWatcher::fileChanged, this,
          &conanPlugin::setupBuildDirForce);

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
      disconnect(ProjectExplorer::ProjectTree::instance(),
          &ProjectExplorer::ProjectTree::currentProjectChanged, this,
          &conanPlugin::setNewProject);

      disconnect(&_conanFileWatcher, &QFileSystemWatcher::fileChanged, this,
          &conanPlugin::setupBuildDirForce);

      for (const auto& con : _depConnections)
        disconnect(con);
      _depConnections.clear();

      // Hide UI (if you add UI that is not in the main window directly)
      return SynchronousShutdown;
    }

    void conanPlugin::setNewProject(ProjectExplorer::Project* project)
    {
      if (project == nullptr)
      {
        removeDepConnections();
        return;
      }

      const QString settingsPath = QDir(project->projectDirectory().toString())
                                       .filePath(QStringLiteral("qconan.ini"));
      QSettings settings(settingsPath, QSettings::Format::IniFormat);

      if (QFileInfo(settingsPath).exists())
      {
        write(tr("Found settings file"));
      }

      _config =
          PluginConfig(settings.value(QStringLiteral("global/path")).toString(),
              settings.value(QStringLiteral("global/installFlags")).toString());

      if (const auto path = conanFilePath(); !path.isEmpty())
      {
        _conanFileWatcher.removePaths(_conanFileWatcher.files());
        _conanFileWatcher.addPath(path);
      }
      updateDepConnections();
    }

    void conanPlugin::removeDepConnections()
    {
      for (const auto& con : _depConnections)
        disconnect(con);
      _depConnections.clear();
    }

    void conanPlugin::updateDepConnections()
    {
      removeDepConnections();

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
                this, &conanPlugin::setupBuildDirForce));
            setupBuildDir(false);
          }
        }
      }
    }

    void conanPlugin::setupBuildDirForce()
    {
      if (auto conanPath = conanFilePath();
          !_conanFileWatcher.files().contains(conanPath))
        _conanFileWatcher.addPath(conanPath);
      setupBuildDir(true);
    }

    void conanPlugin::setupBuildDir(bool forceInstall)
    {
      const QString buildPath = currentBuildDir();
      if (buildPath.isEmpty())
      {
        write(tr("No valid build directory available. Abort set up build "
                 "directory!"));
        return;
      }

      QDir buildDir;
      if (!buildDir.mkpath(buildPath))
      {
        write(tr("Unable to create directory >%1<.").arg(buildPath));
        return;
      }

      if (const QString conanPath = conanFilePath();
          !conanPath.isEmpty() &&
          (forceInstall == true || _lastInstallDir != buildPath))
      {
        const BuildInfo buildInfo = conanInstall(conanPath, buildPath);
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
#if QTCREATOR_MAJOR == 4 && QTCREATOR_MINOR < 8
        QDir rootDir(project->projectDirectory().toString());
#else
        QDir rootDir(project->rootProjectDirectory().toString());
#endif
        for (const auto& relPath :
            {QStringLiteral("conanfile.py"), QStringLiteral("../conanfile.py")})
        {
          if (rootDir.exists(relPath))
          {
            write(tr("Found conanfile at >%1<").arg(relPath));
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
