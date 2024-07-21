#include "FileFuncs.h"


static void iCreateDirectorys(QString const & path)
{
    QString rp = path;
    rp = rp.replace("\\", "/");
    int lp = rp.lastIndexOf("/");
    if (lp > 0) {
        rp.resize(lp);
    }

    QDir dir;
    dir.mkpath(rp);
}

static QString defaultEncryptKey()
{
    return "CDGD20170707BYZY";
}

FileFuncs::FileFuncs()
{

}

FileFuncs::~FileFuncs()
{

}

void FileFuncs::saveStringToFile(const QString &content, const QString &path)
{
    QString p = path;
    if (p.startsWith("file:///")) p = p.mid(8);

    iCreateDirectorys(p);

    QFile fi(p);
    if (!fi.open(QIODevice::WriteOnly)) {
        return;
    }

    QByteArray ba = content.toUtf8();
    fi.write(ba);
    fi.close();
}

QString FileFuncs::loadFileToString(const QString & inpath) const
{
    QString path = inpath;
    if (path.startsWith("file:///")) path = path.mid(8);

    QFile fi(path);
    if (!fi.open(QIODevice::ReadOnly)) {
        return "";
    }

    QByteArray ba = fi.readAll();

    QString str = QString::fromUtf8(ba);

    fi.close();
    return str;
}

// void FileFuncs::encryptStringToFile(const QString &content, const QString &path)
// {
//     QString p = path;
//     if (p.startsWith("file:///")) p = p.mid(8);

//     QFile fi(p);
//     if (!fi.open(QIODevice::WriteOnly)) {
//         return;
//     }

//     QString aesKey = defaultEncryptKey();
//     QByteArray key = aesKey.toUtf8();
//     QByteArray ba = content.toUtf8();
//     QByteArray ca = Encrypt(ba, key);

//     fi.write(ca);
//     fi.close();
// }

// QString FileFuncs::loadEncryptedFileToString(const QString & inpath)
// {
//     QString path = inpath;
//     if (path.startsWith("file:///")) path = path.mid(8);

//     QFile fi(path);
//     if (!fi.open(QIODevice::ReadOnly)) {
//         return "";
//     }

//     QString aesKey = defaultEncryptKey();
//     QByteArray key = aesKey.toUtf8();
//     QByteArray ba = fi.readAll();
//     QByteArray ea = Uncrypt(ba, key);

//     QString str = QString::fromUtf8(ea);

//     fi.close();
//     return str;
// }

void FileFuncs::openLocalPath(const QString path)
{
    if(path.isEmpty())  return;
    QString dstpath = path;
    dstpath = dstpath.replace("/", "\\");
    QProcess::startDetached("explorer " + dstpath);
}
bool FileFuncs::rmDir(const QString &path)
{
    if (path.isEmpty()){
        return false;
    }

    QDir dir(path);
    if(!dir.exists()){
        return true;
    }
    dir.setFilter(QDir::AllEntries | QDir::NoDotAndDotDot);
    QFileInfoList fileList = dir.entryInfoList();
    foreach (QFileInfo file, fileList)
    {
        if (file.isFile()){
            file.dir().remove(file.fileName());
        }else{
            rmDir(file.absoluteFilePath());
        }
    }
    return dir.rmpath(dir.absolutePath());
}
bool FileFuncs::clearDir(const QString &path)
{
    if (path.isEmpty()){
        return false;
    }

    QDir dir(path);
    if(!dir.exists()){
        return true;
    }
    dir.setFilter(QDir::AllEntries | QDir::NoDotAndDotDot);
    QFileInfoList fileList = dir.entryInfoList();
    foreach (QFileInfo file, fileList)
    {
        if (file.isFile()){
            file.dir().remove(file.fileName());
        }else{
            rmDir(file.absoluteFilePath());
        }
    }
    return true;
}

bool FileFuncs::rmFile(const QString &path)
{
    if (path.isEmpty()) {
        return false;
    }

    return QFile::remove(path);
}

