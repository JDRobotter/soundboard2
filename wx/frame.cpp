
#include <wx/wxprec.h>
#ifndef WX_PRECOMP
# include <wx/wx.h>
#endif

#include "frame.hpp"

wxBEGIN_EVENT_TABLE(SoundboardFrame, wxFrame)

wxEND_EVENT_TABLE()

SoundboardFrame::SoundboardFrame(const wxString& title, const wxPoint& pos, const wxSize& size)
  :wxFrame(NULL, wxID_ANY, title, pos, size) {

  
  // main menu
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

  auto menubar = new wxMenuBar;
  menubar->Append(menu, "&File");

  SetMenuBar(menubar);

}
