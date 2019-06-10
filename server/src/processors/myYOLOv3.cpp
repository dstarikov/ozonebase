#include "../base/oz.h"
#include "../base/ozFeedFrame.h"
#include "myYOLOv3.h"

#include "../base/ozAlarmFrame.h"
#include "darknet.h"

void YOLODetector::construct() {

}

YOLODetector::YOLODetector(const std::string &name, int providers, const Options &options):
    VideoConsumer( cClass(), name, providers ),
    VideoProvider( cClass(), name ),
    Thread( identity() ),
    mOptions( options )
{
    construct();
}

YOLODetector::YOLODetector(VideoProvider &provider, const Options &options, const FeedLink &link):
    VideoConsumer( cClass(), provider, link),
    Thread( identity() ),
    mOptions( options )
{
    construct();
}

YOLODetector::~YOLODetector() {

}

static float get_pixel(image m, int x, int y, int c)
{
    assert(x < m.w && y < m.h && c < m.c);
    return m.data[c*m.h*m.w + y*m.w + x];
}
static float get_pixel_extend(image m, int x, int y, int c)
{
    if(x < 0 || x >= m.w || y < 0 || y >= m.h) return 0;
    /*
    if(x < 0) x = 0;
    if(x >= m.w) x = m.w-1;
    if(y < 0) y = 0;
    if(y >= m.h) y = m.h-1;
    */
    if(c < 0 || c >= m.c) return 0;
    return get_pixel(m, x, y, c);
}
static void set_pixel(image m, int x, int y, int c, float val)
{
    if (x < 0 || y < 0 || c < 0 || x >= m.w || y >= m.h || c >= m.c) return;
    assert(x < m.w && y < m.h && c < m.c);
    m.data[c*m.h*m.w + y*m.w + x] = val;
}
float colors[6][3] = { {1,0,1}, {0,0,1},{0,1,1},{0,1,0},{1,1,0},{1,0,0} };

static float get_color(int c, int x, int max)
{
    float ratio = ((float)x/max)*5;
    int i = floor(ratio);
    int j = ceil(ratio);
    ratio -= i;
    float r = (1-ratio) * colors[i][c] + ratio*colors[j][c];
    return r;
}

void YOLODetector::detect(int numimages, image *ims, uint64_t* ids, uint64_t* timestamps, 
                          std::string* origins, std::vector<Image> &drawn, float thresh,
                          float hier_thresh, float nms, int num_classes, network* net, char** names) {


    detection **dets = calloc(numimages, sizeof(detection*));
    int *nboxes = calloc(numimages, sizeof(int));

    network_predict_images(net, ims, numimages, thresh, hier_thresh, nms, num_classes, dets, nboxes);

    for (int f = 0; f < numimages; f++) {
        std::cout << "img" << f << ": " << nboxes[f] << " detections" << std::endl;

        for (int i = 0; i < nboxes[f]; i++) {
            char labelstr[4096] = {0};
            int classIdx = -1;

            for (int j = 0; j < num_classes; j++) {
                if (dets[f][i].prob[j] > thresh){
                    if (classIdx < 0) {
                        strcat(labelstr, names[j]);
                        classIdx = j;
                    } else {
                        strcat(labelstr, ", ");
                        strcat(labelstr, names[j]);
                    }
                    printf("%s: %.0f%%\n", names[j], dets[f][i].prob[j]*100);
                }
            }

            if (classIdx >= 0) {
                int offset = classIdx*123457 % num_classes;

                unsigned char red = get_color(2,offset, num_classes)*255;
                unsigned char green = get_color(1,offset, num_classes)*255;
                unsigned char blue = get_color(0,offset, num_classes)*255;
                Rgb a;
                RED(((unsigned char*) &a)) = red;
                GREEN(((unsigned char*) &a)) = green;
                BLUE(((unsigned char*) &a)) = blue;

                box b = dets[f][i].bbox;
                int left  = (b.x-b.w/2.)*ims[f].w;
                int right = (b.x+b.w/2.)*ims[f].w;
                int top   = (b.y-b.h/2.)*ims[f].h;
                int bot   = (b.y+b.h/2.)*ims[f].h;

                if(left < 0) left = 0;
                if(right > ims[f].w-1) right = ims[f].w-1;
                if(top < 0) top = 0;
                if(bot > ims[f].h-1) bot = ims[f].h-1;

                // Coord topleft(left, top);
                // Coord botright(right, bot);

                // std::cerr << "left: " << left << ", right: " << right << ", top: " << top << ", bot: " << bot << std::endl;
                // std::cerr << "Coord x: " << topleft.x() << ", Coord y: " << topleft.y() << std::endl;
                // std::cerr << "Coord x: " << botright.x() << ", Coord y: " << botright.y() << std::endl;
                // Coord labelcoord(im.w - right - left, bot - top);
                Coord lo(left, top);
                Coord hi(right, bot);
                Box bbox(lo, hi);
                drawn[f].outline(a, bbox);
                drawn[f].annotate(labelstr, Coord((left + right)/2, top), a, RGB_WHITE);
                // drawn[f].annotate(frameParents[f]->originator()->name().c_str(), Coord((width)/2, height/2), a, RGB_WHITE);
                // std::cerr << " done drawing box and label at " << bbox.centre().x() << ", " << bbox.centre().y() << std::endl;
            }
        }

        free_detections(dets[f], nboxes[f]);

        AlarmFrame *alarmFrame = new AlarmFrame(videoProvider(), origins[f], ids[f], timestamps[f], drawn[f].buffer(), nboxes[f] > 0);
        distributeFrame(FramePtr(alarmFrame));

        mFrameCount++;
        free_image(ims[f]);

    //end
    }

    free(dets);
    free(nboxes);
}