bool FileFuncs::isExistFile(const QString &path)
{
    QString rpath = path;
    if (rpath.startsWith("file:///")) rpath = rpath.mid(8);

    if (rpath.isEmpty()){
        return false;
    }

    QFileInfo file(rpath);

    if (file.isFile()){
        return true;
    }else{
        return false;
    };
}

void FileFuncs::mkdir(const QString &path)
{
    QString folder = path.mid(0, path.lastIndexOf("\\"));
    QDir dir(folder);
    if(!dir.exists()) {
        dir.mkpath(folder);
    }
}

QStringList FileFuncs::getAllFolder(const QString &path)
{
    QDir dir(path);
    QStringList files = dir.entryList(QDir::AllDirs | QDir::NoDotAndDotDot, QDir::Time);
    return files;
}

QString FileFuncs::getTempRoot(QString cpath) const
{
    QString path = QGuiApplication::applicationDirPath();
    QDir dir(path);
    QString dirName = dir.dirName();
    QString rpath;
    if ((dirName == "debug") || (dirName == "release")) {
        dir.cdUp();
        dir.cdUp();
    }
    rpath = dir.absolutePath() + "/temp/" + cpath;
    // 如果不存在,创建
    QDir ndir(rpath);
    if(!ndir.exists()) {
        ndir.mkpath(rpath);
    }
    return rpath;

}

QString FileFuncs::getSystemTempRoot() const
{
    QString dir = QDir::tempPath();
//    dir += "/SuniaCamUpgrade";
    return dir;
}

QString FileFuncs::getNewExePath() const
{
    QString dir = QDir::tempPath();
    return dir + "/SuniaExe/CAM_Client.exe";
}

void FileFuncs::exportSystemLog(const qint64 days)
{
    QString logDir = getSystemTempRoot() + "/Sunia_Agent";
    QDir dir(logDir);
    //该目录下的具体文件的时间
    QFileInfoList fileList = dir.entryInfoList(QDir::NoDotAndDotDot | QDir::Files, QDir::Time); //只获取该目录下的文件，按最迟修改时间排序
//    QDateTime lastModifiedTime(QDate(0, 0, 0));
    QDateTime newTime = QDateTime::currentDateTime();
    QString exportDir = LOG_EXPORT_DIR + newTime.toString("yyyy-MM-dd_hh_mm_ss");
    mkdir(exportDir);
    qDebug() << "exportDir ==" << exportDir;
    if (fileList.isEmpty() == false)
    {
        for(QFileInfo f : fileList){
            QDateTime f_time = f.lastModified();
            QString fileSuffix = f.completeSuffix();
            if(fileSuffix != "txt")  continue;
            QString filePath = f.absoluteFilePath();
            QString fileName = f.fileName();
            qint64 dayTo = f_time.daysTo(newTime);
            if(dayTo > days)    break;
            // 导出满足条件的日志文件
            QString newPath = exportDir + "/" + fileName;
            copy(filePath, newPath, true);
        }
    }
    openLocalPath(exportDir);
}


