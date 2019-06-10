#include <iostream>
#include "../base/oz.h"
#include "myIdentityFilter.h"

#include "../base/ozFeedFrame.h"

/**
* @brief 
*
* @param name
* @param delay
*/
IdentityFilter::IdentityFilter( const std::string &filter, VideoProvider &provider, const FeedLink &link):
    VideoConsumer( cClass(), provider.name() + "_" + filter, 1),
    VideoProvider(cclass(), provider.name() + "_" + filter),
    Thread( identity() )
{
    registerProvider(provider, link);
    id_filter = filter;
    // filtersrc = filter;
}

/**
* @brief 
*/
IdentityFilter::~IdentityFilter()
{
}

AVPixelFormat IdentityFilter::pixelFormat() const {
    return videoProvider()->pixelFormat();
}

uint16_t IdentityFilter::width() const {
    return videoProvider()->width();
}

uint16_t IdentityFilter::height() const {
    return videoProvider()->height();
}

FrameRate IdentityFilter::frameRate() const {
    return videoProvider()->frameRate();
}

/**
* @brief 
*
* @return 
*/
int IdentityFilter::run()
{
    if ( waitForProviders() )
    {
        while ( !mStop )
        {
            mQueueMutex.lock();
            if ( !mFrameQueue.empty() )
            {
                for ( FrameQueue::iterator iter = mFrameQueue.begin(); iter != mFrameQueue.end(); )
                {
                    FramePtr fptr(*iter);
                    FeedFrame* f = fptr.get();
                    // const VideoFrame *frame = dynamic_cast<const VideoFrame *>(iter->get());
                    if (f) {
                        if (f->src == id_filter) {
                            // std::cout << "idfilter " << id_filter << " got a frame" << std::endl;
                            distributeFrame( fptr );
                            iter = mFrameQueue.erase(iter);
                            continue;
                        }
                    }
                    iter++;
                }
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
