#include "audiomixer.hpp"
#include <iostream>
#include <algorithm>
#include <thread>
#include <cmath>

AudioPlayer::AudioPlayer() {
  gain = 1.0;
  repeat = false;
  mute = false;
}

AudioPlayer::~AudioPlayer() {

}

bool AudioPlayer::close(void) {

  if(is_stream_valid()) {
    auto err = Pa_CloseStream(stream);
    if(err != paNoError) {
      std::cerr<<"ErrorX"<<Pa_GetErrorText(err)<<"\n";
      return false;
    }
    return true;
  }

  return false;
}

bool AudioPlayer::play(void) {
  // check if stream is running
  if(is_playing()) {
    return true;
  }
  if(is_stream_valid()) {
    auto err = Pa_StartStream(stream);
    if(err != paNoError) {
      std::cerr<<"Error"<<Pa_GetErrorText(err)<<"\n";
      return false;
    }
    return true;
  }

  return false;
}

bool AudioPlayer::is_playing(void) {
  if(is_stream_valid())
    return Pa_IsStreamActive(stream);
  else
    return false;
}

bool AudioPlayer::stop(void) {
  // check if stream is running
  if(!is_playing()) {
    return true;
  }
  if(is_stream_valid()) {
    auto err = Pa_StopStream(stream);
    if(err != paNoError) {
      std::cerr<<"Error"<<Pa_GetErrorText(err)<<"\n";
      return false;
    }
    return true;
  }

  return false;
}

bool AudioPlayer::reset(void) {
  // stop stream
  stop();

  if(is_stream_valid()) {
    auto err = Pa_CloseStream(stream);
    if(err != paNoError) {
      std::cerr<<"Error"<<Pa_GetErrorText(err)<<"\n";
      return false;
    }
  }

  // kill decoder and build a new one
  open(filename);

  return true;
}

float AudioPlayer::set_gain(float _gain) {
  gain = std::min(2.0f,std::max(0.0f,_gain));
  return gain;
}

float AudioPlayer::get_gain(void) {
  return gain;
}

void AudioPlayer::set_repeat(bool b) {
  repeat = b;
  decoder->set_auto_rewind(repeat);
}

void AudioPlayer::set_mute(bool b) {
  mute = b;
}

bool AudioPlayer::get_mute() {
  return mute;
}

bool AudioPlayer::open(std::string _filename) {
  // close current stream if any
  if(is_stream_valid()) {
    close();
  }

  // store filename
  filename = _filename;

  // extract extension from filename
  auto ext = filename.substr( filename.find_last_of(".") +  1);

  // fire up decoder (kill existing if any)
  if(ext == "mp3")
    decoder = std::make_unique<MADDecoder>();
  else if(ext == "wav")
    decoder = std::make_unique<WAVDecoder>();
  else
    return false;

  decoder->set_auto_rewind(repeat);

  // open audio file
  if(!decoder->open(filename)) {
    return false;
  }
  // start decoding
  decoder->start();
 
  // get decoded audio parameters
  auto p = decoder->get_parameters();
  std::cerr<<"decoder: "<<p.samplerate_hz<<","<<p.channels<<"\n";
  // start PA stream
  auto err = Pa_OpenDefaultStream(
    &stream,
    0, 2,
    paFloat32,
    p.samplerate_hz,
    2048,//paFramesPerBufferUnspecified,
    AudioPlayer::portaudio_feed_callback,
    (void*)this);

  if(err != paNoError)
    std::cerr<<"Error"<<Pa_GetErrorText(err)<<"\n";

  return true;
}

int AudioPlayer::portaudio_feed_callback(
			const void *input_buffer,
			void *output_buffer,
			unsigned long frames_per_buffer,
			const PaStreamCallbackTimeInfo *time_info,
			PaStreamCallbackFlags status_flags,
			void *data) {
  
  AudioPlayer *player = (AudioPlayer*)data;

  // get frame from decoder (will potentially block)
  auto frames = player->decoder->pop_frames(frames_per_buffer);
  if(frames.size() == 0) {
    // we reached end of file
    return paComplete;
  }

  // copy to buffer
  float *out = (float*)output_buffer;
  auto gain = player->get_gain();
  if(player->get_mute()) {
    gain = 0.0;
  }

  // measure L/R max signal enveloppe
  float lmax=0.0,rmax=0.0;

  for(auto frame: frames) {
    float l = gain*frame.left;
    float r = gain*frame.right;

    float fl = fabs(l), fr = fabs(r);
    if(fl > lmax)
      lmax = fl;
    if(fr > rmax)
      rmax = fr;

    *(out++) = l;
    *(out++) = r;
  }

  // update mean signal level
  player->set_level((lmax + rmax)/2);

  return paContinue;
}

bool AudioPlayer::is_stream_valid(void) {
  return filename != "";
}

void AudioPlayer::set_level(float v) {
  level = v;
}

float AudioPlayer::get_level(void) {
  return level;
}

//

AudioMixer::AudioMixer() {
  next_audio_player_id = 0;

  auto err = Pa_Initialize();
  if(err != paNoError)
    std::cerr<<"Error"<<Pa_GetErrorText(err)<<"\n";
}

AudioMixer::~AudioMixer() {

  auto err = Pa_Terminate();
  if(err != paNoError)
    std::cerr<<"Error"<<Pa_GetErrorText(err)<<"\n";
}

AudioPlayerID AudioMixer::new_player() {
  next_audio_player_id++;
  players[next_audio_player_id] = std::make_shared<AudioPlayer>();
  return next_audio_player_id;
}

std::shared_ptr<AudioPlayer> AudioMixer::get_player(AudioPlayerID uid) {
  return players.find(uid)->second;
}

void AudioMixer::remove_player(AudioPlayerID id) {
  get_player(id)->close();
  players.erase(id);
}
