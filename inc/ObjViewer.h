#pragma once

// For compilers that support precompilation, includes "wx/wx.h".
#include <wx/wxprec.h>

#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif

#if !wxUSE_GLCANVAS
#error "OpenGL required: set wxUSE_GLCANVAS to 1 and rebuild the library"
#endif

#define wxUSE_GUI 1

#include <fstream>
#include <string>
#include <vector>
#include <opencv2/opencv.hpp>
#include <wx/tglbtn.h>
#include "OVCanvas.h"


namespace ov
{


enum RENDER_MODE
{
    RENDER_SOLID,
    RENDER_WIREFRAME,
};

enum WINDOW_ID
{
    ID_ANY = -1,
    ID_MENU_OPEN_MODEL,
    ID_MENU_OPEN_BACKGROUND_IMAGE,
    ID_MENU_SAVE_IMAGE,
    ID_MENU_GEN_SEQ,
    ID_MENU_EXIT,
    ID_MENU_HELP,
    ID_CANVAS,
    ID_RENDER_MODE_RADIO,
    ID_RESET,
    ID_LIGHTING,
};


class OVCanvas;

class ObjViewer : public wxFrame
{
  public:
    ObjViewer(const wxString& title);
    ~ObjViewer();

  protected:
    void onMenuFileOpenObjModel(wxCommandEvent& evt);
    void onMenuFileOpenBackgroundImage(wxCommandEvent& evt);
    void onMenuFileSaveImage(wxCommandEvent& evt);
    void onMenuGenerateSequence(wxCommandEvent& evt);
    void onMenuFileExit(wxCommandEvent& evt);
    void onMenuHelpAbout(wxCommandEvent& evt);
    void onRenderModeRadio(wxCommandEvent& evt);
    void onLightingCheck(wxCommandEvent& evt);
    void onReset(wxCommandEvent& evt);
    void onMouse(wxMouseEvent& evt);

  private:  
    void reLayout();

    wxBoxSizer*           _mainSizer;

    // ================== Canvas part ==================
    OVCanvas*             _ovCanvas;

    // ================== Controller part ==================
    wxBoxSizer*           _controllerSizer;
    wxRadioBox*           _renderModeRadioBox;
    wxButton*             _resetButton;
    wxCheckBox*           _lightingCheckBox;

    // Some options
    int  _renderMode;
    bool _lightingOn;
    
    // Data path
    std::string _dataFolder;
    std::string _objModelFile;
    std::string _imageFile;
};

} // namespace ov
