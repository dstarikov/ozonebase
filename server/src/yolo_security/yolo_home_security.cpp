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
#include "../protocols/ozHttpController.h"
#include "../consumers/ozMp4FileOutput.h"
#include "../processors/myYOLOv3.h"
#include "../processors/myIdentityFilter.h"

#include "../libgen/libgenDebug.h"

#include <iostream>

bool feedCompare(const FramePtr &frame, const FeedConsumer *consumer) {
    if (frame->originator()->name() == consumer->name()) {
        return true;
    } else {
        return false;
    }
}
int main( int argc, const char *argv[] )
{
    debugInitialise( "yolo security", "", 0 );
    Info( "Starting" );

    avInit();

    Application app;

    // Two RTSP sources 
    // AVInput southeastraw( "southeast", "rtsp://admin:starikovcamera@10.10.10.161/h264Preview_01_main" );
    // AVInput southwestraw( "southwest", "rtsp://admin:starikovcamera@10.10.10.163/h264Preview_01_main" );
    // AVInput backpatioraw( "backpatio", "rtsp://admin:starikovcamera@10.10.10.162/h264Preview_01_main" );
    // AVInput frontentryraw( "frontentry", "rtsp://admin:starikovcamera@10.10.10.183/h264Preview_01_main" );
    // AVInput officeraw( "office", "rtsp://admin:starikovcamera@10.10.10.185/h264Preview_01_main" );
    // AVInput northwestraw( "northwest", "rtsp://admin:starikovcamera@10.10.10.186/h264Preview_01_main" );

    Options recordedPlaybackOptions;
    recordedPlaybackOptions.add("realtime", true);
    recordedPlaybackOptions.add("loop", true);
    AVInput southeastraw( "southeast", "/media/test_videos/southeast.mp4" , recordedPlaybackOptions);
    AVInput southwestraw( "southwest", "/media/test_videos/southwest.mp4", recordedPlaybackOptions );
    AVInput backpatioraw( "backpatio", "/media/test_videos/backpatio.mp4", recordedPlaybackOptions );
    AVInput frontentryraw( "frontentry", "/media/test_videos/frontentry.mp4", recordedPlaybackOptions );
    AVInput officeraw( "office", "/media/test_videos/office.mp4", recordedPlaybackOptions );
    AVInput northwestraw( "northwest", "/media/test_videos/northwest.mp4", recordedPlaybackOptions );

    app.addThread( &southeastraw );
    app.addThread( &southwestraw );
    app.addThread( &backpatioraw );
    app.addThread( &frontentryraw );
    app.addThread( &officeraw );
    app.addThread( &northwestraw );

    // Limit the frame rate
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

    // Run Object detection on six cameras
    YOLODetector yolo("yolo", 6);
    yolo.registerProvider(southwest);
    yolo.registerProvider(southeast);
    yolo.registerProvider(backpatio);
    yolo.registerProvider(frontentry);
    yolo.registerProvider(office);
    yolo.registerProvider(northwest);

    // Demultiplex object detection output
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

    HttpController rawHttpController( "watch", 9292 );
    rawHttpController.addStream( "watch", southeastraw);
    rawHttpController.addStream( "watch", southwestraw);
    rawHttpController.addStream( "watch", backpatioraw);
    rawHttpController.addStream( "watch", frontentryraw);
    rawHttpController.addStream( "watch", officeraw);
    rawHttpController.addStream( "watch", northwestraw);

    HttpController detectHttpController( "detect", 9293 );
    detectHttpController.addStream( "detect", sefilter);
    detectHttpController.addStream( "detect", swfilter);
    detectHttpController.addStream( "detect", bpfilter);
    detectHttpController.addStream( "detect", fefilter);
    detectHttpController.addStream( "detect", ofilter);
    detectHttpController.addStream( "detect", nwfilter);
    
    listener.addController( &rawHttpController );
    listener.addController( &detectHttpController );
    app.run();
}
