// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

#if !wxUSE_GLCANVAS
#error "OpenGL required: set wxUSE_GLCANVAS to 1 and rebuild the library"
#endif

#define wxUSE_GUI 1

#include "wx/glcanvas.h"
#include <opencv2/opencv.hpp>
#include <fstream>
#include <iomanip>
#include <algorithm>
#include "ObjViewer.h"
#include "OVCanvas.h"
#include "OVTexture.h"
#include "OVUtil.h"
#include "OVCommon.h"

namespace ov
{

int OVCanvas::FrameWidth = 800;
int OVCanvas::FrameHeight = 600;
double OVCanvas::PlaneNear = 0.01;
double OVCanvas::PlaneFar = 100;

OVCanvas::OVCanvas(ObjViewer *objViewer,
                   wxWindowID id,
                   wxPoint pos,
                   wxSize size,
                   long style,
                   wxString name)
    : wxGLCanvas(objViewer, id, NULL, pos, size, style | wxFULL_REPAINT_ON_RESIZE, name)
{
    _objViewer = objViewer;

    // Offset transformation coefficients
    _offsetRotation = { 180, 0, 0 };
    _offsetTranslation = { 0, 0, 7 };
    _offsetScale = 1;

    _mousePos = Vec2::Zero();

    _oglContext = NULL;
    _renderMode = RENDER_SOLID;
    _isNewFile = false;
    _lightingOn = true;
    resetMatrix();

    Connect(wxEVT_PAINT, wxPaintEventHandler(OVCanvas::onPaint));
    Connect(wxEVT_SIZE, wxSizeEventHandler(OVCanvas::onSize));
    Connect(wxEVT_IDLE, wxIdleEventHandler(OVCanvas::onIdle));
    Connect(wxEVT_MOTION, wxMouseEventHandler(OVCanvas::onMouse));
    Connect(wxEVT_MOUSEWHEEL, wxMouseEventHandler(OVCanvas::onMouseWheel));

    // Explicitly create a new rendering context instance for this canvas.
    _oglContext = new wxGLContext(this);
    
    oglInit();
}

OVCanvas::~OVCanvas()
{
    if (_oglContext) delete _oglContext;
}

bool
OVCanvas::setForegroundObject(const std::string& filename, bool isUnitization)
{
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::unordered_map<std::string, GLuint> textureIds;
    std::string dir = GetDir(filename);
    std::string err;

    if(!tinyobj::LoadObj(shapes,
                         materials,
                         err,
                         filename.c_str(),
                         dir.c_str(),
                         tinyobj::triangulation | tinyobj::calculate_normals))
    {
        wxString msg = err;
        wxMessageBox(msg, wxT("Error"), wxICON_ERROR);
        return false;
    }
    
    if (!LoadTextures(materials, textureIds, dir))
        return false;

    if (isUnitization)
        unitize(shapes);

    _shapes = shapes;
    _materials = materials;
    _textureIds = textureIds;

    return true;
}

bool
OVCanvas::setBackgroundImamge(const std::string& filename)
{
    cv::Mat cameraImage = cv::imread(filename, CV_LOAD_IMAGE_COLOR);
    if (cameraImage.empty())
    {
        wxString msg = "Cannot open \"" + filename + "\".\n";
        wxMessageBox(msg, wxT("Error"), wxICON_ERROR);
        return false;
    }

    _backgroundImage = cameraImage;
    FrameWidth = _backgroundImage.cols;
    FrameHeight = _backgroundImage.rows;

    glBindTexture(GL_TEXTURE_2D, _backgroundImageTextureId);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
    // set texture filter to linear - we do not build mipmaps for speed
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    // create the texture from OpenCV image data
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, FrameWidth, FrameHeight, 0, GL_BGR, GL_UNSIGNED_BYTE, _backgroundImage.data);

    // After getting the projection matrix, we do resize one time
    SetClientSize(wxSize(OVCanvas::FrameWidth, OVCanvas::FrameHeight));
    SetMinClientSize(wxSize(OVCanvas::FrameWidth, OVCanvas::FrameHeight));
    onSize(wxSizeEvent());

