#pragma once

#include "conan_global.h"

#include <QDir>
#include <extensionsystem/iplugin.h>
#include <projectexplorer/projecttree.h>

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
      QVariantMap conanInstall(
          const QString& pathToConanFile, const QDir& directory) const;

      bool initialize(const QStringList& arguments, QString* errorString);
      void extensionsInitialized();
      ShutdownFlag aboutToShutdown();

      void updateDepConnections();

      ///
      /// Evaluate the dependency tree and set up the build directory
      ///
      /// Try to install the referenced requirements and create the dependency
      /// information.
      ///
      void evaluateDependencies();

    private:
      QString currentBuildDir() const;

    private:
      QChar _pathSeparator;
      std::list< QMetaObject::Connection > _depConnections;
    };

  } // namespace Internal
} // namespace conan
