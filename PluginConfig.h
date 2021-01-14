#pragma once

#include <QString>
#include <optional>

class PluginConfig
{
private:
  QString _conanFilePath;
  QString _installFlags;
  bool _useLibPath;
  bool _useBinPath;

public:
  PluginConfig() = default;

  ///
  /// \brief fromFile Provide a way how to load the configuration from a file
  /// \details The referenced file must be encode as ASCII ini file.
  /// \param pathToFile Absolut or relative path to the configuration file
  /// \return A valid PluginConfig instance as value or in case of an failure no
  /// valid reference.
  ///
  static std::optional< PluginConfig > fromFile(QString pathToFile);

  bool useLibraryPathAsEnvironmentPath() const
  {
    return _useLibPath;
  }
  bool useBinaryPathAsEnvironmentPath() const
  {
    return _useBinPath;
  }
  bool isAutoDetect() const
  {
    return _conanFilePath.isEmpty();
  }

  QString conanFile() const
  {
    return _conanFilePath;
  }

  QString installFlags() const
  {
    return _installFlags;
  }
};
