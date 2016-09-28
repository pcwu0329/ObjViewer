#pragma once

#include <wx/wx.h>

namespace ov
{

// Define a new application
class MyApp : public wxApp
{
public:
    virtual bool OnInit() wxOVERRIDE;
};

} // namespace ov