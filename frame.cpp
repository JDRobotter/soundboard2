
#include <wx/wxprec.h>
#ifndef WX_PRECOMP
# include <wx/wx.h>
#endif

#include <iostream>
#include <cmath>
#include <wx/filedlg.h>
#include <wx/stdpaths.h>
#include <wx/filename.h>

#include "frame.hpp"

enum {
  FRAME_BUTTON_NEW_COLUMN = 0,
  FRAME_BUTTON_NEW_ROW,
  FRAME_BUTTON_REMOVE_COLUMN,
  FRAME_BUTTON_REMOVE_ROW,
};

wxBEGIN_EVENT_TABLE(SoundboardFrame, wxFrame)
  EVT_SIZE(SoundboardFrame::on_size)
  EVT_BUTTON(FRAME_BUTTON_NEW_COLUMN, SoundboardFrame::on_button_new_column)
  EVT_BUTTON(FRAME_BUTTON_NEW_ROW, SoundboardFrame::on_button_new_row)
  EVT_BUTTON(FRAME_BUTTON_REMOVE_COLUMN, SoundboardFrame::on_button_remove_column)
  EVT_BUTTON(FRAME_BUTTON_REMOVE_ROW, SoundboardFrame::on_button_remove_row)
wxEND_EVENT_TABLE()

SoundboardFrame::SoundboardFrame(const wxString& title, const wxPoint& pos, const wxSize& size)
  :wxFrame(NULL, wxID_ANY, title, pos, size),
  mixer(std::make_shared<AudioMixer>()),
  panel(NULL) {

  // setup menubar
  menu = new wxMenu();

  // build output device menu
  menu_device = new wxMenu();

  for(auto device : mixer->get_devices()) {
    PaDeviceIndex idx = device.first;
    std::string& name = device.second;
    
    menu_device->AppendRadioItem(idx,wxString(name));
    Bind(wxEVT_COMMAND_MENU_SELECTED, &SoundboardFrame::on_device_menu, this, idx);
  }
  menu->AppendSubMenu(menu_device, "&Output device", "Select output device");

  // build output mode menu
  menu_mode = new wxMenu();

  for(auto mode : mixer->get_modes()) {
    auto idx = mode.first;
    std::string& name = mode.second;

    menu_mode->AppendRadioItem(idx, wxString(name));
    Bind(wxEVT_COMMAND_MENU_SELECTED, &SoundboardFrame::on_mode_menu, this, idx);
  }
  menu->AppendSubMenu(menu_mode, "&Output mode", "Select output mode");

  menu->AppendSeparator();
  menu->Append(wxID_EXIT);

  menubar = new wxMenuBar();
  menubar->Append(menu, "&File");

  SetMenuBar(menubar);

  // setup rest of layout
  ugs = new wxGridBagSizer(0,0);
  ugs->SetRows(2);
  ugs->SetCols(2);

  panel = new SoundboardMainPanel(this, title.ToStdString());
  ugs->Add(panel, wxGBPosition(0,0));

  auto vbox = new wxBoxSizer(wxHORIZONTAL);
  ugs->Add(vbox, wxGBPosition(1,0),wxDefaultSpan,wxEXPAND);

  vbox->Add(new wxButton(this, FRAME_BUTTON_NEW_COLUMN, "+", wxDefaultPosition, wxSize(-1,25)),
    1, wxEXPAND);
  vbox->Add(new wxButton(this, FRAME_BUTTON_REMOVE_COLUMN, "-", wxDefaultPosition, wxSize(-1,25)),
    1, wxEXPAND);

  auto hbox = new wxBoxSizer(wxVERTICAL);
  ugs->Add(hbox, wxGBPosition(0,1),wxDefaultSpan,wxEXPAND);

  hbox->Add(new wxButton(this, FRAME_BUTTON_NEW_ROW, "+", wxDefaultPosition, wxSize(25,-1)),
    1, wxEXPAND);
  hbox->Add(new wxButton(this, FRAME_BUTTON_REMOVE_ROW, "-", wxDefaultPosition, wxSize(25,-1)),
    1, wxEXPAND);

  set_sizer_and_fit();

  // load device
  auto idx = mixer->get_default_device();
  auto devname = panel->configuration_get_string("device",std::string());
  if(!devname.empty()) {
    auto _idx = mixer->get_device_by_name(devname);
    if(_idx > paNoDevice)
      idx = _idx;
  }
  set_mixer_device(idx);

}
void SoundboardFrame::set_sizer_and_fit() {
  SetMinSize(wxDefaultSize);
  SetMaxSize(wxDefaultSize);
  SetSizerAndFit(ugs);
  SetMinSize(GetSize());
  SetMaxSize(GetSize());
}

