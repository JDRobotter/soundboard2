#include "audiomixer.hpp"
#include <iostream>
#include <algorithm>
#include <thread>
#include <cmath>


AudioPlayer::AudioPlayer(AudioMixer *mixer)
  :mixer(mixer) {
  gain = 1.0;
  level = 0.0;
  repeat = false;
  mute = false;
}

AudioPlayer::~AudioPlayer() {

}

bool AudioPlayer::close(void) {

  if(is_stream_valid()) {
    auto err = Pa_CloseStream(stream);
    if(err != paNoError) {
      std::cerr<<"ErrorA"<<Pa_GetErrorText(err)<<"\n";
      return false;
    }
    // invalidate
    filename = std::string();
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
      std::cerr<<"ErrorB"<<Pa_GetErrorText(err)<<"\n";
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
      std::cerr<<"ErrorC"<<Pa_GetErrorText(err)<<"\n";
      return false;
    }
    set_level(0.0);
    return true;
  }

  return false;
}

bool AudioPlayer::reset(void) {
  // stop stream
  stop();

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
  if(is_stream_valid()) {
    decoder->set_auto_rewind(repeat);
  }
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
    filename = std::string();
    return false;
  }
  // start decoding
  decoder->start();
 
  // get decoded audio parameters
  auto p = decoder->get_parameters();
  // set up PA parameters
  PaDeviceIndex idx = mixer->get_device();
  std::cerr<<"open stream "<<idx<<"\n";
  PaStreamParameters op;
  op.device = idx;
  op.channelCount = 2;
  op.sampleFormat = paFloat32;
  op.suggestedLatency = 0.1;
  op.hostApiSpecificStreamInfo = NULL;
  // start PA stream
  auto err = Pa_OpenStream(
    &stream,
    NULL, /*inputParameters*/
    &op,
    p.samplerate_hz,
    paFramesPerBufferUnspecified,
    0, /*flags*/
    AudioPlayer::portaudio_feed_callback,
    (void*)this);

  if(err != paNoError) {
    std::cerr<<"ErrorE"<<Pa_GetErrorText(err)<<"\n";
    return false;
  }

  return true;
}

int AudioPlayer::portaudio_feed_callback(
			const void *input_buffer,
			void *output_buffer,
			unsigned long frames_per_buffer,
			const PaStreamCallbackTimeInfo *time_info,
			PaStreamCallbackFlags status_flags,
			void *data) {
  (void)status_flags;
  (void)time_info;
  (void)input_buffer;

  AudioPlayer *player = (AudioPlayer*)data;


  //std::cerr << "FEED " << frames_per_buffer << "\n";
  // get frame from decoder (will potentially block)
  auto frames = player->decoder->pop_frames(frames_per_buffer);
  //std::cerr << "> " << frames.size() << "\n";
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

  int i = 0;
  for(auto frame: frames) {
    i++;
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
  return !filename.empty();
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
    std::cerr<<"ErrorF"<<Pa_GetErrorText(err)<<"\n";


  // iterate over devices
  int ndevices = Pa_GetDeviceCount();
  if(ndevices < 0)
    std::cerr<<"ErrorG"<<Pa_GetErrorText(ndevices)<<"\n";
  for(PaDeviceIndex i=0;i<ndevices;i++) {
    auto devinfo = Pa_GetDeviceInfo(i);
    
    // device must be at least stereo to work
    if(devinfo->maxOutputChannels < 2)
      continue;
    
    std::string name = std::string(devinfo->name);
    devices.push_back({i,name});
  }

  set_default_device();
}

AudioMixer::~AudioMixer() {

  auto err = Pa_Terminate();
  if(err != paNoError)
    std::cerr<<"ErrorH"<<Pa_GetErrorText(err)<<"\n";
}

std::vector<std::pair<PaDeviceIndex,std::string>> AudioMixer::get_devices() {
  return devices;
}

AudioPlayerID AudioMixer::new_player() {
  next_audio_player_id++;
  players[next_audio_player_id] = std::make_shared<AudioPlayer>(this);
  return next_audio_player_id;
}

std::shared_ptr<AudioPlayer> AudioMixer::get_player(AudioPlayerID uid) {
  return players.find(uid)->second;
}

void AudioMixer::remove_player(AudioPlayerID id) {
  get_player(id)->close();
  players.erase(id);
}

std::string AudioMixer::get_device_name(PaDeviceIndex idx) {
  for(auto const& device: devices) {
    if(device.first == idx)
      return device.second;
  }
  return std::string();
}

PaDeviceIndex AudioMixer::get_device_by_name(std::string name) {
  for(auto const& device: devices) {
    if(device.second == name) {
      return device.first;
    }
  }
  return paNoDevice;
}

PaDeviceIndex AudioMixer::get_device(void) {
  std::cerr<<"get_device"<<current_device<<"\n";
  return current_device;
}

void AudioMixer::set_device(PaDeviceIndex idx) {
  current_device = idx;
  std::cerr<<"set_device"<<idx<<"\n";
  // iterate over all players to stop them
  for(auto const&  item: players) {
    auto const& player = item.second;
    player->stop();
  }
}

void AudioMixer::set_default_device() {
  set_device(Pa_GetDefaultOutputDevice());
}
