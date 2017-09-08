
#include <wx/wxprec.h>
#ifndef WX_PRECOMP
# include <wx/wx.h>
#endif
#include <wx/sizer.h>
#include <wx/gbsizer.h>
#include <wx/tglbtn.h>
#include <wx/fileconf.h>

#include "audiomixer.hpp"

class SoundboardVUMeter: public wxPanel {
  
  public:
    SoundboardVUMeter(wxWindow *parent);
    ~SoundboardVUMeter();

    void paint_event(wxPaintEvent& event);
    void paint_now(void);

    void render(wxDC& dc);

		void set_level(float);

  private:
		
		float level;

  wxDECLARE_EVENT_TABLE();
};

class SoundboardMainPanel;

class SoundboardPlayerPanel: public wxPanel {
  public:

    static const int WIDTH = 150;
    static const int HEIGHT = 150;
    SoundboardPlayerPanel(SoundboardMainPanel *parent, int x, int y);
    ~SoundboardPlayerPanel();

    std::shared_ptr<AudioPlayer> get_player(void);

  private:

    std::shared_ptr<AudioMixer> mixer;
		
    int xpos,ypos;

    void open_file_in_player(std::string filename);

    std::string configuration_own_keyify(std::string key);

    void configuration_set_int(std::string key, int);
    int configuration_get_int(std::string key, int vdefault);

    void configuration_set_float(std::string key, float);
    float configuration_get_float(std::string key, float vdefault);

    void configuration_set_string(std::string key, std::string);
    std::string configuration_get_string(std::string key, std::string vdefault);

    void on_button_play(wxCommandEvent& event);
    void on_button_loop(wxCommandEvent& event);
    void on_button_mute(wxCommandEvent& event);
    void on_button_open(wxCommandEvent& event);

		void on_timer(wxTimerEvent& event);

		void on_slider(wxCommandEvent& event);

    void on_destroy(wxWindowDestroyEvent& event);

    SoundboardMainPanel *main_panel;
    AudioPlayerID pid;

		SoundboardVUMeter *vumeter;

		wxSlider *slider_volume;

    wxToggleButton *play_button;
    wxToggleButton *loop_button;
    wxToggleButton *mute_button;
    wxButton *open_button;

		wxTimer *timer;

    wxDECLARE_EVENT_TABLE();
};

class SoundboardFrame;

class SoundboardMainPanel: public wxPanel {

  public:

    SoundboardMainPanel(SoundboardFrame *parent, std::string app_name);
 
    std::shared_ptr<AudioMixer> mixer;

    void configuration_set_int(std::string key, int);
    int configuration_get_int(std::string key, int vdefault);

    void configuration_set_float(std::string key, float);
    float configuration_get_float(std::string key, float vdefault);

    void configuration_set_string(std::string key, std::string);
    std::string configuration_get_string(std::string key, std::string vdefault);

    void increment_player_grid_size(int,int);

  private:

    wxGridBagSizer *gs;
 
    std::unique_ptr<wxFileConfig> config;

    bool load_configuration_from_file(std::string app_name);

    void create_new_player_panel_at_position(int i, int j);

    void remove_player_panel_at_position(int i, int j);

    void set_player_grid_size(int,int);

    wxDECLARE_EVENT_TABLE();
};

class SoundboardFrame: public wxFrame {

  public:
    SoundboardFrame(const wxString& title, const wxPoint& pos, const wxSize& size);

    std::shared_ptr<AudioMixer> get_mixer();

  private:

    wxMenu *menu;
    wxMenu *menu_device;

    wxGridBagSizer *ugs;

    void on_menu(wxCommandEvent& event);

    void on_size(wxSizeEvent& event);

    void set_mixer_device(PaDeviceIndex);

    void on_button_new_column(wxCommandEvent& event);
    void on_button_remove_column(wxCommandEvent& event);

    void on_button_new_row(wxCommandEvent& event);
    void on_button_remove_row(wxCommandEvent& event);

    void set_sizer_and_fit(void);

    std::shared_ptr<AudioMixer> mixer;
 
    wxMenuBar *menubar;
    SoundboardMainPanel *panel;
  
    wxDECLARE_EVENT_TABLE();
};