    return true;
}

bool
OVCanvas::readCameraParameters(const std::string& camParamFile)
{
    cv::FileStorage fs(camParamFile, cv::FileStorage::READ);
    if (!fs.isOpened())
        return false;
    cv::Mat cameraMatrix;
    fs["camera_matrix"] >> cameraMatrix;

    double fx = cameraMatrix.at<double>(0, 0);
    double fy = cameraMatrix.at<double>(1, 1);
    double cx = cameraMatrix.at<double>(0, 2);
    double cy = cameraMatrix.at<double>(1, 2);

    double w, h;
    fs["image_width"] >> w;
    fs["image_height"] >> h;

    assert(FrameWidth == w && FrameHeight && h);

    _projectionMatrix[0] = 2 * fx / w;
    _projectionMatrix[1] = 0;
    _projectionMatrix[2] = 0;
    _projectionMatrix[3] = 0;
    _projectionMatrix[4] = 0;
    _projectionMatrix[5] = -2 * fy / h;
    _projectionMatrix[6] = 0;
    _projectionMatrix[7] = 0;
    _projectionMatrix[8] = 2 * (cx / w) - 1;
    _projectionMatrix[9] = 1 - 2 * (cy / h);
    _projectionMatrix[10] = (OVCanvas::PlaneFar + OVCanvas::PlaneNear) / (OVCanvas::PlaneFar - OVCanvas::PlaneNear);
    _projectionMatrix[11] = 1;
    _projectionMatrix[12] = 0;
    _projectionMatrix[13] = 0;
    _projectionMatrix[14] = 2 * OVCanvas::PlaneFar*OVCanvas::PlaneNear / (OVCanvas::PlaneNear - OVCanvas::PlaneFar);
    _projectionMatrix[15] = 0;

    // After getting the projection matrix, we do resize one time
    onSize(wxSizeEvent());

    return true;
}

void
OVCanvas::setRenderMode(int renderMode)
{
    _renderMode = renderMode;
    Refresh();
}

void
OVCanvas::forceRender(const Mat3& R, const Vec3& t)
{
    _R = R;
    _t = t;
    onPaint(wxPaintEvent());
}

void
OVCanvas::printScreen(cv::Mat& image)
{
    GLint vp[4];
    glGetIntegerv(GL_VIEWPORT, vp);

    int x, y, w, h;
    x = vp[0];
    y = vp[1];
    w = vp[2];
    h = vp[3];

    image = cv::Mat(h, w, CV_8UC3);

    // Byte alignment (that is, no alignment)
    glPixelStorei(GL_PACK_ALIGNMENT, 1);

    // Read pixels from GPU memory
    glReadPixels(x, y, w, h, GL_BGR, GL_UNSIGNED_BYTE, image.data);

    // Flip around the x-axis
    cv::flip(image, image, 0);
}

void
OVCanvas::resetMatrix()
{
    // Get the default camera parameters accordring to the image size
    double fx = Vec2(FrameWidth, FrameHeight).norm();
    double fy = fx;
    double cx = (FrameWidth - 1.) / 2.;
    double cy = (FrameHeight - 1.) / 2.;
    double w = FrameWidth;
    double h = FrameHeight;

    // Set the projection matrix for opengl
    _projectionMatrix[0] = 2 * fx / w;
    _projectionMatrix[1] = 0;
    _projectionMatrix[2] = 0;
    _projectionMatrix[3] = 0;
    _projectionMatrix[4] = 0;
    _projectionMatrix[5] = -2 * fy / h;
    _projectionMatrix[6] = 0;
    _projectionMatrix[7] = 0;
    _projectionMatrix[8] = 2 * (cx / w) - 1;
    _projectionMatrix[9] = 1 - 2 * (cy / h);
    _projectionMatrix[10] = (PlaneFar + PlaneNear) / (PlaneFar - PlaneNear);
    _projectionMatrix[11] = 1;
    _projectionMatrix[12] = 0;
    _projectionMatrix[13] = 0;
    _projectionMatrix[14] = 2 * PlaneFar*PlaneNear / (PlaneNear - PlaneFar);
    _projectionMatrix[15] = 0;

    // Set the rotation matrix and translation vector
    _R = Mat3::Identity();
    _t = Vec3::Zero();

    // After getting the reseted matrices, we do resize one time
    onSize(wxSizeEvent());
}

