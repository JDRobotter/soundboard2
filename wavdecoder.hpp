#ifndef _WAVDECODER_HPP
#define _WAVDECODER_HPP

#include "sndfile.h"

#include "decoder.hpp"

#include <atomic>

class WAVDecoder : public Decoder {

  public:
    WAVDecoder();
    virtual ~WAVDecoder();

    bool open(std::string filename);

    void start(void);

    void join(void);

    void exit(void);

    void rewind(void);

    void set_auto_rewind(bool);

    void wait_for_space_available(void);

    std::vector<audio_frame_t> pop_frames(unsigned int n);

  private:

    std::string filename;

    SF_INFO sfinfo;

    SNDFILE *sffile;

    std::atomic<bool> auto_rewind;
};

#endif//_WAVDECODER_HPP
