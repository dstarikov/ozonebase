/** @addtogroup Processors */
/*@{*/

#ifndef OZ_YOLO_V3_H
#define OZ_YOLO_V3_H

#include "../base/ozFeedBase.h"
#include "../base/ozFeedProvider.h"
#include "../base/ozFeedConsumer.h"
#include "../base/ozDetector.h"
#include "../base/ozZone.h"
#include "../base/ozOptions.h"
#include "darknet.h"

///
/// Processor that detects objects in a video frame
///
class YOLODetector : public virtual Detector
{
CLASSID(YOLODetector);

private:
	Options	mOptions;
    float threshold;
    void construct();

public:
    YOLODetector(const std::string &name, int providers, const Options &options=gNullOptions);
    YOLODetector(VideoProvider &provider, const Options &options=gNullOptions, const FeedLink &link=gQueuedVideoLink);
    ~YOLODetector();
    
    uint16_t width() const { return( videoProvider()->width() ); }
    uint16_t height() const { return( videoProvider()->height() ); }
    AVPixelFormat pixelFormat() const { return( Image::getFfPixFormat( Image::FMT_RGB ) ); }
    FrameRate frameRate() const { return( videoProvider()->frameRate() ); }

    void detect(int numimages, image *ims, uint64_t* ids, uint64_t* timestamps, 
                std::string* origins, std::vector<Image> &drawn, float thresh,
                float hier_thresh, float nms, int num_classes, network* net, char** names);

protected:
    int run();
};

#endif // OZ_YOLO_V3_H


/*@}*/
