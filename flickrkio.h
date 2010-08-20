#ifndef PACKAGEKIO_H
#define PACKAGEKIO_H

#include <kio/forwardingslavebase.h>
#include <QNetworkReply>

#define FLICKR_API_KEY "aa7b4ac1428ee1f50e6acaf11f23708d"
#define FLICKR_SECRET "8d31579fd9b68ede"

/**
  This class implements an installed package viewing kioslave
 */
class flickrkio :  public KIO::ForwardingSlaveBase
{
    Q_OBJECT
public:
    flickrkio( const QByteArray &pool, const QByteArray &app );
    void get( const KUrl &url );
    void listDir( const KUrl &url );
    void stat( const KUrl &url );
    bool rewriteUrl(const KUrl&, KUrl&);

private:
    /** Executes Flickr API query synchronously, parses JSON returns as QVariants.*/
    QVariantMap flickrQuery(QString method, QMap<QString,QString> queryItems);

};

#endif
