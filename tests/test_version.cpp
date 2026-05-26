#include <QtTest>
#include <version.h>

class TestVersion : public QObject
{
    Q_OBJECT

private slots:
    void versionMacrosAreDefined()
    {
        QVERIFY(BETTER_FRAMELESS_WINDOW_VERSION_MAJOR >= 0);
        QVERIFY(BETTER_FRAMELESS_WINDOW_VERSION_MINOR >= 0);
        QVERIFY(BETTER_FRAMELESS_WINDOW_VERSION_PATCH >= 0);
    }

    void versionStringIsPlausible()
    {
        const int major = BETTER_FRAMELESS_WINDOW_VERSION_MAJOR;
        const int minor = BETTER_FRAMELESS_WINDOW_VERSION_MINOR;
        const int patch = BETTER_FRAMELESS_WINDOW_VERSION_PATCH;
        QVERIFY(major >= 0 && major < 100);
        QVERIFY(minor >= 0 && minor < 100);
        QVERIFY(patch >= 0 && patch < 1000);
    }
};

QTEST_MAIN(TestVersion)
#include "test_version.moc"
