#ifndef PACKAGEKIO_H
#define PACKAGEKIO_H

#include <kio/forwardingslavebase.h>

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
    void enterLoop(void);
    bool rewriteUrl(const KUrl&, KUrl&);

};

#endif