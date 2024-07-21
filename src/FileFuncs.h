#ifndef FileFuncs_h_
#define FileFuncs_h_

#include <QObject>
#include <QFile>
#include <QDir>
#include <QGuiApplication>
#include <QDebug>
#include <QDateTime>
#include <QDir>
#include <QProcess>
#include <QTime>

#define LOG_EXPORT_DIR "C:/ProgramData/Sunia_Agent/"


class FileFuncs : public QObject
{
    Q_OBJECT

public:
    FileFuncs();
    ~FileFuncs();

    bool rmDir(const QString & path);
    bool clearDir(const QString & path);
    void mkdir(const QString & path);
    QStringList getAllFolder(const QString & path);

public slots:
    bool rmFile(const QString & path);
    bool isExistFile(const QString & path);

    void saveStringToFile(QString const & content, QString const & path);
    QString loadFileToString(const QString & path) const;

    // void encryptStringToFile(QString const & content, QString const & path);
    // QString loadEncryptedFileToString(const QString & path);

    void openLocalPath(const QString path);    // 打开本地文件夹

    //  获取临时文件的本地目录
    QString getTempRoot(QString cpath = "") const;

    QString getSystemTempRoot() const;  // 更新程序临时保存目录
    QString getNewExePath() const;  // 最新程序安装包保存路径

    void exportSystemLog(const qint64 days = 1);

    static bool copyFile(QString Destination);//拷贝更新软件到公共区域
    static bool copyFileToPath(QString sourceDir ,QString toDir, bool coverFileIfExist);
    static bool copyDirectoryFiles(const QString &fromDir, const QString &toDir, bool coverFileIfExist);

    bool copy(const QString &oldpath, const QString &newpath, bool createDirs = true);
    static bool existDir(QString const & path);
    static bool createDirectorys(QString const & path);

};
#endif
