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
#include <projectexplorer/environmentaspect.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/kitmanager.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projecttree.h>
#include <projectexplorer/runconfiguration.h>
#include <projectexplorer/target.h>
#include <utils/environment.h>

namespace {
  const QString kitName = QStringLiteral("conan kit");
};

#if QTCREATOR_MAJOR == 4 && QTCREATOR_MINOR < 11
namespace Utils {
  using EnvironmentItems = QList< EnvironmentItem >;
  using NameValueItem = EnvironmentItem;
} // namespace Utils
#endif
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
      QStringList installCommand = {QStringLiteral("install"), pathToConanFile,
          QStringLiteral("-g"), QStringLiteral("json")};

      if (!_config.installFlags().isEmpty())
      {
        installCommand.push_back(_config.installFlags());
      }

      write(tr("Run conan %1 for >%2< in >%3<")
                .arg(installCommand.join(" "))
                .arg(pathToConanFile)
                .arg(directory.path()));

      _lastInstallDir = directory.canonicalPath();

      _conanBin.setWorkingDirectory(directory.path());
      _conanBin.start(conanBinPath, installCommand);

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
      connect(&_settingsFileWatcher, &QFileSystemWatcher::fileChanged, this,
          &conanPlugin::setupBuildDirForce);

      connect(&_conanBin,
          QOverload< int, QProcess::ExitStatus >::of(&QProcess::finished), this,
          &conanPlugin::onInstallFinished);
      connect(&_conanBin, &QProcess::readyRead, [=]() {
        if (auto string = QString::fromLocal8Bit(_conanBin.readAll());
            !string.isEmpty())
          write(string);
      });
      _conanBin.setProcessChannelMode(
          QProcess::ProcessChannelMode::MergedChannels);

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

      disconnect(&_settingsFileWatcher, &QFileSystemWatcher::fileChanged, this,
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

      loadProjectConfiguration(project);

      if (const auto path = conanFilePath(); !path.isEmpty())
      {
        _conanFileWatcher.removePaths(_conanFileWatcher.files());
        watchConanfile(path);
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
      watchConanfile(conanFilePath());
      setupBuildDir(true);
    }

    void conanPlugin::setupBuildDir(bool forceInstall)
    {
      loadProjectConfiguration(
          ProjectExplorer::ProjectTree::instance()->currentProject());

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
        conanInstall(conanPath, buildPath);
      }
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

    ProjectExplorer::EnvironmentAspect*
    conanPlugin::runEnvironmentAspect() const
    {
      if (auto target = currentTarget(); target)
      {
        if (auto run = target->activeRunConfiguration(); run)
        {
#if QTCREATOR_MAJOR == 4 && QTCREATOR_MINOR < 8
          return run->extraAspect< ProjectExplorer::EnvironmentAspect >();
#else
          return run->aspect< ProjectExplorer::EnvironmentAspect >();
#endif
        }
      }
      return nullptr;
    }

    void conanPlugin::onInstallFinished(
        int exitCode, QProcess::ExitStatus exitStatus)
    {
      if (exitStatus != QProcess::ExitStatus::NormalExit)
      {
        write(tr("Abort conan install: The conan executable exited unexpected, "
                 "possibly crashed."));
        return;
      }
      if (exitCode != 0)
      {
        write(tr("Abort conan install: The conan executable returned an "
                 "unexpected return value: %1")
                  .arg(exitCode));
        return;
      }

      write(tr("Finished conan install, parse package information"));
      const QDir& directory(_lastInstallDir);
      const BuildInfo buildInfo = BuildInfo::fromJsonFile(
          directory.filePath(QStringLiteral("conanbuildinfo.json")));

      QStringList appendPath = buildInfo.environmentPath();
      if (_config.useLibraryPathAsEnvironmentPath())
      {
        appendPath += buildInfo.libraryPath();
        write(tr("Use library path information from conan"));
      }
      if (_config.useBinaryPathAsEnvironmentPath())
      {
        appendPath += buildInfo.binaryPath();
        write(tr("Use binary path information from conan"));
      }

      write(
          tr("Use path information from conan >%1<").arg(appendPath.join(":")));

      appendPath = appendPath.toSet().values();

      if (auto runEnv = runEnvironmentAspect(); runEnv)
      {
        Utils::EnvironmentItems newPaths;
        foreach (const auto& path, appendPath)
        {
          auto envItem = Utils::EnvironmentItem(
              QStringLiteral("PATH"), path, Utils::NameValueItem::Append);
          newPaths.push_back(envItem);
          write(tr("Add path to env >%1<").arg(path));
        }
        runEnv->setUserEnvironmentChanges(newPaths);
      }
      else
      {
        write(tr("Error, no run environment available"));
      }
    }

    void conanPlugin::loadProjectConfiguration(
        const ProjectExplorer::Project* project)
    {
      if (project == nullptr)
        return;

      const QString settingsPath = QDir(project->projectDirectory().toString())
                                       .filePath(QStringLiteral("qconan.ini"));

      if (auto newConfig = PluginConfig::fromFile(settingsPath); newConfig)
      {
        watchConanfile(settingsPath);

        _config = newConfig.value();
        write(tr("Found settings file"));
      }
    }

    void conanPlugin::write(const QString& text) const
    {
      auto messenger = Core::MessageManager::instance();
      messenger->write(tr("conan plugin: %1").arg(text));
    }

    ProjectExplorer::Target* conanPlugin::currentTarget()
    {
      auto prjTree = ProjectExplorer::ProjectTree::instance();
#if QTCREATOR_MAJOR == 4 && QTCREATOR_MINOR >= 12
      return prjTree->currentTarget();
#else
      if (auto prj = prjTree->currentProject(); prj)
        return prj->activeTarget();
      return nullptr;
#endif
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

    void conanPlugin::watchConanfile(const QString& path)
    {
      QFileInfo conanFile(path);
      if (auto conanPath = conanFile.canonicalFilePath();
          !_conanFileWatcher.files().contains(conanPath))
      {
        _conanFileWatcher.addPath(conanPath);
      }
    }

  } // namespace Internal
} // namespace conan