void SoundboardFrame::on_size(wxSizeEvent& event) {
  // continue event progression
  event.Skip();
}

void SoundboardFrame::on_button_remove_column(wxCommandEvent& event) {
  panel->increment_player_grid_size(0,-1);
  set_sizer_and_fit();
}

void SoundboardFrame::on_button_remove_row(wxCommandEvent& event) {
  panel->increment_player_grid_size(-1,0);
  set_sizer_and_fit();
}

void SoundboardFrame::on_button_new_column(wxCommandEvent& event) {
  panel->increment_player_grid_size(0,+1);
  set_sizer_and_fit();
}

void SoundboardFrame::on_button_new_row(wxCommandEvent& event) {
  panel->increment_player_grid_size(+1,0);
  set_sizer_and_fit();
}

void SoundboardFrame::on_device_menu(wxCommandEvent& event) {
  // get selected device index
  PaDeviceIndex idx  = event.GetId();
  set_mixer_device(idx);
  panel->configuration_set_string("device",mixer->get_device_name(idx));
}

void SoundboardFrame::set_mixer_device(PaDeviceIndex idx) {
  mixer->set_device(idx);
  // set menu items checks
  menu_device->Check(idx,true);
}

void SoundboardFrame::on_mode_menu(wxCommandEvent& event) {
  // get selected mode idx
  AudioMixerMode idx = (AudioMixerMode)event.GetId();
  set_mixer_mode(idx);
  panel->configuration_set_int("mode",idx);
}

void SoundboardFrame::set_mixer_mode(AudioMixerMode idx) {
  mixer->set_mode(idx);
  // set menu items checks
  menu_mode->Check(idx,true);
}

std::shared_ptr<AudioMixer> SoundboardFrame::get_mixer() {
  return mixer;
}

wxBEGIN_EVENT_TABLE(SoundboardMainPanel, wxPanel)
wxEND_EVENT_TABLE()

SoundboardMainPanel::SoundboardMainPanel(SoundboardFrame *parent, std::string app_name)
  :wxPanel(parent) {

  // load configuration from disk
  load_configuration_from_file(app_name);

  // create audio mixer
  mixer = parent->get_mixer();

  gs = new wxGridBagSizer(0,0);

  auto ncols = configuration_get_int("grid-ncols", 1);
  auto nrows = configuration_get_int("grid-nrows", 1);
  set_player_grid_size(ncols, nrows);
  SetSizerAndFit(gs);
}

bool SoundboardMainPanel::load_configuration_from_file(std::string app_name) {
  auto local = wxStandardPaths::Get().GetUserLocalDataDir();

  std::hash<std::string> hash_fn;
  size_t hash = hash_fn(app_name);

  // check if directory exist, if not create it
  if(!wxDirExists(local)) {
    if(!wxFileName::Mkdir(local,wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL))
      return false;
  }
  
  auto filename = wxFileName(local, std::to_string(hash), "conf").GetFullPath();

  config = std::make_unique<wxFileConfig>(wxT(""), wxT(""), filename);

  return true;
}