int YOLODetector::run() {
    if (waitForProviders()) {
        AVPixelFormat pixelFormat = videoProvider()->pixelFormat();
        int16_t width = videoProvider()->width();
        int16_t height = videoProvider()->height();
        std::cout << "pixelFormat: " << pixelFormat << std::endl;

        // TODO with options
        char* datacfg = "cfg/coco.data";
        char* cfg = "cfg/yolov3.cfg";
        char* weights = "yolov3.weights";
        char *name_list = "data/coco.names";

        list *options = read_data_cfg(datacfg);
        char **names = get_labels(name_list);

        float thresh = 0.5;
        float hier_thresh = 0.5;
        float nms = 0.45;


        image **alphabet = load_alphabet();
        network *net = load_network(cfg, weights, 0);

        srand(2222222);

        bool saved = false;

        int num_classes = 80;
        int framecache = 6;
        set_batch_network(net, 3);

        uint64_t ts[6];
        uint64_t ids[6];
        std::string origins[6];
        for (int i = 0; i < 6; i++) {
            origins[i] = "";
        }
        std::vector<Image> drawn1(3, Image(AVPixelFormat(2), width, height, NULL));
        std::vector<Image> drawn2(3, Image(AVPixelFormat(2), width, height, NULL));
        image ims[6];

        setReady();

        while (!mStop) {
            if (mStop) break;

            mQueueMutex.lock();
            if ( !mFrameQueue.empty() )
            {
                for ( FrameQueue::iterator iter = mFrameQueue.begin(); iter != mFrameQueue.end(); iter++ )
                {
                    // Only operate on video frames
                    const VideoFrame *frame = dynamic_cast<const VideoFrame *>(iter->get());
                    int idx = -1;
                    if ( frame ) { 
                        auto origin = frame->originator()->name();
                        bool found = false;
                        for (int i = 0; i < 6; i++) {
                            if (origins[i] == origin) {
                                found = true;
                                break;
                            } else if (origins[i] == "" && idx == -1) {
                                origins[i] = origin;
                                idx = i;
                            }
                        }

                        if (found) continue;
                        assert(idx >= 0);

                        Image fImage( frame->pixelFormat(), width, height, frame->buffer().data() );
                        if (idx > 2) {
                            drawn1[idx - 3].convert(fImage);
                        } else {
                            drawn2[idx].convert(fImage);
                        }
                        ims[idx] = make_image(width, height, 3); 
                        for (int y = 0; y < height; y++) {
                            for (int x = 0; x < width; x++) {
                                ims[idx].data[0*width*height + y*width + x] = fImage.red(x, y)/255.;
                                ims[idx].data[1*width*height + y*width + x] = fImage.green(x, y)/255.;
                                ims[idx].data[2*width*height + y*width + x] = fImage.blue(x, y)/255.;
                            }
                        }
                        ids[idx] = frame->id();
                        ts[idx] = frame->timestamp();
                    }

                    if (idx == 5) {
                        detect(3, &ims[3], &ids[3], &ts[3], &origins[3], drawn1, thresh, hier_thresh, nms, num_classes, net, names);
                        detect(3, ims, ids, ts, origins, drawn2, thresh, hier_thresh, nms, num_classes, net, names);
                        for (int i = 0; i < 6; i++) {
                            origins[i] = "";
                        }
                    }

                    //     // for (int x = 0; x < width; x++) {
                    //     //     for (int y = 0; y < height; y++) {
                    //     //         set_pixel(dnetImage, x, y, 0, fImage.red(x, y));
                    //     //         set_pixel(dnetImage, x, y, 1, fImage.green(x, y));
                    //     //         set_pixel(dnetImage, x, y, 2, fImage.blue(x, y));
                    //     //     }
                    //     // }

                    //     //image diskIm = load_image_color("temp.jpg", 0, 0);
                    //     // image sized = letterbox_image(dnetImage, net->w, net->h);
                    //     // save_image(sized, "letterboxed");

                    //     network_predict_image(net, im);
                    //     // network_predict(net, sized.data);
                    //     std::cout << "there are " << num_detections(net, 0.5) << " detections" << std::endl;
                    //     std::cout << "there are " << num_classes << " classes " << std::endl;

                    //     int nboxes = 0;
                    //     detection *dets = get_network_boxes(net, im.w, im.h, thresh, hier_thresh, 0, 1, &nboxes);
                    //     do_nms_sort(dets, nboxes, num_classes, nms);
                    //     // draw_detections(dnetImage, dets, nboxes, thresh, names, alphabet, num_classes);
                    //     // save_image(dnetImage, "detections");

                    //     for (int i = 0; i < nboxes; i++) {
                    //         char labelstr[4096] = {0};
                    //         int classIdx = -1;

                    //         for (int j = 0; j < num_classes; j++) {
                    //             if (dets[i].prob[j] > thresh){
                    //                 if (classIdx < 0) {
                    //                     strcat(labelstr, names[j]);
                    //                     classIdx = j;
                    //                 } else {
                    //                     strcat(labelstr, ", ");
                    //                     strcat(labelstr, names[j]);
                    //                 }
                    //                 printf("%s: %.0f%%\n", names[j], dets[i].prob[j]*100);
                    //             }
                    //         }

                    //         if (classIdx >= 0) {
                    //             int offset = classIdx*123457 % num_classes;

                    //             unsigned char red = get_color(2,offset, num_classes)*255;
                    //             unsigned char green = get_color(1,offset, num_classes)*255;
                    //             unsigned char blue = get_color(0,offset, num_classes)*255;
                    //             Rgb a;
                    //             RED(&a) = red;
                    //             GREEN(&a) = green;
                    //             BLUE(&a) = blue;

                    //             box b = dets[i].bbox;
                    //             int left  = (b.x-b.w/2.)*im.w;
                    //             int right = (b.x+b.w/2.)*im.w;
                    //             int top   = (b.y-b.h/2.)*im.h;
                    //             int bot   = (b.y+b.h/2.)*im.h;

                    //             if(left < 0) left = 0;
                    //             if(right > im.w-1) right = im.w-1;
                    //             if(top < 0) top = 0;
                    //             if(bot > im.h-1) bot = im.h-1;

                    //             // Coord topleft(left, top);
                    //             // Coord botright(right, bot);

                    //             std::cerr << "left: " << left << ", right: " << right << ", top: " << top << ", bot: " << bot << std::endl;
                    //             // std::cerr << "Coord x: " << topleft.x() << ", Coord y: " << topleft.y() << std::endl;
                    //             // std::cerr << "Coord x: " << botright.x() << ", Coord y: " << botright.y() << std::endl;
                    //             // Coord labelcoord(im.w - right - left, bot - top);
                    //             Coord lo(left, top);
                    //             Coord hi(right, bot);
                    //             Box bbox(lo, hi);
                    //             drawn[f].outline(a, bbox);
                    //             drawn[f].annotate(labelstr, Coord((left + right)/2, top), a, RGB_WHITE);
                    //             // std::cerr << " done drawing box and label at " << bbox.centre().x() << ", " << bbox.centre().y() << std::endl;
                    //         }
                    //     }

                    //     free_detections(dets, nboxes);

                    //     AlarmFrame *alarmFrame = new AlarmFrame(videoProvider(), *iter, frame->id(), frame->timestamp(), draw.buffer(), nboxes > 0);
                    //     distributeFrame(FramePtr(alarmFrame));

                    //     mFrameCount++;
                    //     free_image(im);
                    //     // free_image(diskIm);
                    //     // free_image(sized);
                } 
                mFrameQueue.clear();
            } 

            mQueueMutex.unlock();
            checkProviders();
            usleep( INTERFRAME_TIMEOUT);
        }
    }
    FeedProvider::cleanup();
    FeedConsumer::cleanup();
    return( !ended() );
}
