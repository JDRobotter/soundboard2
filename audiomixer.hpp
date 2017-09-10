#ifndef _AUDIOMIXER_HPP
#define _AUDIOMIXER_HPP

#include <memory>
#include <map>
#include <portaudio.h>
#include <atomic>

#include "maddecoder.hpp"
#include "wavdecoder.hpp"

class AudioMixer;

enum AudioMixerMode {
  MIXER_MODE_STEREO = 1,
  MIXER_MODE_FULL_RIGHT,
  MIXER_MODE_FULL_LEFT,
};

class AudioPlayer {
  public:
    AudioPlayer(AudioMixer *mixer);
    ~AudioPlayer();

    bool open(std::string filename);

		static int portaudio_feed_callback(
			const void *input_buffer,
			void *output_buffer,
			unsigned long frames_per_buffer,
			const PaStreamCallbackTimeInfo *time_info,
			PaStreamCallbackFlags status_flags,
			void *data);

		bool play(void);

    bool is_playing(void);

		bool stop(void);

		bool close(void);

		bool reset(void);

		float set_gain(float);
		float get_gain(void);
		
    void set_mute(bool);
    bool get_mute();

		void set_repeat(bool);

    void set_level(float);
    float get_level(void);

    std::string get_filename(void) { return filename;};

	private:
	  AudioMixer* mixer;

    // return true if stream as been created, false otherwise
    bool is_stream_valid(void);

    // return current audio mixer mode
    AudioMixerMode get_mixer_mode(void);

		PaStream *stream;
		
		std::unique_ptr<Decoder> decoder;

		std::string filename;

		std::atomic<float> gain;

		std::atomic<bool> repeat;

    std::atomic<bool> mute;

		std::atomic<float> level;
};


using AudioMixerModePair = std::pair<AudioMixerMode,std::string>;
const std::vector<AudioMixerModePair> MIXER_MODES {
  {MIXER_MODE_STEREO, "stereo"},
  {MIXER_MODE_FULL_RIGHT, "all to right"},
  {MIXER_MODE_FULL_LEFT, "all to left"},
};

using AudioPlayerID = int;
using AudioPlayerMap = std::map<AudioPlayerID, std::shared_ptr<AudioPlayer>>;

class AudioMixer {

  public:
    AudioMixer();
    ~AudioMixer();

    AudioPlayerID new_player(void);

    std::shared_ptr<AudioPlayer> get_player(AudioPlayerID);
    
    void remove_player(AudioPlayerID);

    std::vector<std::pair<PaDeviceIndex,std::string>> get_devices();

    std::string get_device_name(PaDeviceIndex idx);

    PaDeviceIndex get_device_by_name(std::string name);

    void set_device(PaDeviceIndex idx);
    PaDeviceIndex get_default_device(void);

    PaDeviceIndex get_device(void);

    void set_mode(AudioMixerMode);
    std::vector<AudioMixerModePair> get_modes(void);
    AudioMixerMode get_mode(void);

  private:

    // currently selected device
    std::atomic<PaDeviceIndex> current_device;
    // currently selected mode
    std::atomic<AudioMixerMode> current_mode;
    // map of audio players
    AudioPlayerMap players;

    // list of available devices 
    std::vector<std::pair<PaDeviceIndex,std::string>> devices;

    // next audio player ID
    AudioPlayerID next_audio_player_id;
};

#endif
