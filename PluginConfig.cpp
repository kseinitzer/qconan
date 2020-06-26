#include "PluginConfig.h"
#include <QFileInfo>
#include <QSettings>

std::optional< PluginConfig > PluginConfig::fromFile(QString pathToFile)
{
  if (!QFileInfo(pathToFile).exists())
    return {};

  QSettings settings(pathToFile, QSettings::Format::IniFormat);

  PluginConfig conf;
  conf._conanFilePath =
      settings.value(QStringLiteral("global/path")).toString();
  conf._useBinPath =
      settings.value(QStringLiteral("environment/useBinPath"), false).toBool();
  conf._useLibPath =
      settings.value(QStringLiteral("environment/useLibPath"), false).toBool();
  conf._installFlags =
      settings.value(QStringLiteral("global/installFlags")).toString();
  return std::optional< PluginConfig >(conf);
}