void SoundboardMainPanel::configuration_set_int(std::string key, int v) {
  if(!config)
    return;

  config->Write(key,v);
  config->Flush();
}
  
int SoundboardMainPanel::configuration_get_int(std::string key, int vdefault ) {
  if(!config)
    return vdefault;

  int rv=0;
  if(config->Read(key, &rv))
    return rv;
  else
    return vdefault;
}

void SoundboardMainPanel::configuration_set_float(std::string key, float v) {
  if(!config)
    return;

  config->Write(key,v);
  config->Flush();
}

float SoundboardMainPanel::configuration_get_float(std::string key, float vdefault) {
  if(!config)
    return vdefault;

  float rv;
  if(config->Read(key, &rv))
    return rv;
  else
    return vdefault;
}

void SoundboardMainPanel::configuration_set_string(std::string key, std::string v) {
  if(!config)
    return;

  config->Write(key, wxString(v));
  config->Flush();
}

std::string SoundboardMainPanel::configuration_get_string(std::string key, std::string vdefault) {
  if(!config)
    return vdefault;

  wxString rv;
  if(config->Read(key, &rv))
    return rv.ToStdString();
  else
    return vdefault;
}

void SoundboardMainPanel::increment_player_grid_size(int ncols, int nrows) {
  auto cw = gs->GetCols(), ch = gs->GetRows();
  set_player_grid_size(cw+ncols, ch+nrows);
  SetSizerAndFit(gs);
}