void
OVCanvas::setLightingOn(bool lightingOn)
{
    _lightingOn = lightingOn;
    Refresh();
}

void
OVCanvas::setOffsetPose(const Vec3& r, const Vec3& t, const double s)
{
    _offsetRotation = r;
    _offsetTranslation = t;
    _offsetScale = s;
}

void
OVCanvas::getOffsetPose(Vec3& r, Vec3& t, double& s)
{
    r = _offsetRotation;
    t = _offsetTranslation;
    s = _offsetScale;
}

void
OVCanvas::onMouse(wxMouseEvent& evt)
{
    // It's weird that when we open a new model, this function will be executed
    if (_isNewFile)
    {
        _isNewFile = false;
        return;
    }

    if (evt.Dragging())
    {
        if (evt.LeftIsDown())
        {
            wxSize sz(GetClientSize());
            Mat3 R = Trackball(Vec2((2.0 * _mousePos(0) - sz.x) / sz.x, (sz.y - 2.0 * _mousePos(1)) / sz.y),
                               Vec2((2.0 * evt.GetX() - sz.x) / sz.x, (sz.y - 2.0 * evt.GetY()) / sz.y));
            _R = R * _R;
            Refresh();
        }
        else
        {
            wxSize sz(GetClientSize());
            double ratio = 4;
            double diffX = (evt.GetX() - _mousePos(0)) / sz.x;
            double diffY = (_mousePos(1) - evt.GetY()) / sz.y;
            _t += Vec3(diffX, diffY, 0) * ratio;
            Refresh();
        }
    }

    _mousePos = Vec2(evt.GetX(), evt.GetY());
}

void OVCanvas::onMouseWheel(wxMouseEvent& evt)
{
    if (evt.GetWheelAxis() != wxMOUSE_WHEEL_VERTICAL)
        return;

    double ratio = 0.005;
    _t -= Vec3(0, 0, evt.GetWheelRotation()) * ratio;
    Refresh();
}

void
OVCanvas::onIdle(wxIdleEvent& WXUNUSED(evt))
{
    // Nothing to do
}

