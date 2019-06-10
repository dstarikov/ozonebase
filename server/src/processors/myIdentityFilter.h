/** @addtogroup Processors */
/*@{*/


#ifndef MY_IDENTITY_FILTER_H
#define MY_IDENTITY_FILTER_H

#include "../base/ozFeedBase.h"
#include "../base/ozFeedProvider.h"
#include "../base/ozFeedConsumer.h"

///
/// Processor that delays frames for a specified period
///
class IdentityFilter : public VideoConsumer, public VideoProvider, public Thread
{
CLASSID(IdentityFilter);

protected:
	std::string id_filter;
    VideoProvider &filtersrc;

public:
    IdentityFilter( const std::string &filter, VideoProvider &provider, const FeedLink &link);
    ~IdentityFilter();

    uint16_t width() const;
    uint16_t height() const;
    AVPixelFormat pixelFormat() const;
    FrameRate frameRate() const;

protected:
    int run();
};

#endif // MY_IDENTITY_FILTER_H


/*@}*/