void SoundboardMainPanel::set_player_grid_size(int ncols, int nrows) {
  auto nw = ncols;
  auto nh = nrows;

  if(nw == 0 || nh == 0)
    return;   

  // save new size
  configuration_set_int("grid-ncols", ncols);
  configuration_set_int("grid-nrows", nrows);

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
    gs->Add(new SoundboardPlayerPanel(this,i,j),
      wxGBPosition(i,j),
      wxDefaultSpan,
      wxEXPAND);
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

SoundboardPlayerPanel::SoundboardPlayerPanel(SoundboardMainPanel *parent,
  int x, int y)
  :wxPanel(parent),
  xpos(x), ypos(y) {

  // assign mixer shared ptr
  main_panel = parent;
  // keep a ref to audio mixer
  mixer = main_panel->mixer;
  // create a new player
  pid = mixer->new_player();

  // force min/max size 
  auto sz = wxSize(SoundboardPlayerPanel::WIDTH, SoundboardPlayerPanel::HEIGHT);
  SetSizeHints(sz,sz);

  // debug
  //SetBackgroundColour(*wxGREEN);

  auto vbox = new wxBoxSizer(wxVERTICAL);
  SetSizer(vbox);

  play_button = new wxToggleButton(this, PLAYER_BUTTON_PLAY, wxT("-"));

  vbox->Add(play_button, 6, wxEXPAND);

  vumeter = new SoundboardVUMeter(this);
  vbox->Add(vumeter, 1, wxEXPAND|wxALL, 2);

  float gain = configuration_get_float("gain", 1.0);
  int vol = 100*gain;
  slider_volume = new wxSlider(this, PLAYER_SLIDER_VOLUME, vol, 0, 125);
  get_player()->set_gain(gain);
  vbox->Add(slider_volume, 1, wxEXPAND);

  auto hbox = new wxBoxSizer(wxHORIZONTAL);
  vbox->Add(hbox, 2, wxEXPAND);
  
  loop_button = new wxToggleButton(this, PLAYER_BUTTON_LOOP, wxT("L"),
                                    wxDefaultPosition, wxSize(10,-1));
  loop_button->SetLabelMarkup("<b>L</b>");
  loop_button->SetForegroundColour(wxColour(66,119,244,255));
  hbox->Add(loop_button, 1, wxEXPAND);
  bool loop = configuration_get_int("loop", false);
  loop_button->SetValue(loop);
  get_player()->set_repeat(loop);

  mute_button = new wxToggleButton(this, PLAYER_BUTTON_MUTE, wxT("M"),
                                    wxDefaultPosition, wxSize(10,-1));
  mute_button->SetLabelMarkup("<b>M</b>");
  mute_button->SetForegroundColour(wxColour(244,80,66,255));
  hbox->Add(mute_button, 1, wxEXPAND);
  bool mute = configuration_get_int("mute", false);
  mute_button->SetValue(mute);
  get_player()->set_mute(mute);

  open_button = new wxButton(this, PLAYER_BUTTON_OPEN, wxT("O"),
                                    wxDefaultPosition, wxSize(10,-1));
  hbox->Add(open_button, 1, wxEXPAND);
  auto path = configuration_get_string("path", "");
  if(!path.empty()) {
    open_file_in_player(path);
  }


  timer = new wxTimer(this, PLAYER_TIMER);
  timer->Start(100);
}

SoundboardPlayerPanel::~SoundboardPlayerPanel() {
  // stop timer events
  timer->Stop();
  // remove player from mixer
  mixer->remove_player(pid);
}

void SoundboardPlayerPanel::open_file_in_player(std::string filename) {
  get_player()->open(filename);
  play_button->SetLabelMarkup(wxFileName(filename).GetName());
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
  bool b = event.IsChecked();
  p->set_repeat(b);
  configuration_set_int("loop",b);
}

void SoundboardPlayerPanel::on_button_mute(wxCommandEvent& event) {
  auto p = get_player();
  bool b = event.IsChecked();
  p->set_mute(b);
  configuration_set_int("mute",b);
}

void SoundboardPlayerPanel::on_button_open(wxCommandEvent& event) {
  wxFileDialog dialog(this, wxT("Open sample"), "", "",
    "mp3 files (*.mp3)|*.mp3|wav files (*.wav)|*.wav");

  // recall last opened directory
  auto previous = main_panel->configuration_get_string("last-directory", std::string());
  if(!previous.empty()) {
    dialog.SetDirectory(previous);
  }

  auto rv = dialog.ShowModal();

  // store directory to configuration (even if cancel was pressed)
  auto directory = dialog.GetDirectory().ToStdString();
  main_panel->configuration_set_string("last-directory", directory);

  if(rv == wxID_CANCEL)
    return;

  auto path = dialog.GetPath().ToStdString();
  open_file_in_player(path);
  configuration_set_string("path",path);
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
  configuration_set_float("gain",gain);
}

std::string SoundboardPlayerPanel::configuration_own_keyify(std::string key) {
  return "player#"+std::to_string(xpos) + "#" + std::to_string(ypos) + "#" + key;
}

void SoundboardPlayerPanel::configuration_set_int(std::string key, int v) {
  main_panel->configuration_set_int(configuration_own_keyify(key), v);
}
  
int SoundboardPlayerPanel::configuration_get_int(std::string key, int vdefault ) {
  return main_panel->configuration_get_int(configuration_own_keyify(key), vdefault);
}

void SoundboardPlayerPanel::configuration_set_float(std::string key, float v) {
  main_panel->configuration_set_float(configuration_own_keyify(key), v);
}

float SoundboardPlayerPanel::configuration_get_float(std::string key, float vdefault) {
  return main_panel->configuration_get_float(configuration_own_keyify(key), vdefault);
}

void SoundboardPlayerPanel::configuration_set_string(std::string key, std::string v) {
  main_panel->configuration_set_string(configuration_own_keyify(key), v);
}

std::string SoundboardPlayerPanel::configuration_get_string(std::string key, std::string vdefault) {
  return main_panel->configuration_get_string(configuration_own_keyify(key), vdefault);
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

  // plot sound level on a logarithm scale
  v = std::max(0.0f, std::min(1.0f, v));
  float new_level = 1.0 + 0.5*std::log10(v);

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
