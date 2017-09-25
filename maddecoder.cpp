#include "maddecoder.hpp"

#include <iostream>
#include <cstring>

// static functions

enum mad_flow MADDecoder::output_mad_callback(void *data,
  struct mad_header const *header, struct mad_pcm *pcm) {
  // access object
  MADDecoder *mad = (MADDecoder*)data;
  {
    std::unique_lock<std::mutex> mlock(mad->parameters_mutex);
    auto& p = mad->get_parameters();

    // for simplicity we will always output stereo from this decoder
    p.channels = 2;
    p.bitrate_hz = header->bitrate;
    p.samplerate_hz = header->samplerate;
    mad->parameters_updated_cv.notify_all();
  }

  // sleep until there is space available in the queue
  mad->wait_for_space_available();
  // check quit
  if(mad->quit)
    return MAD_FLOW_STOP;

  // decode frame to floats, lock queue
  {
    std::unique_lock<std::mutex> mlock(mad->frames_mutex);
    for(unsigned i=0; i<pcm->length; i++) {
      float left,right;

      left = MADDecoder::mad_sample_to_float(pcm->samples[0][i]);
      if(pcm->channels == 2) {
        right = MADDecoder::mad_sample_to_float(pcm->samples[1][i]);
      }
      else {
        right = left;
      }

      mad->frames.push({left, right});
    }
    mad->frames_available_cv.notify_all();
  }

  return MAD_FLOW_CONTINUE;
}

enum mad_flow MADDecoder::error_mad_callback(void *data,
  struct mad_stream *stream, struct mad_frame *frame) {

  //std::cerr<<"ERROR "<<stream->error<<" "<<mad_stream_errorstr(stream)<<"\n";
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
  
  std::streamsize rsize = 0;
  while(1) {
    // read data from file, copy to buffer starting after remaining data
    {
      std::unique_lock<std::mutex> mlock(mad->file_mutex);
      ifile.read((char*)mad->buffer + rem, offset);
      rsize = ifile.gcount();
    }

    if(rsize > 0) {
      // data was successfully read
      break;
    }
    else {
      // no data read
      if(mad->auto_rewind) {
        mad->rewind();
        // try again
      }
      else {
        // end of file reached
        return MAD_FLOW_STOP;
      }
    }

  }//while(1)

  // feed data to mad (read data + remaining data)
  mad_stream_buffer(stream, mad->buffer, rsize + rem);

  return MAD_FLOW_CONTINUE;
}

float MADDecoder::mad_sample_to_float(mad_fixed_t sample) {
  const float factor = 1.0f/(1<<MAD_F_FRACBITS);
  return std::min(1.0f, std::max(-1.0f, sample*factor));
}

// member functions
MADDecoder::MADDecoder():
  ifile(),
  filename("") {
  quit = false;
  eof = false;
}

MADDecoder::~MADDecoder() {
  // signal thread to quit
  quit = true;
  frames_consumed_cv.notify_all();

  // wait for thread to quit
  join();

  // close file
  ifile.close();

  // close mad decoder
  mad_decoder_finish(&decoder);
}

bool MADDecoder::open(std::string _filename) {
  filename = _filename;

  {
    std::unique_lock<std::mutex> mlock(file_mutex);

    ifile.open(_filename, std::ifstream::binary);

    if(!ifile) {
      return false;
    }
  }
  
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
  // eof reached
  eof = true;
}

void MADDecoder::start() {
  decoder_thread = std::make_unique<std::thread>(&MADDecoder::decode, this);

  // wait for parameters to be valid
  {
    std::unique_lock<std::mutex> mlock(parameters_mutex);
    parameters_updated_cv.wait(mlock);
  }
}

void MADDecoder::rewind() {
  // rewing to start of file
  ifile.clear();
  ifile.seekg(0, std::ifstream::beg);
}

void MADDecoder::set_auto_rewind(bool b) {
  auto_rewind = b;
}

void MADDecoder::join() {
  if(decoder_thread)
    decoder_thread->join();
}

void MADDecoder::wait_for_space_available(void) {
  // acquire frames mutex
  std::unique_lock<std::mutex> mlock(frames_mutex);

  while(frames.size() >= max_frames) {
    // wait for notification of changes in queue
    frames_consumed_cv.wait(mlock);
    // check quit
    if(quit)
      return;
  }
}

std::vector<audio_frame_t> MADDecoder::pop_frames(unsigned int n) {
  std::vector<audio_frame_t> out;

  {
    std::unique_lock<std::mutex> mlock(frames_mutex);
    bool poped=false;
    while(n--) {

      while(frames.empty()) {
        if(eof)
          return out;
        // notify frames consumed
        if(poped) {
          frames_consumed_cv.notify_all();
          poped = false;
        }
        // not enough frames in queue, sleep on it
        frames_available_cv.wait(mlock);
      }

      out.push_back(frames.front());
      frames.pop();
      poped = true;
    }
    // some frames consumed
    frames_consumed_cv.notify_all();
  }

  return out;
}

void MADDecoder::exit() {
  quit = true;
}