void
OVCanvas::onPaint(wxPaintEvent& WXUNUSED(evt))
{
    SetCurrent(*_oglContext);

    // Render the background image
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_TEXTURE_2D);
    glDisable(GL_LIGHTING);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    drawBackground(_backgroundImageTextureId);
    glDisable(GL_TEXTURE_2D);

    glClear(GL_DEPTH_BUFFER_BIT);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    if (_lightingOn)
    {
        const GLfloat a[] = { 0.1f, 0.1f, 0.1f, 1.0f };
        const GLfloat d[] = { 0.5f, 0.5f, 0.5f, 1.0f };
        const GLfloat s[] = { 0.1f, 0.1f, 0.1f, 1.0f };
        const GLfloat p0[] = { 7.0f, 0.0f, 0.0f, 1.0f };
        const GLfloat p1[] = { -7.0f, 0.0f, 0.0f, 1.0f };
        const GLfloat p2[] = { 0.0f, 7.0f, 0.0f, 1.0f };
        const GLfloat p3[] = { 0.0f, -7.0f, 0.0f, 1.0f };
        glLightfv(GL_LIGHT0, GL_AMBIENT, a);
        glLightfv(GL_LIGHT0, GL_DIFFUSE, d);
        glLightfv(GL_LIGHT0, GL_SPECULAR, s);
        glLightfv(GL_LIGHT0, GL_POSITION, p0);
        glLightfv(GL_LIGHT1, GL_AMBIENT, a);
        glLightfv(GL_LIGHT1, GL_DIFFUSE, d);
        glLightfv(GL_LIGHT1, GL_SPECULAR, s);
        glLightfv(GL_LIGHT1, GL_POSITION, p1);
        glLightfv(GL_LIGHT2, GL_AMBIENT, a);
        glLightfv(GL_LIGHT2, GL_DIFFUSE, d);
        glLightfv(GL_LIGHT2, GL_SPECULAR, s);
        glLightfv(GL_LIGHT2, GL_POSITION, p2);
        glLightfv(GL_LIGHT3, GL_AMBIENT, a);
        glLightfv(GL_LIGHT3, GL_DIFFUSE, d);
        glLightfv(GL_LIGHT3, GL_SPECULAR, s);
        glLightfv(GL_LIGHT3, GL_POSITION, p3);
        glEnable(GL_LIGHTING);
    }

    // Render the foreground target
    glTranslatef(_offsetTranslation[0], _offsetTranslation[1], _offsetTranslation[2]);
    glRotatef(_offsetRotation[2], 0, 0, 1);
    glRotatef(_offsetRotation[1], 0, 1, 0);
    glRotatef(_offsetRotation[0], 1, 0, 0);
    glScalef(_offsetScale, _offsetScale, _offsetScale);
    // Fill in modelViewMatrix
    for (int i = 0; i < 3; ++i)
    {
        _modelViewMatrix[12 + i] = _t(i, 0);
        for (int j = 0; j < 3; ++j)
            _modelViewMatrix[i * 4 + j] = _R(j, i);
    }
    _modelViewMatrix[3] = _modelViewMatrix[7] = _modelViewMatrix[11] = 0;
    _modelViewMatrix[15] = 1;
    glMultMatrixd(_modelViewMatrix);

    // Semitransparent effect 
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    if (_renderMode == RENDER_SOLID)
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    else
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    drawForeground(_shapes, _materials, _textureIds);
    glDisable(GL_BLEND);

    glFlush();
    SwapBuffers();
}

void
OVCanvas::onSize(wxSizeEvent& WXUNUSED(evt))
{
    if (!IsShownOnScreen())
        return;

    int w, h;
    GetClientSize(&w, &h);

    glViewport(0, 0, (GLsizei)w, (GLsizei)h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glMultMatrixd(_projectionMatrix);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    Refresh();
}

void
OVCanvas::oglInit()
{
    SetCurrent(*_oglContext);

    // OpenGL initialization
    glClearColor(0.0, 0.0, 0.0, 0.0);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glShadeModel(GL_SMOOTH);
    glEnable(GL_TEXTURE_2D);
    glGenTextures(1, &_backgroundImageTextureId);

    glEnable(GL_LIGHT0);
    glEnable(GL_LIGHT1);
    glEnable(GL_LIGHT2);
    glEnable(GL_LIGHT3);
}

void 
OVCanvas::drawBackground(GLuint backgroundImageTextureId)
{
    SetCurrent(*_oglContext);

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(-1, 1, -1, 1, 0, 1);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    // Draw the quad textured with the background image
    glBindTexture(GL_TEXTURE_2D, backgroundImageTextureId);
    glBegin(GL_QUADS);
    glTexCoord2f(0, 1);
    glVertex2f(-1, -1);
    glTexCoord2f(0, 0);
    glVertex2f(-1, 1);
    glTexCoord2f(1, 0);
    glVertex2f(1, 1);
    glTexCoord2f(1, 1);
    glVertex2f(1, -1);
    glEnd();

    // Reset the projection matrix
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
}

void
OVCanvas::drawForeground(const std::vector<tinyobj::shape_t>& shapes,
                         const std::vector<tinyobj::material_t>& materials,
                         const std::unordered_map<std::string, GLuint>& textureIds)
{
    glDisable(GL_COLOR_MATERIAL);
    glEnable(GL_TEXTURE_2D);
    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    int preId = -1;
    bool isTexture = false;
    for (int i = 0; i < shapes.size(); ++i)
    {
        for (int f = 0; f < shapes[i].mesh.indices.size() / 3; ++f)
        {
            int material_id = shapes[i].mesh.material_ids[f];
            if (material_id != preId)
            {
                GLfloat ambient[4], diffuse[4], specular[4];
                memcpy(ambient, materials[material_id].ambient, 3 * sizeof(float));
                memcpy(diffuse, materials[material_id].diffuse, 3 * sizeof(float));
                memcpy(specular, materials[material_id].specular, 3 * sizeof(float));
                ambient[3] = diffuse[3] = specular[3] = materials[material_id].dissolve;
                glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, ambient);
                glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, diffuse);
                glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, specular);
                glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, materials[material_id].shininess);

                std::string map_Kd = materials[material_id].diffuse_texname;
                auto got = textureIds.find(map_Kd);
                if (got == textureIds.end())
                {
                    glBindTexture(GL_TEXTURE_2D, 0);
                    isTexture = false;
                }
                else
                {
                    glBindTexture(GL_TEXTURE_2D, got->second);
                    isTexture = true;
                }
                preId = material_id;
            }

            glBegin(GL_TRIANGLES);
            for (int j = 0; j < 3; ++j)
            {
                int idx = shapes[i].mesh.indices[3 * f + j];
                glNormal3fv(&shapes[i].mesh.normals[3 * idx]);
                if (isTexture) glTexCoord2fv(&shapes[i].mesh.texcoords[2 * idx]);
                glVertex3fv(&shapes[i].mesh.positions[3 * idx]);
            }
            glEnd();
        }
    }
}

