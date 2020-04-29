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

      QTest::newRow("doubleBin") << BuildInfo(root) << QStringList()
                                 << QStringList({"dep1_test", "dep2_test"});
      deps.push_back(dep1);
      root["dependencies"] = deps;
      QTest::newRow("twoDepsDoubleBin")
          << BuildInfo(root) << QStringList()
          << QStringList({"dep1_test", "dep1_test", "dep2_test", "dep2_test"});
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
};

QTEST_APPLESS_MAIN(BuildInfoTest)

#include "tst_buildinfotest.moc"
