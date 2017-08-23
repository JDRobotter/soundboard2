
#include <wx/wxprec.h>
#ifndef WX_PRECOMP
# include <wx/wx.h>
#endif

#include <iostream>
#include <cmath>
#include <wx/filedlg.h>

#include "frame.hpp"

wxBEGIN_EVENT_TABLE(SoundboardFrame, wxFrame)
wxEND_EVENT_TABLE()

SoundboardFrame::SoundboardFrame(const wxString& title, const wxPoint& pos, const wxSize& size)
  :wxFrame(NULL, wxID_ANY, title, pos, size) {
  
  // setup menubar
  auto menu = new wxMenu;

  // build output device menu
  auto menu_device = new wxMenu;

  wxString devices[] = {
    "main audio",
    "secondary audio",
    "default",
    "fup one",
  };
  for(auto device : devices) {
    menu_device->Append(1,device);
  }

  menu->AppendSubMenu(menu_device, "&Output device", "Select output device");

  menu->AppendSeparator();
  menu->Append(wxID_EXIT);

  menubar = new wxMenuBar;
  menubar->Append(menu, "&File");

  SetMenuBar(menubar);

  // setup rest of layout

  panel = new SoundboardMainPanel(this);

  Centre();
}

wxBEGIN_EVENT_TABLE(SoundboardMainPanel, wxPanel)
  EVT_SIZE(SoundboardMainPanel::on_size)
wxEND_EVENT_TABLE()

SoundboardMainPanel::SoundboardMainPanel(wxWindow *parent)
  :wxPanel(parent) {

  // create audio mixer
  mixer = std::make_shared<AudioMixer>();
 
  // debug
  //SetBackgroundColour(*wxRED);

  gs = new wxGridBagSizer(0,0);

  SetSizer(gs);
  Fit();
  
  Centre();
}

void SoundboardMainPanel::on_size(wxSizeEvent& event) {
  // continue event propagation
  event.Skip();

  //SetSizeHints(wxSize(50,50), wxSize(50,50), wxSize(50,50));
  auto sz = GetSize();
  auto w = sz.GetWidth(), h = sz.GetHeight();
  auto pw = SoundboardPlayerPanel::WIDTH, ph = SoundboardPlayerPanel::HEIGHT;

  auto nw = floor(w/pw);
  auto nh = floor(h/ph);
  std::cerr<<"new grid sz= "<<nw<<","<<nh<<"\n";

  if(nw == 0 || nh == 0)
    return;   

  auto cw = gs->GetCols(), ch = gs->GetRows();

  // remove players
  // iterating over all lines and old columns
  for(int i=0;i<nh;i++)
  for(int j=nw;j<cw;j++) {
    remove_player_panel_at_position(i,j);
  }

  // iterating over all columns and new lines
  for(int j=0;j<nw;j++)
  for(int i=nh;i<ch;i++) {
    remove_player_panel_at_position(i,j);
  }

  // resize grid
  gs->SetRows(nh);
  gs->SetCols(nw);

  // iterating over all lines and new columns
  for(int i=0;i<nh;i++)
  for(int j=cw;j<nw;j++) {
    create_new_player_panel_at_position(i,j);
  }
  // iterating over all columns and new lines
  for(int j=0;j<nw;j++)
  for(int i=ch;i<nh;i++) {
    create_new_player_panel_at_position(i,j);
  }

}

void SoundboardMainPanel::create_new_player_panel_at_position(int i, int j) {

  // check if panel exist
  auto p = gs->FindItemAtPosition(wxGBPosition(i,j));
  // if not create it
  if(!p) {
    gs->Add(new SoundboardPlayerPanel(this),
      wxGBPosition(i,j),
      wxDefaultSpan,
      wxEXPAND);
    std::cerr<<"adding\n";
  }
}

void SoundboardMainPanel::remove_player_panel_at_position(int i, int j) {
  // check if panel exist
  auto p = gs->FindItemAtPosition(wxGBPosition(i,j));
  // if its the case remove it
  if(p) {
    auto w = p->GetWindow();
    if(w)
      w->Destroy();
  }
}

enum {
  PLAYER_BUTTON_PLAY,
  PLAYER_BUTTON_LOOP,
  PLAYER_BUTTON_MUTE,
  PLAYER_BUTTON_OPEN,
  PLAYER_TIMER,
  PLAYER_SLIDER_VOLUME,
};

wxBEGIN_EVENT_TABLE(SoundboardPlayerPanel, wxPanel)
  EVT_TOGGLEBUTTON(PLAYER_BUTTON_PLAY, SoundboardPlayerPanel::on_button_play)
  EVT_TOGGLEBUTTON(PLAYER_BUTTON_LOOP, SoundboardPlayerPanel::on_button_loop)
  EVT_TOGGLEBUTTON(PLAYER_BUTTON_MUTE, SoundboardPlayerPanel::on_button_mute)
  EVT_BUTTON(PLAYER_BUTTON_OPEN, SoundboardPlayerPanel::on_button_open)
  EVT_TIMER(PLAYER_TIMER, SoundboardPlayerPanel::on_timer)
  EVT_SLIDER(PLAYER_SLIDER_VOLUME, SoundboardPlayerPanel::on_slider)
