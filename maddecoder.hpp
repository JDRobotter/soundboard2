#ifndef _MADDECODER_HPP
#define _MADDECODER_HPP

#include <string>
#include <fstream>
#include <mad.h>
#include <thread>
#include <vector>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <atomic>

#include "decoder.hpp"

class MADDecoder: public Decoder {

  public:
    MADDecoder();
    virtual ~MADDecoder();

    bool open(std::string filename);

    void start(void);

    void join(void);

    // rewing decoding to start of file, keeping existing buffers
    void rewind(void);

    // if set to true, will rewind at eof
    void set_auto_rewind(bool);

    // wait until queue is not full
    void wait_for_space_available(void);

    // pop at most n audio frames from decoder
    std::vector<audio_frame_t> pop_frames(unsigned int n);

    void exit(void);

  private:

    static enum mad_flow output_mad_callback(void *data,
      struct mad_header const *header, struct mad_pcm *pcm);

    static enum mad_flow error_mad_callback(void *data,
      struct mad_stream *stream, struct mad_frame *frame);

    static enum mad_flow header_mad_callback(void *data, struct mad_header const *header);

    static enum mad_flow input_mad_callback(void *data, struct mad_stream *stream);

    void decode(void);

    // convert mad sample to float
    static float mad_sample_to_float(mad_fixed_t sample);

    // internal buffer for file read
    unsigned char buffer[4096];

    // mp3 input file
    std::ifstream ifile;
    std::string filename;
    
    // mad decoder structure
    struct mad_decoder decoder;

    // thread running mad decoder
    std::unique_ptr<std::thread> decoder_thread;

    // maximum number of stored frames
    const unsigned max_frames = 1024;
    // internal decoded frames queue
    std::queue<audio_frame_t> frames;
    // associated mutex
    std::mutex frames_mutex;
    // associated condition variable with new frames in queue
    std::condition_variable frames_available_cv;
    // associated condition variable with new frames consumed from queue
    std::condition_variable frames_consumed_cv;

    // associated condition variable with parameters updated
    std::mutex parameters_mutex;
    std::condition_variable parameters_updated_cv;

    // file mutex
    std::mutex file_mutex;

    // thread will try to quit when true
    std::atomic<bool> quit;
    // end of file reached
    std::atomic<bool> eof;
    // rewind at end of file
    std::atomic<bool> auto_rewind;
};

#endif