bool FileFuncs::copyFile(QString Destination)
{
    QVector<QString> Dllall ;
    Dllall  <<"Qt5Widgets.dll"
            <<"D3Dcompiler_47.dll"
            <<"libEGL.dll"
            <<"libGLESV2.dll"
            <<"opengl32sw.dll"
            <<"Qt5Core.dll"
            <<"Qt5Gui.dll"
            <<"Qt5Svg.dll"
            <<"Qt5Network.dll"
            <<"bearer"
            <<"iconengines"
            <<"imageformats"
            <<"platforms"
            <<"styles"
            <<"translations"
            <<"update.exe";
    QDir DestinationDir;
    DestinationDir.mkpath(Destination);

    QVector<QString> TmpDllall;
    //获取当前 程序路径
    //QString ExePathDll = QDir::currentPath();
    QString ExePathDll = QCoreApplication::applicationDirPath();
    //QDir(const QDir &dir)
    for(int i = 0 ; i <Dllall.size() ; i++ ){
        TmpDllall <<ExePathDll+"/"+Dllall[i];
    }
    QVector<QString> CopyPathDll ;
    for(int i = 0 ; i <Dllall.size() ; i++ ){
        CopyPathDll <<Destination+"/"+Dllall[i];
    }

    for(int i = 0 ; i <Dllall.size() ; i++ ){
        if(Dllall[i].contains(".",Qt::CaseSensitive)){
            //包含
            if(!copyFileToPath(TmpDllall[i] ,CopyPathDll[i], true))
            {
                //拷贝失败
                qDebug()<<"Copies of the failure"<<Dllall[i];

            }

        }else{
            //不包含
            if(!copyDirectoryFiles(TmpDllall[i], CopyPathDll[i], true)){
                //拷贝失败
                qDebug()<<"Copies of the failure"<<Dllall[i];
            }

        }

    }
    return true;
}

//拷贝文件：
bool FileFuncs::copyFileToPath(QString sourceDir ,QString toDir, bool coverFileIfExist)
{
    toDir.replace("\\","/");
    if (sourceDir == toDir){
        return true;
    }
    if (!QFile::exists(sourceDir)){
        return false;
    }
    QDir *createfile     = new QDir;
    bool exist = createfile->exists(toDir);
    if (exist){
        if(coverFileIfExist){
            createfile->remove(toDir);
        }
    }//end if

    if(!QFile::copy(sourceDir, toDir))
    {
        return false;
    }

    return true;
}

//拷贝文件夹：
bool FileFuncs::copyDirectoryFiles(const QString &fromDir, const QString &toDir, bool coverFileIfExist)
{
    QDir sourceDir(fromDir);
    QDir targetDir(toDir);
    if(!targetDir.exists()){    /**< 如果目标目录不存在，则进行创建 */
        if(!targetDir.mkdir(targetDir.absolutePath()))
            return false;
    }
    QFileInfoList fileInfoList = sourceDir.entryInfoList();
    foreach(QFileInfo fileInfo, fileInfoList){
        if(fileInfo.fileName() == "." || fileInfo.fileName() == "..")
            continue;

        if(fileInfo.isDir()){    /**< 当为目录时，递归的进行copy */
            if(!copyDirectoryFiles(fileInfo.filePath(),
                                   targetDir.filePath(fileInfo.fileName()),
                                   coverFileIfExist))
                return false;
        }
        else{            /**< 当允许覆盖操作时，将旧文件进行删除操作 */
            if(coverFileIfExist && targetDir.exists(fileInfo.fileName())){
                targetDir.remove(fileInfo.fileName());
            }

            /// 进行文件copy
            if(!QFile::copy(fileInfo.filePath(),
                            targetDir.filePath(fileInfo.fileName()))){
                return false;
            }
        }
    }
    return true;
}

bool FileFuncs::copy(const QString &oldpath, const QString &newpath, bool createDirs)
{
    QFileInfo fi(oldpath);
    if (!fi.exists()) {
        return false;
    }

    if (createDirs) createDirectorys(newpath);

    QFile::remove(newpath);

    bool res = QFile::copy(oldpath, newpath);
    if (1 && res == false) {
    }

    QFileInfo finew(newpath);
    if (!finew.exists(newpath)) {
    }

    return res;
}

bool FileFuncs::existDir(const QString &path)
{
    QFileInfo fi(path);
    return fi.exists() && fi.isFile();
}

bool FileFuncs::createDirectorys(const QString &path)
{
    QString rp = path;
    rp = rp.replace("\\", "/");
    int lp = rp.lastIndexOf("/");
    if (lp > 0) {
        rp.resize(lp);
    }

    QDir dir(rp);
    if (dir.exists()) {
        return true;
    }

    dir.mkpath(rp);
    bool res = existDir(rp);

    if (res == false) {
        qDebug() << "Create directory fail:" << rp;
    }

    return res;
}