wxEND_EVENT_TABLE()


SoundboardPlayerPanel::SoundboardPlayerPanel(wxWindow *parent)
  :wxPanel(parent) {

  std::cerr<<"in with the new\n";

  // assign mixer shared ptr
  auto panel = dynamic_cast<SoundboardMainPanel*>(parent);
  mixer = panel->mixer;
  // create a new player
  pid = mixer->new_player();

  // force min/max size 
  auto sz = wxSize(SoundboardPlayerPanel::WIDTH, SoundboardPlayerPanel::HEIGHT);
  SetSizeHints(sz,sz);

  // debug
  //SetBackgroundColour(*wxGREEN);

  auto vbox = new wxBoxSizer(wxVERTICAL);
  SetSizer(vbox);

  play_button = new wxToggleButton(this, PLAYER_BUTTON_PLAY, wxT("PLAY"));
  vbox->Add(play_button, 6, wxEXPAND);

  vumeter = new SoundboardVUMeter(this);
  vbox->Add(vumeter, 1, wxEXPAND|wxALL, 5);

  slider_volume = new wxSlider(this, PLAYER_SLIDER_VOLUME, 100, 0, 125);
  vbox->Add(slider_volume, 1, wxEXPAND);

  auto hbox = new wxBoxSizer(wxHORIZONTAL);
  vbox->Add(hbox, 2, wxEXPAND);

  loop_button = new wxToggleButton(this, PLAYER_BUTTON_LOOP, wxT("L"));
  hbox->Add(loop_button, 1, wxEXPAND);

  mute_button = new wxToggleButton(this, PLAYER_BUTTON_MUTE, wxT("M"));
  hbox->Add(mute_button, 1, wxEXPAND);

  open_button = new wxButton(this, PLAYER_BUTTON_OPEN, wxT("O"));
  hbox->Add(open_button, 1, wxEXPAND);

  timer = new wxTimer(this, PLAYER_TIMER);
  timer->Start(100);
}

SoundboardPlayerPanel::~SoundboardPlayerPanel() {

  mixer->remove_player(pid);

  std::cerr<<"out with the old\n";
}

std::shared_ptr<AudioPlayer> SoundboardPlayerPanel::get_player() {
  return mixer->get_player(pid);
}

void SoundboardPlayerPanel::on_button_play(wxCommandEvent& event) {
  auto p = get_player();
  // play if stopped / stop if playing
  if(!event.IsChecked()) {
    p->stop();
  }
  else {
    p->reset();
    p->play();
  }

}

void SoundboardPlayerPanel::on_button_loop(wxCommandEvent& event) {
  auto p = get_player();
  p->set_repeat(event.IsChecked());
}

void SoundboardPlayerPanel::on_button_mute(wxCommandEvent& event) {
  auto p = get_player();
  p->set_mute(event.IsChecked());
}

void SoundboardPlayerPanel::on_button_open(wxCommandEvent& event) {
  wxFileDialog dialog(this, wxT("Open sample"), "", "",
    "mp3 files (*.mp3)|*.mp3|wav files (*.wav)|*.wav");
  if(dialog.ShowModal() == wxID_CANCEL)
    return;

  get_player()->open(dialog.GetPath().ToStdString());
}

void SoundboardPlayerPanel::on_timer(wxTimerEvent& event) {
  // if play button is pressed and stream is no longer active
  if(play_button->GetValue() && !get_player()->is_playing()) {
    play_button->SetValue(false);
  }

  // update vu meter
  vumeter->set_level(get_player()->get_level());
}

void SoundboardPlayerPanel::on_slider(wxCommandEvent& event) {
  float gain = (slider_volume->GetValue()/100.0f);
  get_player()->set_gain(gain);
}

wxBEGIN_EVENT_TABLE(SoundboardVUMeter, wxPanel)
  EVT_PAINT(SoundboardVUMeter::paint_event)
wxEND_EVENT_TABLE()

SoundboardVUMeter::SoundboardVUMeter(wxWindow *parent)
  :wxPanel(parent) {

  //SetBackgroundColour(*wxBLUE);
}

SoundboardVUMeter::~SoundboardVUMeter() {
  level = 0.0;
}

void SoundboardVUMeter::set_level(float v) {
  float new_level = std::max(0.0f, std::min(1.0f, v));
  bool as_changed = level != new_level;
  level = new_level;
  if(as_changed)
    Refresh();
}

void SoundboardVUMeter::paint_event(wxPaintEvent& event) {
  wxPaintDC dc(this);
  render(dc);
}

void SoundboardVUMeter::render(wxDC& dc) {
  auto sz = GetSize();
  auto w = sz.GetWidth(), h = sz.GetHeight();
  auto r = wxRect(0,0,w,h);

  dc.SetPen(wxNullPen);
  dc.SetBrush(*wxGREY_BRUSH);
  dc.DrawRectangle(r);

  r = wxRect(0,0,w*level,h);
  dc.GradientFillLinear(r, *wxGREEN, wxColour(level*255,255,0));
}
