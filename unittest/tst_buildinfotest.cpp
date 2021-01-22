#include "../BuildInfo.h"
#include <QtTest>
Q_DECLARE_METATYPE(BuildInfo)

class BuildInfoTest : public QObject
{
  Q_OBJECT

public:
  BuildInfoTest() {}
  ~BuildInfoTest() {}

private slots:
  void testInvalid_data()
  {
    QTest::addColumn< BuildInfo >("info");

    QVariantMap map;
    map.insert("Test", "Test");
    BuildInfo wrongMap(map);

    QTest::newRow("emptyTest") << BuildInfo();
    QTest::newRow("emptyMap") << BuildInfo(QVariantMap());
    QTest::newRow("wrongData") << wrongMap;
    QTest::newRow("notExistingFile") << BuildInfo::fromJsonFile("NOTEXISTING");
    QTest::newRow("dependencieIsNotAList")
        << BuildInfo(QVariantMap({{QString("dependencies"), QVariant()}}));
    {
      QVariantMap dep1;
      dep1.insert("bin_paths", QVariant::fromValue(QList< QVariant >()));
      QList< QVariant > deps = {dep1};
      QVariantMap root({{QString("dependencies"), deps}});
      QTest::newRow("noLibPath") << BuildInfo(root);
    }
    {
      QVariantMap dep1;
      dep1.insert("lib_paths", QVariant::fromValue(QList< QVariant >()));
      QList< QVariant > deps = {dep1};
      QVariantMap root({{QString("dependencies"), deps}});
      QTest::newRow("noBinPath") << BuildInfo(root);
    }
  }

  void testInvalid()
  {
    QFETCH(BuildInfo, info);

    QCOMPARE(info.isValid(), false);
    QCOMPARE(info.lastError().isEmpty(), false);
  }

  void testfromJsonFile()
  {
    auto parseJson = BuildInfo::fromJsonFile("../unittest/validEmpty.json");
    QCOMPARE(parseJson.lastError().isNull(), true);
  }

  void testDataStructure_data()
  {
    QTest::addColumn< BuildInfo >("info");
    QTest::addColumn< QStringList >("libPaths");
    QTest::addColumn< QStringList >("binPaths");

    {
      QVariantMap dep1;
      dep1.insert(
          "lib_paths", QVariant::fromValue(QList< QVariant >({"dep1_test"})));
      dep1.insert("bin_paths", QVariant::fromValue(QList< QVariant >()));

      QList< QVariant > deps = {dep1};

      QVariantMap root;
      root.insert("dependencies", deps);
      root.insert("deps_env_info", QVariantMap());

      QTest::newRow("singleLib")
          << BuildInfo(root) << QStringList("dep1_test") << QStringList();
      deps.push_back(dep1);
      root["dependencies"] = deps;
      QTest::newRow("twoDepsSameLib")
          << BuildInfo(root) << QStringList({"dep1_test", "dep1_test"})
          << QStringList();
    }
    {
      QVariantMap dep1;
      dep1.insert("lib_paths",
          QVariant::fromValue(QList< QVariant >({"dep1_test", "dep2_test"})));
      dep1.insert("bin_paths", QVariant::fromValue(QList< QVariant >()));

      QList< QVariant > deps = {dep1};

      QVariantMap root;
      root.insert("dependencies", deps);
      root.insert("deps_env_info", QVariantMap());

      QTest::newRow("doubleLib")
          << BuildInfo(root) << QStringList({"dep1_test", "dep2_test"})
          << QStringList();
    }
    {
      QVariantMap dep1;
      dep1.insert("lib_paths", QVariant::fromValue(QList< QVariant >()));
      dep1.insert("bin_paths",
          QVariant::fromValue(QList< QVariant >({"dep1_test", "dep2_test"})));

      QList< QVariant > deps = {dep1};

      QVariantMap root;
      root.insert("dependencies", deps);
      root.insert("deps_env_info", QVariantMap());

      QTest::newRow("doubleBin") << BuildInfo(root) << QStringList()
                                 << QStringList({"dep1_test", "dep2_test"});
      deps.push_back(dep1);
      root["dependencies"] = deps;
      QTest::newRow("twoDepsDoubleBin")
          << BuildInfo(root) << QStringList()
          << QStringList({"dep1_test", "dep1_test", "dep2_test", "dep2_test"});
    }
    {
      QVariantMap dep1;
      dep1.insert(
          "lib_paths", QVariant::fromValue(QList< QVariant >(
                           {QDir::toNativeSeparators("src/../dep1_test")})));
      dep1.insert("bin_paths", QVariant::fromValue(QList< QVariant >()));

      QList< QVariant > deps = {dep1};

      QVariantMap root;
      root.insert("dependencies", deps);
      root.insert("deps_env_info", QVariantMap());

      QTest::newRow("relativeLib")
          << BuildInfo(root) << QStringList("dep1_test") << QStringList();
    }
    {
      QVariantMap dep1;
      dep1.insert(
          "bin_paths", QVariant::fromValue(QList< QVariant >(
                           {QDir::toNativeSeparators("src/../dep1_test")})));
      dep1.insert("lib_paths", QVariant::fromValue(QList< QVariant >()));

      QList< QVariant > deps = {dep1};

      QVariantMap root;
      root.insert("dependencies", deps);
      root.insert("deps_env_info", QVariantMap());

      QTest::newRow("relativeBin")
          << BuildInfo(root) << QStringList() << QStringList("dep1_test");
    }
    {
      QVariantMap dep1;
      dep1.insert("bin_paths", QVariant::fromValue(QList< QVariant >(
                                   {R"_(src\dep1_test/include)_"})));
      dep1.insert("lib_paths", QVariant::fromValue(QList< QVariant >()));

      QList< QVariant > deps = {dep1};

      QVariantMap root;
      root.insert("dependencies", deps);
      root.insert("deps_env_info", QVariantMap());

      QTest::newRow("mixedPaths")
          << BuildInfo(root) << QStringList()
          << QStringList(QDir::toNativeSeparators("src/dep1_test/include"));
    }
  }

