#pragma once

#include "PluginConfig.h"
#include "conan_global.h"
#include <QDir>
#include <QFileSystemWatcher>
#include <QProcess>
#include <extensionsystem/iplugin.h>
#include <projectexplorer/projecttree.h>

class BuildInfo;
namespace ProjectExplorer {
  class EnvironmentAspect;
  class Target;
} // namespace ProjectExplorer

namespace conan {
  namespace Internal {

    class conanPlugin : public ExtensionSystem::IPlugin
    {
      Q_OBJECT
      Q_PLUGIN_METADATA(
          IID "org.qt-project.Qt.QtCreatorPlugin" FILE "conan.json")

    public:
      conanPlugin();
      ~conanPlugin();

      ///
      /// Execute conan install on the given conanFile path
      ///
      /// The provided directory must already exist, otherwise the behavior is
      /// undefined.
      /// \return A map containing the buildinfo generated by conan
      ///
      BuildInfo conanInstall(
          const QString& pathToConanFile, const QDir& directory);

      bool initialize(const QStringList& arguments, QString* errorString);
      void extensionsInitialized();
      ShutdownFlag aboutToShutdown();

      void setNewProject(ProjectExplorer::Project* project);

      void removeDepConnections();
      void updateDepConnections();

      ///
      /// Evaluate the dependency tree and set up the build directory
      ///
      /// Try to install the referenced requirements and create the dependency
      /// information.
      ///
      void setupBuildDirForce();
      void setupBuildDir(bool forceInstall);

      QString conanFilePath() const;

      ProjectExplorer::EnvironmentAspect* runEnvironmentAspect() const;

    private:
      void onInstallFinished(int exitCode, QProcess::ExitStatus exitStatus);
      void loadProjectConfiguration(const ProjectExplorer::Project* project);

      void write(const QString& text) const;

      static ProjectExplorer::Target* currentTarget();

      QString currentBuildDir() const;

      void watchConanfile(const QString& path);

      PluginConfig _config;

    private:
      QProcess _conanBin;
      QString _lastInstallDir;
      QFileSystemWatcher _conanFileWatcher;
      QFileSystemWatcher _settingsFileWatcher;
      QChar _pathSeparator;
      std::list< QMetaObject::Connection > _depConnections;
    };

  } // namespace Internal
} // namespace conan
