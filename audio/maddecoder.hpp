#include <string>
#include <fstream>
#include <mad.h>
#include <thread>
#include <vector>
#include <mutex>
#include <condition_variable>

class MADDecoder {

  public:
    MADDecoder();
    ~MADDecoder();

    static enum mad_flow output_mad_callback(void *data,
      struct mad_header const *header, struct mad_pcm *pcm);

    static enum mad_flow error_mad_callback(void *data,
      struct mad_stream *stream, struct mad_frame *frame);

    static enum mad_flow header_mad_callback(void *data, struct mad_header const *header);

    static enum mad_flow input_mad_callback(void *data, struct mad_stream *stream);

    bool open(std::string filename);

    void start(void);

    void join(void);

    // wait until queue is not full
    void wait_for_space_available(void);

    // pop at most n audio samples from decoder
    std::vector<float> pop_samples(int n);

  private:

    void decode(void);

    // convert mad sample to float
    static float mad_sample_to_float(mad_fixed_t sample);

    // internal buffer for file read
    unsigned char buffer[4096];

    // mp3 input file
    std::ifstream ifile;
    
    // mad decoder structure
    struct mad_decoder decoder;

    // thread running mad decoder
    std::unique_ptr<std::thread> decoder_thread;

    // maximum number of stored samples
    const int max_samples = 2048;
    // internal decoded samples queue
    std::vector<float> samples;
    // associated mutex
    std::mutex samples_mutex;
    // associated condition variable with space available in queue
    std::condition_variable samples_available_cv;
};
