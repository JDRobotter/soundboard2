
#include <wx/wxprec.h>
#ifndef WX_PRECOMP
# include <wx/wx.h>
#endif
#include <wx/sizer.h>
#include <wx/gbsizer.h>
#include <wx/tglbtn.h>

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

class SoundboardPlayerPanel: public wxPanel {
  public:

    static const int WIDTH = 200;
    static const int HEIGHT = 200;
    SoundboardPlayerPanel(wxWindow *parent);
    ~SoundboardPlayerPanel();

    std::shared_ptr<AudioPlayer> get_player(void);

  private:
    void on_button_play(wxCommandEvent& event);
    void on_button_loop(wxCommandEvent& event);
    void on_button_mute(wxCommandEvent& event);
    void on_button_open(wxCommandEvent& event);

		void on_timer(wxTimerEvent& event);

		void on_slider(wxCommandEvent& event);

    void on_destroy(wxWindowDestroyEvent& event);

    std::shared_ptr<AudioMixer> mixer;
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

class SoundboardMainPanel: public wxPanel {

  public:

    SoundboardMainPanel(wxWindow *parent);    
 
    std::shared_ptr<AudioMixer> mixer;

  private:

    wxGridBagSizer *gs;
    
    void create_new_player_panel_at_position(int i, int j);

    void remove_player_panel_at_position(int i, int j);

    void on_size(wxSizeEvent&);

    wxDECLARE_EVENT_TABLE();
};

class SoundboardFrame: public wxFrame {

  public:
    SoundboardFrame(const wxString& title, const wxPoint& pos, const wxSize& size);

  private:
 
    wxMenuBar *menubar;
    SoundboardMainPanel *panel;
  
    wxDECLARE_EVENT_TABLE();
};


