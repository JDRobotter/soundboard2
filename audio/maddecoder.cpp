#include "maddecoder.hpp"

#include <iostream>
#include <cstring>

// static functions

enum mad_flow MADDecoder::output_mad_callback(void *data,
  struct mad_header const *header, struct mad_pcm *pcm) {

  // access object
  MADDecoder *mad = (MADDecoder*)data;

  std::cerr<<"output "<<std::this_thread::get_id()
    <<" |CH "<<pcm->channels
    <<" |BR "<<header->bitrate
    <<" |SR "<<header->samplerate
    <<"\n";

  // sleep until there is space available in the queue
  mad->wait_for_space_available();

  // decode frame to floats, lock queue
  for(unsigned i=0; i<pcm->length; i++) {
    float left = MADDecoder::mad_sample_to_float(pcm->samples[0][i]);
    float right = MADDecoder::mad_sample_to_float(pcm->samples[1][i]);
  }

  return MAD_FLOW_CONTINUE;
}

enum mad_flow MADDecoder::error_mad_callback(void *data,
  struct mad_stream *stream, struct mad_frame *frame) {

  std::cerr<<"ERROR "<<stream->error<<" "<<mad_stream_errorstr(stream)<<"\n";
  return MAD_FLOW_CONTINUE;
}

enum mad_flow MADDecoder::header_mad_callback(void *data, struct mad_header const *header) {
  return MAD_FLOW_CONTINUE;
}

enum mad_flow MADDecoder::input_mad_callback(void *data, struct mad_stream *stream) {
  // access object
  MADDecoder *mad = (MADDecoder*)data;
  auto& ifile = mad->ifile;

  size_t offset = sizeof(mad->buffer);
  size_t rem = 0;

  // if next_frame is not null, next_frame mark start of next frame in current buffer
  if(stream->next_frame) {
    offset = stream->next_frame - mad->buffer;
    // compute length of remaining data in buffer
    rem = sizeof(mad->buffer) - offset;
    // move remaining data to start of buffer
    memmove(mad->buffer, stream->next_frame, rem);
  }
  
  // read data from file, copy to buffer starting after remaining data
  ifile.read((char*)mad->buffer + rem, offset);
  auto rsize = ifile.gcount();

  if(rsize == 0) {
    std::cerr<<"EOF\n";
    return MAD_FLOW_STOP;
  }

  // feed data to mad (read data + remaining data)
  mad_stream_buffer(stream, mad->buffer, rsize + rem);

  return MAD_FLOW_CONTINUE;
}

float MADDecoder::mad_sample_to_float(mad_fixed_t sample) {
  const double factor = 1.0/(1<<MAD_F_FRACBITS);
  return std::min(1.0, std::max(-1.0, 0.5*sample*factor));
}

// member functions

MADDecoder::MADDecoder() {

}

MADDecoder::~MADDecoder() {

}

bool MADDecoder::open(std::string filename) {

  ifile.open(filename, std::ios::in | std::ios::binary);

  if(!ifile) {
    return false;
  }
  
  std::cerr<<"init "<<std::this_thread::get_id();
  mad_decoder_init(&decoder, (void*)this,
    input_mad_callback,
    header_mad_callback,
    NULL /*filter*/,
    output_mad_callback,
    error_mad_callback,
    NULL /*message*/);

  return true;
}

void MADDecoder::decode() {
  mad_decoder_run(&decoder, MAD_DECODER_MODE_SYNC);
}

void MADDecoder::start() {
  decoder_thread = std::make_unique<std::thread>(&MADDecoder::decode, this);
}

void MADDecoder::join() {
  decoder_thread->join();
}

void MADDecoder::wait_for_space_available(void) {
  // acquire samples mutex
  std::unique_lock<std::mutex> mlock(samples_mutex);

  while(samples.size() < max_samples) {
    // wait for notification of available space
    samples_available_cv.wait(mlock);
  }
}
