
#include "wavdecoder.hpp"

#include <iostream>
#include <memory>

WAVDecoder::WAVDecoder() :
  sfinfo({0}),
  sffile(NULL),
  auto_rewind(false) {

}

WAVDecoder::~WAVDecoder() {

  if(sffile) {
    sf_close(sffile);
  }

}

bool WAVDecoder::open(std::string _filename) {

  // copy string locally to emit a valid c pointer
  filename = _filename;
  // open file
  sffile = sf_open(filename.c_str(), SFM_READ, &sfinfo);

  if(sffile == NULL) {
    std::cerr<<"sf_open error "<<sf_strerror(NULL)<<"\n";
    return false;
  }

  // fill parameters structure
  auto &p = get_parameters();
  p.channels = 2;
  p.samplerate_hz = sfinfo.samplerate;
  p.bitrate_hz = 0;

  return true;
}

void WAVDecoder::start(void) {
  return;
}

void WAVDecoder::join(void) {
  return;
}

void WAVDecoder::exit(void) {
  return;
}

void WAVDecoder::rewind(void) {
  if(sffile == NULL)
    return;
  sf_seek(sffile, 0, SEEK_SET);
}

void WAVDecoder::set_auto_rewind(bool b) {
  auto_rewind = b;
}

std::vector<audio_frame_t> WAVDecoder::pop_frames(unsigned int nframes) {

  const int nchannels = sfinfo.channels;
  int rsamples = nframes * nchannels;
  auto buffer = std::make_unique<float[]>(rsamples);

  std::vector<audio_frame_t> out;
  sf_count_t rsz = 0;
  while(1) {
    rsz = sf_readf_float(sffile, buffer.get(), nframes);
    if(rsz > 0) {
      break;
    }
    else {
      if(auto_rewind) {
        rewind();
      }
      else {
        return {};
      }
    }
  }

  for(int i=0;i<rsz;i++) {
    auto k = i*nchannels;
    float sl,sr;

    sl = buffer[k];

    if (nchannels == 1)
      sr = sl;
    else
      sr = buffer[k+1];

    out.push_back({sl,sr});
  }

  return out;
}
