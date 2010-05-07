#include "flickrkio.h"
#include <kdebug.h> 
#include <kcomponentdata.h>
#include <QFile>
#include <qfileinfo.h>
#include <kdebug.h>
#include <KApplication>
#include <kcmdlineargs.h>
#include <qjson/parser.h>

using namespace KIO;

extern "C" int KDE_EXPORT kdemain( int argc, char **argv )
{                                   
  kDebug(7233) << "Entering function";
  KComponentData instance( "kio_flirckkio" );

  //need an app to have an eventloop
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
  kDebug() << "Get " << url.prettyUrl();
}

void flickrkio::stat( const KUrl &url )
{
  Q_UNUSED(url);
  //TODO ...this entire function..
  finished();
}

// void flickrkio::addPackage( Package* package)
// {
//     UDSEntry e;
// 
//     e.clear();
//     e.insert( KIO::UDSEntry::UDS_NAME, QFile::decodeName(package->name().toLocal8Bit()));
//     e.insert( KIO::UDSEntry::UDS_FILE_TYPE, S_IFDIR);
//     e.insert( KIO::UDSEntry::UDS_ACCESS, 0400);
//     e.insert( KIO::UDSEntry::UDS_SIZE, 10);
//     //TODO check exit status..handle failure more gracefully


//needed to fix pure virtual function in parent
bool flickrkio::rewriteUrl(const KUrl&, KUrl&)
{
  return true;
}

//we need to have a loop otherwise our signals don't get processed
void flickrkio::enterLoop()
{
  QEventLoop eventLoop;

  //kill the loop when flickrkio finishes - so the program can end.
  connect(this, SIGNAL(leaveModality()),&eventLoop, SLOT(quit()));
  eventLoop.exec(QEventLoop::ExcludeUserInputEvents);
}



void flickrkio::listDir( const KUrl &url )
{
  kDebug(7233) << "Start List Dir";
  
  
  // create a JSonDriver instance
    QJson::Parser parser;
    bool ok;
    
  
    UDSEntry e;

    e.clear();
    e.insert( KIO::UDSEntry::UDS_NAME, QFile::decodeName("Cheese"));
    e.insert( KIO::UDSEntry::UDS_FILE_TYPE, S_IFDIR);
    e.insert( KIO::UDSEntry::UDS_ACCESS, 0400);
    e.insert( KIO::UDSEntry::UDS_SIZE, 10);
  
    listEntry(e,false);
    
    listEntry(e,true);
    
    finished();
//   enterLoop();

//   emit leaveModality();


  kDebug(7233) << "End List Dir";
}



flickrkio::flickrkio( const QByteArray &pool, const QByteArray &app ):
  ForwardingSlaveBase( "flickrkio", pool, app ) {}

#include "flickrkio.moc"
