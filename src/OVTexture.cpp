#include <wx/msgdlg.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <string>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include "OVTexture.h"
#include "OVUtil.h"
#include "TinyObjLoader.h"

namespace ov
{

GLubyte HeaderUTGA[12] = { 0,0, 2,0,0,0,0,0,0,0,0,0 }; // Uncompressed TGA Header
GLubyte HeaderCTGA[12] = { 0,0,10,0,0,0,0,0,0,0,0,0 }; // Compressed TGA Header

bool
LoadTextures(std::vector<tinyobj::material_t>& materials,
             std::unordered_map<std::string, GLuint>& textureIds,
             const std::string& dir)
  {
    for (int i = 0; i < materials.size(); ++i)
    {
        std::string map_Kd = materials[i].diffuse_texname;

        // Remove leading space
        const auto strBegin = map_Kd.find_first_not_of(" \t");
        if (!(strBegin == std::string::npos))
        {
            const auto strEnd = map_Kd.find_last_not_of(" \t");
            const auto strRange = strEnd - strBegin + 1;
            map_Kd = map_Kd.substr(strBegin, strRange);
        }

        if (map_Kd != "" && textureIds.find(map_Kd) == textureIds.end())
        {
            cv::Mat texture;
            if (!LoadTexture(texture, dir + map_Kd))
                return false;

            GLuint textureId;
            int width = texture.cols;
            int height = texture.rows;

            if ((width * 3) % 4 == 0)
                glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
            else
                glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

            glGenTextures(1, &textureId);
            glBindTexture(GL_TEXTURE_2D, textureId);

            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST_MIPMAP_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

            int type = (texture.type() == CV_8UC3) ? GL_BGR : GL_BGRA;
            gluBuild2DMipmaps(GL_TEXTURE_2D, GL_RGBA, width, height, type, GL_UNSIGNED_BYTE, texture.data);

            textureIds[map_Kd] = textureId;
        }
    }

    return true;
}

bool
LoadTexture(cv::Mat& texture, const std::string& filename)
{
    texture.release();
    std::string ext = GetExt(filename);
    for (int i = 0; i < ext.size(); ++i)
        ext[i] = tolower(ext[i]);

    if (ext == "tga")
        return LoadTGA(texture, filename);
    else
    {
        cv::flip(cv::imread(filename, CV_LOAD_IMAGE_COLOR), texture, 0);
        if (texture.empty())
        {
            wxString msg = "Cannot open \"" + filename + "\"";
            wxMessageBox(msg, wxT("Error"), wxICON_ERROR);
            return false;
        }
    }

    return true;
}

bool
LoadTGA(cv::Mat& texture, const std::string& filename)
{
    FILE * fTGA;
    fTGA = fopen(filename.c_str(), "rb");
    if (fTGA == NULL)
    {
        wxString msg = "Cannot open \"" + filename + "\"";
        wxMessageBox(msg, wxT("Error"), wxICON_ERROR);
        return false;
    }

    bool isCompressed;

    GLubyte headerUC[12];
    if (fread(&headerUC, sizeof(headerUC), 1, fTGA) == 0)
    {
        wxString msg = "Cannot read header of \"" + filename + "\"";
        wxMessageBox(msg, wxT("Error"), wxICON_ERROR);
        return false;
    }

    if (memcmp(HeaderUTGA, &headerUC, sizeof(headerUC)) == 0)
        isCompressed = false;
    else if (memcmp(HeaderCTGA, &headerUC, sizeof(headerUC)) == 0)
        isCompressed = true;
    else
    {
        wxString msg = "Cannot parse \"" + filename + "\"\n(TGA file should be type 2 or type 10)\n";
        wxMessageBox(msg, wxT("Error"), wxICON_ERROR);
        fclose(fTGA);
        return false;
    }

    GLubyte headerInfo[6];
    if (fread(headerInfo, sizeof(headerInfo), 1, fTGA) == 0)
    {
        wxString msg = "Cannot read first part header of \"" + filename + "\"";
        wxMessageBox(msg, wxT("Error"), wxICON_ERROR);
        return false;
    }

    int width = headerInfo[1] * 256 + headerInfo[0];
    int height = headerInfo[3] * 256 + headerInfo[2];
    int bpp = headerInfo[4];
    bool flipV = (headerInfo[5] & 0x20) != 0;
    bool flipH = (headerInfo[5] & 0x10) != 0;
    if ((width <= 0) || (height <= 0) || ((bpp != 24) && (bpp != 32)))
    {
        wxString msg = "Invalid header of \"" + filename + "\"";
        wxMessageBox(msg, wxT("Error"), wxICON_ERROR);
        return false;
    }

    int type = (bpp == 24) ? CV_8UC3 : CV_8UC4;
    int bytesPerPixel = (bpp == 24) ? 3 : 4;
    texture = cv::Mat(height, width, type);
    int imageSize = height * width;

    if (isCompressed)
    {
        int currentpixel = 0;
        int currentbyte = 0;
        GLubyte *colorbuffer = new GLubyte[bytesPerPixel];
        do
        {
            GLubyte chunkheader = 0;
            if (fread(&chunkheader, sizeof(GLubyte), 1, fTGA) == 0)
            {
                wxString msg = "Invalid header of \"" + filename + "\"";
                wxMessageBox(msg, wxT("Error"), wxICON_ERROR);
                fclose(fTGA);
                delete[] colorbuffer;
                return false;
            }

            if (chunkheader < 128)
            {
                chunkheader++;
                for (short counter = 0; counter < chunkheader; counter++)
                {
                    if (fread(colorbuffer, 1, bytesPerPixel, fTGA) != bytesPerPixel)
                    {
                        wxString msg = "Cannot read \"" + filename + "\"";
                        wxMessageBox(msg, wxT("Error"), wxICON_ERROR);
                        fclose(fTGA);
                        delete[] colorbuffer;
                        return false;
                    }

                    // write to memory
                    texture.data[currentbyte] = colorbuffer[0];
                    texture.data[currentbyte + 1] = colorbuffer[1];
                    texture.data[currentbyte + 2] = colorbuffer[2];
                    if (bytesPerPixel == 4) texture.data[currentbyte + 3] = colorbuffer[3];

                    currentbyte += bytesPerPixel;
                    currentpixel++;

                    if (currentpixel > imageSize)
                    {
                        wxString msg = "Too many pixels in \"" + filename + "\"";
                        wxMessageBox(msg, wxT("Error"), wxICON_ERROR);
                        fclose(fTGA);
                        delete[] colorbuffer;
                        return false;
                    }
                }
            }
            else
            {
                chunkheader -= 127;
                if (fread(colorbuffer, 1, bytesPerPixel, fTGA) != bytesPerPixel)
                {
                    wxString msg = "Cannot read \"" + filename + "\"";
                    wxMessageBox(msg, wxT("Error"), wxICON_ERROR);
                    fclose(fTGA);
                    delete[] colorbuffer;
                    return false;
                }

                for (int i = 0; i < chunkheader; ++i)
                {
                    texture.data[currentbyte] = colorbuffer[0];
                    texture.data[currentbyte + 1] = colorbuffer[1];
                    texture.data[currentbyte + 2] = colorbuffer[2];
                    if (bytesPerPixel == 4) texture.data[currentbyte + 3] = colorbuffer[3];

                    currentbyte += bytesPerPixel;
                    currentpixel++;

                    if (currentpixel > imageSize)
                    {
                        wxString msg = "Too many pixels in \"" + filename + "\"";
                        wxMessageBox(msg, wxT("Error"), wxICON_ERROR);
                        fclose(fTGA);
                        delete[] colorbuffer;
                        return false;
                    }
                }
            }
        } while (currentpixel < imageSize);
        delete[] colorbuffer;
    }
    else
    {
        if (fread(texture.data, bytesPerPixel, imageSize, fTGA) != imageSize)
        {
            wxString msg = "Cannot read the content of \"" + filename + "\"";
            wxMessageBox(msg, wxT("Error"), wxICON_ERROR);
            fclose(fTGA);
            return false;
        }
    }

    if (flipV)
        cv::flip(texture, texture, 0);
    if (flipH)
        cv::flip(texture, texture, 1);

    fclose(fTGA);
    return true;
}

} // namespace ov