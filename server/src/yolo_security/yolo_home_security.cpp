#include "../base/ozApp.h"
#include "../base/ozListener.h"
#include "../providers/ozAVInput.h"
#include "../processors/ozMotionDetector.h"
#include "../processors/ozFaceDetector.h"
#include "../processors/ozShapeDetector.h"
#include "../processors/ozRateLimiter.h"
#include "../processors/ozImageScale.h"
#include "../processors/ozAVFilter.h"
#include "../processors/ozMatrixVideo.h"
#include "../processors/ozYOLOv3.h"
#include "../protocols/ozHttpController.h"
#include "../processors/myIdentityFilter.h"

#include "../libgen/libgenDebug.h"

#include <iostream>

//
//

bool feedCompare(const FramePtr &frame, const FeedConsumer *consumer) {
    if (frame->originator()->name() == consumer->name()) {
        return true;
    } else {
        return false;
    }
}
int main( int argc, const char *argv[] )
{
    debugInitialise( "starter-example", "", 0 );

    std::cout << " ---------------------- Starter Example ------------------\n"
            " do a export DBG_PRINT=0/1 to turn off/on logs\n"
            " once running, load up starter-example.html in your browser\n"
            " You should see a traffic cam window as well as motion detection frames in another\n"
            " in real-time\n";

    Info( "Starting" );

    avInit();

    Application app;

    // Two RTSP sources 
    AVInput southeastraw( "southeast", "rtsp://admin:starikovcamera@10.10.10.161/h264Preview_01_sub" );
    AVInput southwestraw( "southwest", "rtsp://admin:starikovcamera@10.10.10.163/h264Preview_01_sub" );
    AVInput backpatioraw( "backpatio", "rtsp://admin:starikovcamera@10.10.10.162/h264Preview_01_sub" );
    AVInput frontentryraw( "frontentry", "rtsp://admin:starikovcamera@10.10.10.183/h264Preview_01_sub" );
    AVInput officeraw( "office", "rtsp://admin:starikovcamera@10.10.10.185/h264Preview_01_sub" );
    AVInput northwestraw( "northwest", "rtsp://admin:starikovcamera@10.10.10.186/h264Preview_01_sub" );

    app.addThread( &southeastraw );
    app.addThread( &southwestraw );
    app.addThread( &backpatioraw );
    app.addThread( &frontentryraw );
    app.addThread( &officeraw );
    app.addThread( &northwestraw );

    RateLimiter southeast(15, southeastraw);
    RateLimiter southwest(15, southwestraw);
    RateLimiter backpatio(15, backpatioraw);
    RateLimiter frontentry(15, frontentryraw);
    RateLimiter office(15, officeraw);
    RateLimiter northwest(15, northwestraw);

    app.addThread( &southeast);
    app.addThread( &southwest);
    app.addThread( &backpatio);
    app.addThread( &frontentry);
    app.addThread( &office);
    app.addThread( &northwest);

    // object detection on six cameras
    YOLODetector yolo("yolo", 6);
    yolo.registerProvider(southwest);
    yolo.registerProvider(southeast);
    yolo.registerProvider(backpatio);
    yolo.registerProvider(frontentry);
    yolo.registerProvider(office);
    yolo.registerProvider(northwest);

    // demultiplexes object detection output
    IdentityFilter sefilter(southeast.name(), yolo, gQueuedVideoLink);
    IdentityFilter swfilter(southwest.name(), yolo, gQueuedVideoLink);
    IdentityFilter bpfilter(backpatio.name(), yolo, gQueuedVideoLink);
    IdentityFilter fefilter(frontentry.name(), yolo, gQueuedVideoLink);
    IdentityFilter ofilter(office.name(), yolo, gQueuedVideoLink);
    IdentityFilter nwfilter(northwest.name(), yolo, gQueuedVideoLink);

    app.addThread( &yolo);
    app.addThread( &sefilter);
    app.addThread( &swfilter);
    app.addThread( &bpfilter);
    app.addThread( &fefilter);
    app.addThread( &ofilter );
    app.addThread( &nwfilter);

    Listener listener;
    app.addThread( &listener );

    HttpController httpController( "watch", 9292 );
    // httpController.addStream( "watch", southeastraw);
    // httpController.addStream( "watch", southwestraw);
    // httpController.addStream( "watch", backpatioraw);
    // httpController.addStream( "watch", frontentryraw);
    // httpController.addStream( "watch", officeraw);
    // httpController.addStream( "watch", northwestraw);

    httpController.addStream( "detect", sefilter);
    httpController.addStream( "detect", swfilter);
    httpController.addStream( "detect", bpfilter);
    httpController.addStream( "detect", fefilter);
    httpController.addStream( "detect", ofilter);
    httpController.addStream( "detect", nwfilter);
    
    listener.addController( &httpController );
    app.run();
}
