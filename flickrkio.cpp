#include "flickrkio.h"
#include <kdebug.h>
#include <kcomponentdata.h>
#include <qfileinfo.h>
#include <kdebug.h>
#include <KApplication>
#include <kcmdlineargs.h>
#include <qjson/parser.h>


#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>


using namespace KIO;

extern "C" int KDE_EXPORT kdemain( int argc, char **argv )
{
    kDebug(7233) << "Entering function";
    KComponentData instance( "kio_flirckkio" );

    //need an app to have an eventloop, which is needed for our http downloads
    putenv(strdup("SESSION_MANAGER="));
    KCmdLineArgs::init(argc, argv, "kio_flickr", 0, KLocalizedString(), 0, KLocalizedString());
    KCmdLineOptions options;
    options.add("+protocol", ki18n( "Protocol name" ));
    options.add("+pool", ki18n( "Socket name" ));
    options.add("+app", ki18n( "Socket name" ));
    KCmdLineArgs::addCmdLineOptions( options );
    KApplication app;

    if (argc != 4)
    {
        fprintf( stderr, "Usage: kio_flickr protocol domain-socket1 domain-socket2\n");
        exit( -1 );
    }
    flickrkio slave( argv[2], argv[3] );
    slave.dispatchLoop();
    return 0;
}

void flickrkio::get( const KUrl &url )
{
    //if it's not a photo, the user has gone wrong
    //format is (currently) something/PHOTO_ID

//     if (!url.path().endsWith(".jpg"))
//     {
//       finished();
//       return;
//     }
    QString photo_id = url.path().section('/',-1);

    KUrl getPhotoUrlQuery("http://api.flickr.com/services/rest/?method=flickr.photos.getSizes");
    getPhotoUrlQuery.addQueryItem("api_key",FLICKR_API_KEY);
    getPhotoUrlQuery.addQueryItem("format","json");
    getPhotoUrlQuery.addQueryItem("nojsoncallback","1");

    getPhotoUrlQuery.addQueryItem("photo_id",photo_id);


    QNetworkAccessManager accessManager(this);
    QNetworkReply* getPhotoUrlReply = accessManager.get(QNetworkRequest(getPhotoUrlQuery));
    blockUntilFinished(getPhotoUrlReply);

    QJson::Parser parser;
    bool ok;

    QVariantMap result = parser.parse(getPhotoUrlReply->readAll(),&ok).toMap();

    //FIXME lazily assumes "original size" is last in the list.. could do with fixing
    //not as crash prone as it appears - toMap()/toList create empty lists if QVariant cannot be converted
    //if any of these fail, we just get an empty string.
    QString photoUrl = result["sizes"].toMap()["size"].toList().last().toMap()["source"].toString();

    //forward to the real image now.
    ForwardingSlaveBase::get(KUrl(photoUrl));
}



void flickrkio::stat( const KUrl &url )
{
    Q_UNUSED(url);
    //TODO ...this entire function..
    finished();
}

//needed to fix pure virtual function in parent
bool flickrkio::rewriteUrl(const KUrl&, KUrl&)
{
    return true;
}


