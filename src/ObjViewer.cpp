// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

#if !wxUSE_GLCANVAS
#error "OpenGL required: set wxUSE_GLCANVAS to 1 and rebuild the library"
#endif

#define wxUSE_GUI 1

#include <string>
#include <windows.h>
#include <opencv2/opencv.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <wx/tglbtn.h>
#include <wx/filepicker.h>
#include <wx/wfstream.h>
#include "ObjViewer.h"
#include "OVCanvas.h"
#include "OVUtil.h"

namespace ov
{

// MyFrame constructor
ObjViewer::ObjViewer(const wxString& title)
    : wxFrame(NULL, wxID_ANY, title, wxDefaultPosition, wxSize(660, 566))
{
    _renderMode = RENDER_SOLID;
    _lightingOn = true;

    // Data path
#ifdef RESEARCH_HANDTRACKING
    _dataFolder = "../../../data/PenPoseTracker/";
#else
    _dataFolder = "./";
#endif
    _objModelFile = _dataFolder + "model/cube/cube.obj";
    _imageFile = _dataFolder + "image/black.png";

    // Make the "File" menu
    wxMenu *fileMenu = new wxMenu;
    fileMenu->Append(ID_MENU_OPEN_MODEL, wxT("Open &Model"), "Open .obj model file");
    fileMenu->Append(ID_MENU_OPEN_BACKGROUND_IMAGE, wxT("Open &Background Image"), "Open background image file");
    fileMenu->Append(ID_MENU_SAVE_IMAGE, wxT("S&ave Image"), "Save current frame to image file");
    fileMenu->Append(ID_MENU_GEN_SEQ, wxT("G&enerate Sequences"), "G&enerate Image Sequences with Poses");
    fileMenu->AppendSeparator();
    fileMenu->Append(ID_MENU_EXIT, wxT("E&xit\tEsc"), "Quit this program");
    // Make the "Help" menu
    wxMenu *helpMenu = new wxMenu;
    helpMenu->Append(ID_MENU_HELP, wxT("&About\tF1"), "Show about dialog");

    wxMenuBar *menuBar = new wxMenuBar;
    menuBar->Append(fileMenu, wxT("&File"));
    menuBar->Append(helpMenu, wxT("&Help"));
    SetMenuBar(menuBar);

    CreateStatusBar();
    SetStatusText("OBJ Viewer");

    SetIcon(wxICON(IDI_APP));
    wxColour backgroundColor = wxColour(240, 240, 240);
    SetBackgroundColour(backgroundColor);
    _mainSizer = new wxBoxSizer(wxHORIZONTAL);
    SetSizer(_mainSizer);

    // ==================================== Canvas part ====================================
    _ovCanvas = new OVCanvas(this, ID_CANVAS, wxDefaultPosition, GetClientSize(), wxSUNKEN_BORDER);
    _mainSizer->Add(_ovCanvas, 1, wxEXPAND);
    _ovCanvas->setRenderMode(_renderMode);
    _ovCanvas->setBackgroundImamge(_imageFile);
    _ovCanvas->setForegroundObject(_objModelFile);

    // ==================================== Controller part ====================================
    _controllerSizer = new wxBoxSizer(wxVERTICAL);
    _mainSizer->Add(_controllerSizer, 0, wxEXPAND);
    const wxString render_mode_radio_string[] =
    {
        wxT("Solid"),
        wxT("Wireframe"),
    };
    _renderModeRadioBox = new wxRadioBox(this,
                                         ID_RENDER_MODE_RADIO,
                                         wxT("&Render mode"),
                                         wxDefaultPosition,
                                         wxDefaultSize,
                                         WXSIZEOF(render_mode_radio_string),
                                         render_mode_radio_string,
                                         1,
                                         wxRA_SPECIFY_COLS);
    _controllerSizer->Add(_renderModeRadioBox, 0, wxEXPAND | wxALL, 5);
    _lightingCheckBox = CreateCheckBoxAndAddToSizer(this,
                                                    _controllerSizer,
                                                    wxT("Lighting"),
                                                    ID_LIGHTING);
    _lightingCheckBox->SetValue(true);
    _resetButton = new wxButton(this, ID_RESET, "Reset");
    _controllerSizer->Add(_resetButton, 0, wxEXPAND | wxALL, 5);

    reLayout();
    Show(true);

    Connect(ID_MENU_OPEN_MODEL, wxEVT_MENU, wxCommandEventHandler(ObjViewer::onMenuFileOpenObjModel));
    Connect(ID_MENU_OPEN_BACKGROUND_IMAGE, wxEVT_MENU, wxCommandEventHandler(ObjViewer::onMenuFileOpenBackgroundImage));
    Connect(ID_MENU_SAVE_IMAGE, wxEVT_MENU, wxCommandEventHandler(ObjViewer::onMenuFileSaveImage));
    Connect(ID_MENU_GEN_SEQ, wxEVT_MENU, wxCommandEventHandler(ObjViewer::onMenuGenerateSequence));
    Connect(ID_MENU_EXIT, wxEVT_MENU, wxCommandEventHandler(ObjViewer::onMenuFileExit));
    Connect(ID_MENU_HELP, wxEVT_MENU, wxCommandEventHandler(ObjViewer::onMenuHelpAbout));
    Connect(ID_RENDER_MODE_RADIO, wxEVT_RADIOBOX, wxCommandEventHandler(ObjViewer::onRenderModeRadio));
    Connect(ID_LIGHTING, wxEVT_CHECKBOX, wxCommandEventHandler(ObjViewer::onLightingCheck));
    Connect(ID_RESET, wxEVT_BUTTON, wxCommandEventHandler(ObjViewer::onReset));
}

ObjViewer::~ObjViewer()
{
    if (_ovCanvas) delete _ovCanvas;
}

void
ObjViewer::onMenuFileOpenObjModel(wxCommandEvent& WXUNUSED(evt))
{
    std::string objModelFile = wxFileSelector(wxT("Choose OBJ Model"), _dataFolder + "model", wxT(""), wxT(""),
        wxT("Wavefront Files (*.obj)|*.obj|All files (*.*)|*.*"),
        wxFD_OPEN);

    if (objModelFile != "")
    {
        SetStatusText("Loading the new model file...");
        if (_ovCanvas->setForegroundObject(objModelFile))
        {
            _ovCanvas->setIsNewFile(true);
            _ovCanvas->resetMatrix();
            _objModelFile = objModelFile;
        }
        SetStatusText(GetFileName(_objModelFile));
    }
}

void
ObjViewer::onMenuFileOpenBackgroundImage(wxCommandEvent& WXUNUSED(evt))
{
    std::string imageFile = wxFileSelector(wxT("Choose Image"), _dataFolder + "image", wxT(""), wxT(""),
        wxT("Image Files (*.bmp;*.pbm;*.pgm;*.ppm;*.sr;*.ras;*.jpeg;*.jpg;*.jpe;*.jp2;*.tiff;*.tif;*.png)| \
             *.bmp;*.pbm;*.pgm;*.ppm;*.sr;*.ras;*.jpeg;*.jpg;*.jpe;*.jp2;*.tiff;*.tif;*.png|All files (*.*)|*.*"),
        wxFD_OPEN);

    if (imageFile != "")
    {
        if (_ovCanvas->setBackgroundImamge(imageFile))
        {
            _ovCanvas->setIsNewFile(true);
            _imageFile = imageFile;
            _ovCanvas->resetMatrix();
            reLayout();
        }
    }
}

void
ObjViewer::onMenuFileSaveImage(wxCommandEvent& WXUNUSED(evt))
{
    wxFileDialog saveFileDialog(this, wxT("Save Image"), _dataFolder + "image", "",
        wxT("Image Files (*.bmp;*.pbm;*.pgm;*.ppm;*.sr;*.ras;*.jpeg;*.jpg;*.jpe;*.jp2;*.tiff;*.tif;*.png)| \
             *.bmp;*.pbm;*.pgm;*.ppm;*.sr;*.ras;*.jpeg;*.jpg;*.jpe;*.jp2;*.tiff;*.tif;*.png|All files (*.*)|*.*"),
        wxFD_SAVE | wxFD_OVERWRITE_PROMPT);

    if (saveFileDialog.ShowModal() == wxID_CANCEL)
        return;     // the user changed idea...

    // save the current contents in the file;
    // this can be done with e.g. wxWidgets output streams:
    wxFileOutputStream output_stream(saveFileDialog.GetPath());
    if (!output_stream.IsOk())
    {
        wxLogError("Cannot save current contents in file '%s'.", saveFileDialog.GetPath());
        return;
    }

    std::string filename = saveFileDialog.GetPath();
    cv::Mat image;
    _ovCanvas->printScreen(image);
    imwrite(filename, image);
}

void
ObjViewer::onMenuGenerateSequence(wxCommandEvent& WXUNUSED(evt))
{
    std::string generativeFile = wxFileSelector(wxT("Choose Generative File"), _dataFolder + "batch", wxT(""), wxT(""),
        wxT("Generative Files (*.txt)|*.txt|All files (*.*)|*.*"),
        wxFD_OPEN);

    if (generativeFile == "")
        return;

    Vec3 r, t;
    double s;
    _ovCanvas->getOffsetPose(r, t, s);
    _ovCanvas->setOffsetPose(Vec3(0, 0, 0), Vec3(0, 0, 0), 1);
    double planeNear = OVCanvas::PlaneNear;
    double planeFar = OVCanvas::PlaneFar;
    OVCanvas::PlaneNear = 1;
    OVCanvas::PlaneFar = 10000;

    std::string batchDir = GetDir(generativeFile);
    std::ifstream genIStream(generativeFile);
    std::string line;
    std::string modelFile;
    while (std::getline(genIStream, line))
    {
        std::stringstream lineStream(line);
        std::string token, posesFile;
        // 1. .OBJ model file
        lineStream >> modelFile;
        if (!_ovCanvas->setForegroundObject(modelFile, false))
            break;

        // 2. Background image file
        lineStream >> token;
        if (!_ovCanvas->setBackgroundImamge(token))
            break;

        // 3. Camera parameter file
        lineStream >> token;
        if (!_ovCanvas->readCameraParameters(token))
            break;

        reLayout();

        // 4. Poses file
        lineStream >> posesFile;
        Mat poses = LoadMatrix(posesFile);

        // 5. Sigma of Gaussian blur kernal
        lineStream >> token;
        double blurSigma = std::stod(token);

        // 6. Variance of Gaussian noise
        lineStream >> token;
        double noiseVariance = std::stod(token);

        // 7. Output directory
        lineStream >> token;
        std::string imageDir = batchDir + token;
        if (!IsDirectoryExists(imageDir))
            CreateDirectorys(imageDir);

        int num = poses.rows();
        for (int i = 0; i < num; ++i)
        {
            Mat3 R;
            Vec3 t;
            R << poses(i, 0), poses(i, 3), poses(i, 6),
                 poses(i, 1), poses(i, 4), poses(i, 7),
                 poses(i, 2), poses(i, 5), poses(i, 8);
            t << poses(i, 9), poses(i, 10), poses(i, 11);
            _ovCanvas->forceRender(R, t);
            cv::Mat image;
            _ovCanvas->printScreen(image);

            // Image processing
            if (blurSigma != 0)
                cv::GaussianBlur(image, image, cv::Size(0, 0), blurSigma, blurSigma);
            if (noiseVariance != 0)
            {
                cv::Mat gaussianNoise = cv::Mat(image.size(), CV_8UC3);

                cv::randn(gaussianNoise, 0, noiseVariance);
                image += gaussianNoise;
                normalize(image, image, 0, 255, CV_MINMAX, CV_8UC3);
            }

            std::string imageFile = imageDir + ZeroPadNumber(i, 6) + ".png";
            cv::imwrite(imageFile, image);
            std::string statusTxt =   "Now processing: " + posesFile
                                    + ", Sigma of Gaussian blur kernal: " + std::to_string(blurSigma)
                                    + ", Variance of Gaussian noise: " + std::to_string(noiseVariance)
                                    + ", Frame index: " + std::to_string(i + 1) + "/" + std::to_string(num);
            SetStatusText(statusTxt);
        }
    }

    _ovCanvas->setForegroundObject(modelFile);
    _ovCanvas->setOffsetPose(r, t, s);
    OVCanvas::PlaneNear = planeNear;
    OVCanvas::PlaneFar = planeFar;
    _ovCanvas->forceRender(Mat3::Identity(), Vec3::Zero());
    SetStatusText("OBJ Viewer");
}

void 
ObjViewer::onMenuFileExit(wxCommandEvent& WXUNUSED(evt))
{
    // true is to force the frame to close
    Close(true);
}

void
ObjViewer::onMenuHelpAbout(wxCommandEvent& WXUNUSED(evt))
{
    wxString msg = "Mouse left button: rotation\n"
        + std::string("Mouse middle button: Move up, down, left, and right\n")
        + std::string("Mouse right button: Move forward and backward\n\n\n")
        + std::string("For image sequence generation, the file format should be\n")
        + std::string("  <model> <image> <camemra> <poses> <blur> <noise> <output>")
        + std::string("    <model> : OBJ model file")
        + std::string("    <image> : Background image file")
        + std::string("    <camera>: Camera parameter file")
        + std::string("    <poses> : Poses file")
        + std::string("    <blur>  : Sigma of Gaussian blur kernel")
        + std::string("    <noise> : variance of Gaussian noise")
        + std::string("    <output>: Output directory");
    wxMessageBox(msg);
}

void
ObjViewer::onRenderModeRadio(wxCommandEvent& WXUNUSED(evt))
{
    int renderMode = _renderModeRadioBox->GetSelection();
    if (renderMode != _renderMode)
    {
        _renderMode = renderMode;
        _ovCanvas->setRenderMode(renderMode);
    }
}

void
ObjViewer::onLightingCheck(wxCommandEvent& WXUNUSED(evt))
{
    _lightingOn = _lightingCheckBox->GetValue();
    _ovCanvas->setLightingOn(_lightingOn);
}

void
ObjViewer::onReset(wxCommandEvent& WXUNUSED(evt))
{
    _ovCanvas->resetMatrix();
}

void
ObjViewer::reLayout()
{
    int w, h;
    SetMinSize(wxSize(-1, -1));
    Fit();
    GetSize(&w, &h);
    SetMinSize(wxSize(w, h));
    Centre();
}

} // namespace ov