  void testDataStructure()
  {
    QFETCH(BuildInfo, info);
    QFETCH(QStringList, libPaths);
    QFETCH(QStringList, binPaths);

    QCOMPARE(info.isValid(), true);
    QCOMPARE(info.lastError().isNull(), true);

    auto actualLib = info.libraryPath();
    auto actualBin = info.binaryPath();
    actualLib.sort();
    actualBin.sort();
    libPaths.sort();
    binPaths.sort();

    QCOMPARE(actualLib, libPaths);
    QCOMPARE(actualBin, binPaths);
  }

  void testEnvironmentPath_data()
  {
    QTest::addColumn< BuildInfo >("info");
    QTest::addColumn< QStringList >("paths");

    auto createTree = [](const QVariantList& path) -> QVariantMap {
      QVariantMap envInfo;
      envInfo.insert("PATH", QVariant::fromValue(path));

      QVariantMap dep;
      dep.insert("lib_paths", QVariant::fromValue(QList< QVariant >()));
      dep.insert("bin_paths", QVariant::fromValue(QList< QVariant >()));

      QVariantMap root;
      root.insert("deps_env_info", envInfo);
      root.insert("dependencies", QList< QVariant >({dep}));
      return root;
    };

    QTest::newRow("emptyList") << BuildInfo(createTree({})) << QStringList();
    QTest::newRow("singleEntry")
        << BuildInfo(createTree({"pathInfo1"})) << QStringList("pathInfo1");
    QTest::newRow("doubleEntry")
        << BuildInfo(createTree({"pathInfo1", "pathInfo2"}))
        << QStringList({"pathInfo1", "pathInfo2"});
    QTest::newRow("notUniqueEntries")
        << BuildInfo(createTree({"pathInfo1", "pathInfo2", "pathInfo1"}))
        << QStringList({"pathInfo1", "pathInfo2"});
  }

  void testEnvironmentPath()
  {
    QFETCH(BuildInfo, info);
    QFETCH(QStringList, paths);

    QCOMPARE(info.isValid(), true);
    QCOMPARE(info.lastError().isNull(), true);

    auto actualPath = info.environmentPath();

    actualPath.sort();
    paths.sort();

    QCOMPARE(actualPath, paths);
  }
};

QTEST_APPLESS_MAIN(BuildInfoTest)

#include "tst_buildinfotest.moc"