void
OVCanvas::unitize(std::vector<tinyobj::shape_t>& shapes)
{
    float maxx = FLT_MIN;
    float minx = FLT_MAX;
    float maxy = FLT_MIN;
    float miny = FLT_MAX;
    float maxz = FLT_MIN;
    float minz = FLT_MAX;
    float cx, cy, cz, w, h, d;
    float scale;

    for (int i = 0; i < shapes.size(); ++i)
    {
        for (int v = 0; v < shapes[i].mesh.positions.size() / 3; ++v)
        {
            if (maxx < shapes[i].mesh.positions[3 * v + 0])
                maxx = shapes[i].mesh.positions[3 * v + 0];
            if (minx > shapes[i].mesh.positions[3 * v + 0])
                minx = shapes[i].mesh.positions[3 * v + 0];

            if (maxy < shapes[i].mesh.positions[3 * v + 1])
                maxy = shapes[i].mesh.positions[3 * v + 1];
            if (miny > shapes[i].mesh.positions[3 * v + 1])
                miny = shapes[i].mesh.positions[3 * v + 1];

            if (maxz < shapes[i].mesh.positions[3 * v + 2])
                maxz = shapes[i].mesh.positions[3 * v + 2];
            if (minz > shapes[i].mesh.positions[3 * v + 2])
                minz = shapes[i].mesh.positions[3 * v + 2];
        }
    }
    
    // Calculate model width, height, and depth
    w = abs(maxx) + abs(minx);
    h = abs(maxy) + abs(miny);
    d = abs(maxz) + abs(minz);

    // Calculate center of the model
    cx = (maxx + minx) / 2.0f;
    cy = (maxy + miny) / 2.0f;
    cz = (maxz + minz) / 2.0f;

    // Calculate unitizing scale factor
    scale = 2.0 / std::max(std::max(w, h), d);

    // Translate around center then scale
    for (int i = 0; i < shapes.size(); ++i)
    {
        for (int v = 0; v < shapes[i].mesh.positions.size() / 3; ++v)
        {
            shapes[i].mesh.positions[3 * v + 0] -= cx;
            shapes[i].mesh.positions[3 * v + 1] -= cy;
            shapes[i].mesh.positions[3 * v + 2] -= cz;
            shapes[i].mesh.positions[3 * v + 0] *= scale;
            shapes[i].mesh.positions[3 * v + 1] *= scale;
            shapes[i].mesh.positions[3 * v + 2] *= scale;
        }
    }
}

} // namespace ov