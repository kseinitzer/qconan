#include "BuildInfo.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>

namespace {
  const QString dependencyKey = QStringLiteral("dependencies");
  const QString libraryKey = QStringLiteral("lib_paths");
  const QString binaryKey = QStringLiteral("bin_paths");
} // namespace

BuildInfo::BuildInfo()
    : _failureMessage {QStringLiteral("BuildInfo was empty initialized!")}
{
}

BuildInfo::BuildInfo(const QVariantMap& dependencies) : _root(dependencies) {}

BuildInfo BuildInfo::fromJsonFile(const QString& fileName)
{
  QFile buildInfo(fileName);
  if (!buildInfo.open(QIODevice::ReadOnly))
  {
    return BuildInfo(QVariantMap(),
        QObject::tr("BuildInfo> Error, could not open file: %1").arg(fileName));
  }

  QJsonParseError err;
  auto doc = QJsonDocument::fromJson(buildInfo.readAll(), &err);

  if (err.error != QJsonParseError::NoError)
  {
    return BuildInfo(
        QVariantMap(), QObject::tr("Error, JSON file could not be parsed: %1")
                           .arg(err.errorString()));
  }

  return BuildInfo(doc.object().toVariantMap());
}

BuildInfo::BuildInfo(
    const QVariantMap& dependencies, const QString failureMessage)
    : BuildInfo(dependencies)
{
  _failureMessage = failureMessage;
}

QStringList BuildInfo::libraryPath() const
{
  return dependenciesToStringList(libraryKey);
}

QStringList BuildInfo::binaryPath() const
{
  return dependenciesToStringList(binaryKey);
}

QStringList BuildInfo::dependenciesToStringList(const QString key) const
{
  QStringList result;

  if (const auto& depListObj = _root.value(dependencyKey);
      depListObj.type() == QVariant::Type::List)
  {
    const QVariantList depList = _root.value(dependencyKey).toList();

    foreach (const auto& depObj, depList)
    {
      auto variantList = depObj.toMap().value(key).toList();
      std::for_each(variantList.begin(), variantList.end(),
          [&result](const auto& el) { result.push_back(el.toString()); });
    }
  }
  return result;
}

bool BuildInfo::isValid() const
{

  if (_root.isEmpty() ||
      _root.value(dependencyKey).type() != QVariant::Type::List)
  {
    _failureMessage =
        QObject::tr("BuildInfo: No data available or dependency key missing");
    return false;
  }

  foreach (const auto& depList, _root.value(dependencyKey).toList())
  {
    if (depList.type() != QVariant::Type::Map)
    {
      _failureMessage =
          QObject::tr("BuildInfo: dependency entry is not containing a Map");
      return false;
    }
    auto depMap = depList.toMap();
    if (depMap.value(libraryKey).type() != QVariant::Type::List)
    {
      _failureMessage = QObject::tr(
          "BuildInfo: dependency entry is not containing the library key");
      return false;
    }
    if (depMap.value(binaryKey).type() != QVariant::Type::List)
    {
      _failureMessage = QObject::tr(
          "BuildInfo: dependency entry is not containing the binary key");
      return false;
    }
  }

  return true;
}

QString BuildInfo::lastError() const
{
  return _failureMessage;
}
