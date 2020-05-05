#pragma once

#include <QVariantMap>

///
/// \brief Helper class to allow covenient access to the conan build info
///
/// During the install command a json file can be generated that contain all
/// dependency related information. This class allow a conveniet way to access
/// the stored information. Also the static method fromJsonFile can be used
/// to directly load the information from the file. The class itself do not
/// support or store any file related operation.
///
/// Keys that are used by this helper class:
///  - dependencies/lib_paths,
///  - dependencies/bin_paths.
///
class BuildInfo
{
public:
  BuildInfo();

  explicit BuildInfo(const QVariantMap& dependencies);

  ///
  /// \brief fromJsonFile Create a new BuildInfo instance from a file
  /// \param fileName The Json encoded file that should be loaded
  /// \return A BuildInfo object. If any error occured during the file load,
  /// the BuildInfo instance will be invalid and the lastError method will
  /// give more information.
  ///
  static BuildInfo fromJsonFile(const QString& fileName);

  QStringList libraryPath() const;
  QStringList binaryPath() const;

  ///
  /// \brief environmentPath Read the PATH information from the build info
  ///
  /// Every conan package can add information to the PATH environment. The final
  /// consuming package collect all informations from the required packages and
  /// provide a list of all PATH values. This method provides access to the
  /// available information from deps_env_info.
  /// \return Return a list containing the PATH information from all
  /// dependencies. It is guaranteed that every element is unique in the list.
  ///
  QStringList environmentPath() const;

  ///
  /// \brief dependenciesToStringList Combine all found dependencies values
  /// \param key The key for which the values should be combined.
  /// \return All dependency values with the same key will be returned in a
  /// list. The order of this list is a implementation detail.
  ///
  QStringList dependenciesToStringList(const QString key) const;

  ///
  /// \brief isValid Reflect the internal BuildInfo state
  /// Check if all needed keys are available. The needed keys are documented
  /// at the class description.
  /// \return True if all needed keys are available, otherwise false.
  ///
  bool isValid() const;

  ///
  /// \brief lastError Return the latest occured error message
  /// \return latest occured error message or a null string
  ///
  QString lastError() const;

private:
  explicit BuildInfo(
      const QVariantMap& dependencies, const QString failureMessage);

private:
  const QVariantMap _root;
  mutable QString _failureMessage;
};
