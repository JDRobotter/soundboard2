#ifndef _DECODER_HPP
#define _DECODER_HPP

#include <string>
#include <vector>

typedef struct {

  int channels;
  int bitrate_hz;
  int samplerate_hz;

} audio_parameters_t;

typedef struct {
  float left;
  float right;
} audio_frame_t;

class Decoder {

  public:
    Decoder() {
          
      parameters.channels = -1;
      parameters.bitrate_hz = -1;
      parameters.samplerate_hz = -1;
    }

    virtual ~Decoder() {
    }

    audio_parameters_t& get_parameters(void) {
      return parameters;
    }

    virtual bool open(std::string filename) = 0;

    virtual void start(void) = 0;

    virtual void join(void) = 0;

    virtual void exit(void) = 0;

    virtual void rewind(void) = 0;

    virtual void set_auto_rewind(bool) = 0;

    virtual std::vector<audio_frame_t> pop_frames(unsigned int n) = 0;

  private:
    
    audio_parameters_t parameters;

};

#endif//_DECODER_HPP
