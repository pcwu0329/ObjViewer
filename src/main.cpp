#include "main.h"
#include "ObjViewer.h"
#include <iostream>
namespace ov
{

IMPLEMENT_APP(MyApp)

// The program execution starts here
bool MyApp::OnInit()
{
    ObjViewer *viewer = new ObjViewer(wxT("OBJ Viewer"));

    return true;
}

} // namespace ov

