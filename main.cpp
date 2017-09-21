#include <wx/wxprec.h>
#ifndef WX_PRECOMP
# include <wx/wx.h>
#endif

#include <wx/stdpaths.h>
#include <wx/filename.h>

#include <memory>
#include "frame.hpp"

class SoundboardApp : public wxApp {

public:
	virtual bool OnInit();

};

wxIMPLEMENT_APP(SoundboardApp);

bool SoundboardApp::OnInit() {

#ifdef CONSOLE
	AllocConsole();
	freopen("CONOUT$", "w", stdout);
	freopen("CONOUT$", "w", stderr);
#endif

  // get application name
  wxFileName f(wxStandardPaths::Get().GetExecutablePath());
  wxString appPath(f.GetName());

	auto frame = new SoundboardFrame(appPath, wxPoint(50, 50), wxSize(450, 450));
	frame->Show(true);

	return true;
}
