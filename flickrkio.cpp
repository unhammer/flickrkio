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

/* Currently not used for anything, since we just link: */
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

    QMap<QString,QString> queryArgs;

    queryArgs["photo_id"] = photo_id;

    QVariantMap result = flickrQuery("flickr.photos.getSizes",queryArgs);

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
    QStringList folderPaths = url.path().split("/",QString::SkipEmptyParts);

    if (url.path().isEmpty() || url.path() == "/") // looking at our list of sets
    {
        AuthInfo info;
        QString userId;

        info.url = KUrl("flickr://");

        infoMessage("here");

        if (! checkCachedAuthentication(info) || info.password.isEmpty() || true)
        {
            //open password dialog is rubbish..it doesn't do anything...GAH
            //FIXME!!!
            //openPasswordDialog(info,"Please enter your flickr username (note password isn't actually used)");
        }

        if (! url.user().isEmpty())
        {
            info.username = url.user();
        }

        if (! info.username.isEmpty())
        {
            QMap<QString,QString> queryArgs;
            queryArgs["username"] = info.username;
            QVariantMap result = flickrQuery("flickr.people.findByUsername",queryArgs);

            userId = result["user"].toMap()["nsid"].toString();

            if (userId.isEmpty())
            {
                warning(QString("Could not find Flickr user"));
                finished();
                return;
            }
            else
            {
                infoMessage(info.username);
                cacheAuthentication(info);
            }
        }
        else
        {
            warning("no flickr username supplied");
        }



        QMap<QString,QString> queryArgs;
        queryArgs["user_id"] = userId;
        QVariantMap result = flickrQuery("flickr.photosets.getList", queryArgs);
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
    else if(folderPaths.size() == 1) // looking at a certain set
    {
        //list photos
        QMap <QString,QString> queryArgs;
        queryArgs["photoset_id"] = folderPaths[0];
        QVariantMap result = flickrQuery("flickr.photosets.getPhotos",queryArgs);

        UDSEntry e;
	
        foreach (QVariant photo, result["photoset"].toMap()["photo"].toList())
        {
            QString filename = photo.toMap()["title"].toString().append(".jpg");
            QString photo_id = photo.toMap()["id"].toString();

            e.clear();
            e.insert( KIO::UDSEntry::UDS_NAME, photo_id);
	    e.insert( KIO::UDSEntry::UDS_DISPLAY_NAME, filename);
            e.insert( KIO::UDSEntry::UDS_FILE_TYPE, S_IFDIR); // directory of sizes
            e.insert( KIO::UDSEntry::UDS_ACCESS, 0400);
            e.insert( KIO::UDSEntry::UDS_SIZE, 10);
	    if (false) {
		QMap<QString,QString> queryArgs_p;
		queryArgs_p["photo_id"] = photo_id;
		QVariantMap result = flickrQuery("flickr.photos.getSizes",queryArgs_p);
		QString photoUrl_t;
		QVariantList sizes = result["sizes"].toMap()["size"].toList();
		foreach (QVariant size, sizes ) {
		    QString label = size.toMap()["label"].toString();
		    photoUrl_t = size.toMap()["source"].toString();
		    if(label == QString("Thumbnail")) break;
		}
		e.insert( KIO::UDSEntry::UDS_ICON_NAME, photoUrl_t);
	    }
            listEntry(e,false);
        }

        e.clear();
        listEntry(e,true);
        finished();

    }
    else			// looking at a certain photo, list of sizes
    {
	UDSEntry e;

        QMap <QString,QString> queryArgs1;
        queryArgs1["photo_id"] = folderPaths[1];
        QVariantMap result = flickrQuery("flickr.photos.getSizes",queryArgs);
	
	QString webpage = result.toMap()["url"].toString();
	e.clear();
	e.insert( KIO::UDSEntry::UDS_NAME, webpage);
	e.insert( KIO::UDSEntry::UDS_DISPLAY_NAME, QString("web page"));
	e.insert( KIO::UDSEntry::UDS_URL, webpage);
	// e.insert( KIO::UDSEntry::UDS_MIME_TYPE, "image/jpeg"); // FIXME
	e.insert( KIO::UDSEntry::UDS_FILE_TYPE, S_IFREG); // normal file? FIXME
	e.insert( KIO::UDSEntry::UDS_ACCESS, 0400);
	e.insert( KIO::UDSEntry::UDS_SIZE, 10);
	listEntry(e,false);

        //list sizes
        QMap <QString,QString> queryArgs;
        queryArgs["photo_id"] = folderPaths[1];
        QVariantMap result = flickrQuery("flickr.photos.getSizes",queryArgs);
        foreach (QVariant size, result["sizes"].toMap()["size"].toList())
        {
            QString label = size.toMap()["label"].toString();
            QString filename = size.toMap()["source"].toString();

            e.clear();
            e.insert( KIO::UDSEntry::UDS_NAME, filename);
            e.insert( KIO::UDSEntry::UDS_DISPLAY_NAME, label);
	    // opens in web browser when we set this:
            e.insert( KIO::UDSEntry::UDS_URL, filename);
            e.insert( KIO::UDSEntry::UDS_MIME_TYPE, "image/jpeg");
            e.insert( KIO::UDSEntry::UDS_FILE_TYPE, S_IFREG); // normal file
            e.insert( KIO::UDSEntry::UDS_ACCESS, 0400);
	    // ironically flickr doesn't provide file size in "sizes":
            e.insert( KIO::UDSEntry::UDS_SIZE, 10);
            listEntry(e,false);
        }

        e.clear();
        listEntry(e,true);
        finished();

    }
}


QVariantMap flickrkio::flickrQuery(QString method, QMap<QString,QString> queryItems)
{
    QEventLoop eventLoop;

    KUrl query("http://api.flickr.com/services/rest/");
    query.addQueryItem("method",method);
    query.addQueryItem("api_key",FLICKR_API_KEY);
    query.addQueryItem("format","json");
    query.addQueryItem("nojsoncallback","1");

    QMapIterator<QString, QString> i(queryItems);
    while (i.hasNext())
    {
        i.next();
        query.addQueryItem(i.key(),i.value());
    }

    QNetworkAccessManager accessManager(this);

    //block until finished - when network operation finished, event loop quits and we continue our function

    QNetworkReply* reply = accessManager.get(QNetworkRequest(query));
    connect(reply, SIGNAL(finished()),&eventLoop, SLOT(quit()));
    eventLoop.exec(QEventLoop::ExcludeUserInputEvents);

    //return parsed results
    QJson::Parser parser;
    bool ok;

    return parser.parse(reply->readAll(),&ok).toMap();
    //FIXME if !ok -> raise some kind of error.
}


flickrkio::flickrkio( const QByteArray &pool, const QByteArray &app ):
        ForwardingSlaveBase( "flickrkio", pool, app ) {}

#include "flickrkio.moc"