void flickrkio::listDir( const KUrl &url )
{
    kDebug() << "Start List Dir";
    QString userId = "12809175@N00";

    if (url.user().isEmpty())
    {
        messageBox("Please visit flickr:/yourusername@/. Note this will be fixed shortly.")
        finished();
        return;
    }
    else
    {
        //lookup username
    }

    if (url.path().isEmpty() || url.path() == "/")
    {
        //list photosets

        QJson::Parser parser;
        bool ok;

        KUrl getPhotoSetsListQuery("http://api.flickr.com/services/rest/?method=flickr.photosets.getList");
        getPhotoSetsListQuery.addQueryItem("api_key",FLICKR_API_KEY);
        getPhotoSetsListQuery.addQueryItem("format","json");
        getPhotoSetsListQuery.addQueryItem("nojsoncallback","1");

        getPhotoSetsListQuery.addQueryItem("user_id",userId);

        QNetworkAccessManager accessManager(this);
        QNetworkReply* getPhotoSetsListReply = accessManager.get(QNetworkRequest(getPhotoSetsListQuery));
        blockUntilFinished(getPhotoSetsListReply);

        QVariantMap result = parser.parse(getPhotoSetsListReply->readAll(),&ok).toMap();

        UDSEntry e;

        foreach (QVariant photoset, result["photosets"].toMap()["photoset"].toList())
        {
            QString title = photoset.toMap()["title"].toMap()["_content"].toString();
            QString photoset_id = photoset.toMap()["id"].toString();

            if (! title.isEmpty())
            {
                e.clear();
                e.insert( KIO::UDSEntry::UDS_NAME, photoset_id);
                e.insert( KIO::UDSEntry::UDS_DISPLAY_NAME, title);
                e.insert( KIO::UDSEntry::UDS_FILE_TYPE, S_IFDIR);
                e.insert( KIO::UDSEntry::UDS_ACCESS, 0400);
                e.insert( KIO::UDSEntry::UDS_SIZE, 10);
                listEntry(e,false);
            }
        }

        e.clear();
        listEntry(e,true);

        finished();
    }
    else
    {
        //list photos
        QJson::Parser parser;
        bool ok;
        QStringList folderPaths = url.path().split("/",QString::SkipEmptyParts);

        KUrl getPhotoSetsPhotosQuery("http://api.flickr.com/services/rest/?method=flickr.photosets.getPhotos");
        getPhotoSetsPhotosQuery.addQueryItem("api_key",FLICKR_API_KEY);
        getPhotoSetsPhotosQuery.addQueryItem("format","json");
        getPhotoSetsPhotosQuery.addQueryItem("nojsoncallback","1");

        getPhotoSetsPhotosQuery.addQueryItem("photoset_id",folderPaths[0]);


        QNetworkAccessManager accessManager(this);
        QNetworkReply* getPhotoSetsPhotosReply = accessManager.get(QNetworkRequest(getPhotoSetsPhotosQuery));
        blockUntilFinished(getPhotoSetsPhotosReply);

        QVariantMap result = parser.parse(getPhotoSetsPhotosReply->readAll(),&ok).toMap();

        UDSEntry e;

        foreach (QVariant photo, result["photoset"].toMap()["photo"].toList())
        {
            QString filename = photo.toMap()["title"].toString().append(".jpg");
            QString photo_id = photo.toMap()["id"].toString();

            e.clear();
            e.insert( KIO::UDSEntry::UDS_NAME, photo_id);
            e.insert( KIO::UDSEntry::UDS_DISPLAY_NAME, filename);
            e.insert( KIO::UDSEntry::UDS_MIME_TYPE, "image/jpeg");
            e.insert( KIO::UDSEntry::UDS_FILE_TYPE, S_IFREG); //normal file
            e.insert( KIO::UDSEntry::UDS_ACCESS, 0400);

            e.insert( KIO::UDSEntry::UDS_SIZE, 10);
            listEntry(e,false);
        }

        e.clear();
        listEntry(e,true);

        finished();

    }
    kDebug(7233) << "End List Dir";
}


//we need to have a loop otherwise our signals don't get processed
//turns accessmanager get request into something synchronous
void flickrkio::blockUntilFinished(QNetworkReply *accessManager)
{
    QEventLoop eventLoop;
    //kill the loop when accessmanager finishes - so the program can continue.
    connect(accessManager, SIGNAL(finished()),&eventLoop, SLOT(quit()));
    eventLoop.exec(QEventLoop::ExcludeUserInputEvents);
}

flickrkio::flickrkio( const QByteArray &pool, const QByteArray &app ):
        ForwardingSlaveBase( "flickrkio", pool, app ) {}

#include "flickrkio.moc"
