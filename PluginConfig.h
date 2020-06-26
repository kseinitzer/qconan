#pragma once

#include <QString>

class PluginConfig
{
private:
  QString _conanFilePath;
  QString _installFlags;
  bool _useLibPath;
  bool _useBinPath;

public:
  PluginConfig() = default;
  PluginConfig(const QString& path, const QString& installFlags,
      const bool useBinPath, const bool useLibPath)
      : _conanFilePath {path}, _installFlags {installFlags},
        _useLibPath {useLibPath}, _useBinPath {useBinPath}
  {
  }
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
