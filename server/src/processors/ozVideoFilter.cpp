#include "../base/oz.h"
#include "ozVideoFilter.h"

#include "../base/ozFeedFrame.h"
#include <sys/time.h>
#include <iostream>

/**
* @brief 
*
* @param provider
*/
DirectVideoFilter::DirectVideoFilter( AudioVideoProvider &provider ) :
    VideoConsumer( cClass(), provider, FeedLink( FEED_QUEUED, AudioVideoProvider::videoFramesOnly ) ),
    VideoProvider( cClass(), provider.name() )
{
}

/**
* @brief 
*
* @param frame
* @param 
*
* @return 
*/
static bool DirectVideoFilter::sameIdVideoFrames( const FramePtr &frame, const FeedConsumer * c)
{
    const VideoFrame *videoFrame = dynamic_cast<const VideoFrame *>(frame.get());
    std::cout << "frame id: " << videoFrame->originator()->identity() << ", consumer identity " << c->identity() << std::endl;
    if (videoFrame != NULL && videoFrame->originator()->identity() == c->identity()) {
        return true;
    } else {
        return false;
    }
}

DirectVideoFilter::DirectVideoFilter( AudioVideoProvider &provider, std::string &identity ) :
    VideoConsumer( cClass(), provider.name() ),
    VideoProvider( cClass(), provider.name() )
{
    FeedLink fl(FeedLinkType::FEED_QUEUED, DirectVideoFilter::sameIdVideoFrames);
    registerProvider(provider, fl);
}

/**
* @brief 
*/
DirectVideoFilter::~DirectVideoFilter()
{
}

// Don't locally queue just send the frames on
/**
* @brief 
*
* @param framePtr
* @param provider
*
* @return 
*/
bool DirectVideoFilter::queueFrame( const FramePtr &framePtr, FeedProvider *provider )
{
    distributeFrame( framePtr );
    return( true );
}

/**
* @brief 
*
* @param provider
*/
QueuedVideoFilter::QueuedVideoFilter( AudioVideoProvider &provider ) :
    VideoConsumer( cClass(), provider, FeedLink( FEED_QUEUED, std::list<FeedComparator> {AudioVideoProvider::videoFramesOnly} ) ),
    VideoProvider( cClass(), provider.name() ),
    Thread( identity() )
{
}

/**
* @brief 
*/
QueuedVideoFilter::~QueuedVideoFilter()
{
}

/**
* @brief 
*
* @return 
*/
int QueuedVideoFilter::run()
{
    if ( waitForProviders() )
    {
        while ( !mStop )
        {
            mQueueMutex.lock();
            if ( !mFrameQueue.empty() )
            {
                Debug( 3, "Got %zd frames on queue", mFrameQueue.size() );
                for ( FrameQueue::iterator iter = mFrameQueue.begin(); iter != mFrameQueue.end(); iter++ )
                {
                    distributeFrame( *iter );
                    mFrameCount++;
                }
                mFrameQueue.clear();
            }
            mQueueMutex.unlock();
            checkProviders();
            // Quite short so we can always keep up with the required packet rate for 25/30 fps
            usleep( INTERFRAME_TIMEOUT );
        }
    }
    FeedProvider::cleanup();
    FeedConsumer::cleanup();
    return( !ended() );
}
