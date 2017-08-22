
#include <wx/wxprec.h>
#ifndef WX_PRECOMP
# include <wx/wx.h>
#endif

class SoundboardFrame: public wxFrame {

  public:
    SoundboardFrame(const wxString& title, const wxPoint& pos, const wxSize& size);

  private:
    
    wxDECLARE_EVENT_TABLE();
};
