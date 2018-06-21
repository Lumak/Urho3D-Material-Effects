//
// Copyright (c) 2008-2016 the Urho3D project.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//

#include <Urho3D/Core/Context.h>
#include <Urho3D/Core/ProcessUtils.h>
#include <Urho3D/Core/StringUtils.h>
#include <Urho3D/IO/File.h>
#include <Urho3D/IO/FileSystem.h>
#include <Urho3D/IO/Log.h>
#include <Urho3D/Resource/Image.h>
#include <Urho3D/Resource/XMLElement.h>
#include <Urho3D/Resource/XMLFile.h>

#ifdef WIN32
#include <windows.h>
#include <stdio.h>
#endif

#include <Urho3D/DebugNew.h>

using namespace Urho3D;

//=============================================================================
//=============================================================================
int main(int argc, char** argv);
void Run(Vector<String>& arguments);

//=============================================================================
//=============================================================================
const char *getSequenceDecFormat(int idx, bool leadingZero)
{
    const char *dec1Format = "%1d";
    const char *dec2Format = "%2d";
    const char *dec3Format = "%3d";
    const char *dec02Format = "%02d";
    const char *dec03Format = "%03d";

    if (idx == 1)
    {
        return dec1Format;
    }
    else if (idx == 2)
    {
        return leadingZero?dec02Format:dec2Format;
    }

    return leadingZero?dec03Format:dec3Format;
}

//=============================================================================
//=============================================================================
void Help(const String &message = String::EMPTY)
{
    if (!message.Empty())
    {
        PrintLine(message);
    }

    ErrorExit("SequenceImagePacker, version 0.01, by Lumak 2017\n"
              "Usage: SequenceImagePacker inputFolderPath -options\n\n"
              "options:\n"
              "-sp seq image filename prefix, e.g. fire001.png would be fire\n"
              "-sx seq image filename ext, e.g. jpg, png, bmp, etc.\n"
              "-ss seq start num\n"
              "-se seq end num\n"
              "-sf seq digit format(e.g. fire001.png = 03, leading zero digit format), range[1, 3] or [01, 03]\n"
              "-fw image frame width (default = image width)\n"
              "-fh image frame height (default = image height)\n"
              "-ox x offset (default = 0)\n"
              "-oy y offset (default = 0)\n"
              "-outx output extension (default = sx, image filename ext)\n"
              "-v verbose output\n"
              "-h shows this help message\n\n"
              "Example: SequenceImagePacker myfilepath -sp fire -sx png -ss 4 -se 32 -sf 02 -ox 22 -fh 40 -outx jpg\n\n"
              "Any files missing in the sequence will not terminate the program. You can get the warnings with '-v' option.\n"
              "Output file will be placed in the inputFolderPath as prefixName'SEQ'.ext\n\n");
}

//=============================================================================
//=============================================================================
int main(int argc, char** argv)
{
    Vector<String> arguments;

#ifdef WIN32
    arguments = ParseArguments(GetCommandLineW());
#else
    arguments = ParseArguments(argc, argv);
#endif

    Run(arguments);
    return 0;
}

void Run(Vector<String>& arguments)
{
    // min num args = exe(1), input path(1), sp, sx, ss and se (4*2)
    if (arguments.Size() < 2 + 4 * 2)
    {
        Help("Missing args, requires at least input path, sp, sx, ss and se\n");
    }

    SharedPtr<Context> context(new Context());
    context->RegisterSubsystem(new FileSystem(context));
    context->RegisterSubsystem(new Log(context));
    FileSystem* fileSystem = context->GetSubsystem<FileSystem>();

    String inputPath;
    String seqPrefix;
    String seqExt;
    String outExt;
    int seqStart = 0;
    int seqEnd = 0;
    int seqFormat = 1;
    String strFormat;
    bool hasLeadingZero = false;
    unsigned components = 0;
    int depth = 0;

    int offsetX = 0;
    int offsetY = 0;
    int frameWidth = 0;
    int frameHeight = 0;
    bool verbose = false;

    // input path
    inputPath = arguments[0];
    arguments.Erase(0);

    // parse args
    while (arguments.Size() > 0)
    {
        String arg = arguments[0];
        arguments.Erase(0);

        if (arg.Empty())
            continue;

        if (arg.StartsWith("-"))
        {
                 if (arg == "-sp"  ) { seqPrefix = arguments[0]; arguments.Erase(0); }
            else if (arg == "-sx"  ) { seqExt = arguments[0]; arguments.Erase(0); }
            else if (arg == "-ss"  ) { seqStart = ToInt(arguments[0]); arguments.Erase(0); }
            else if (arg == "-se"  ) { seqEnd = ToInt(arguments[0]); arguments.Erase(0); }
            else if (arg == "-sf"  ) { strFormat = arguments[0]; arguments.Erase(0); }
            else if (arg == "-fw"  ) { frameWidth = ToInt(arguments[0]); arguments.Erase(0); }
            else if (arg == "-fh"  ) { frameHeight = ToInt(arguments[0]); arguments.Erase(0); }
            else if (arg == "-ox"  ) { offsetX = ToInt(arguments[0]); arguments.Erase(0); }
            else if (arg == "-oy"  ) { offsetY = ToInt(arguments[0]); arguments.Erase(0); }
            else if (arg == "-outx") { outExt = arguments[0]; arguments.Erase(0); }
            else if (arg == "-v"   ) { verbose = true; }
            else if (arg == "-h"   ) { Help(); }

        }
        else
        {
            Help("Wrong arg order?");
        }
    }

    // evaluate seqs
    if (seqStart < 0 || seqEnd < 0 || seqEnd - seqStart < 2)
    {
        ErrorExit("improper ss and/or se");
    }

    // check dec format
    hasLeadingZero = strFormat.StartsWith("0");
    const char *p = hasLeadingZero?strFormat.CString()+1:strFormat.CString();
    seqFormat = Variant(VAR_INT, p).GetInt();

    if (seqFormat < 1 || seqFormat > 3)
    {
        ErrorExit("sf not in range");
    }

    // validate input path
    String filePath;

    if (!inputPath.Empty())
    {
        inputPath = RemoveTrailingSlash(inputPath);

        if (fileSystem->DirExists(inputPath))
        {
            filePath = inputPath;
        }

        filePath = AddTrailingSlash(filePath);
    }

    if (verbose)
    {
        PrintLine("Input path: " + GetPath(filePath));
        PrintLine("Seq start " + String(seqStart) + ", end "+ String(seqEnd));
    }

    // query how many files we can open to determine the layout
    int itotalFiles = 0;
    int imgH=0, imgW=0;

    for ( int i = seqStart; i <= seqEnd; ++i )
    {
        char buff[16];
        sprintf(buff, getSequenceDecFormat(seqFormat, hasLeadingZero), i);
        String filename = filePath + seqPrefix + String(buff) + "." + seqExt;

        if (!fileSystem->FileExists(filename))
        {
            continue;
        }

        File file(context, filename);
        Image image(context);
        if (!image.Load(file))
        {
            continue;
        }

        if (components == 0)
        {
            components = image.GetComponents();
            depth = image.GetDepth();
        }

        int imageWidth = image.GetWidth();
        int imageHeight = image.GetHeight();

        if (imgW == 0)
        {
            imgW = imageWidth;
            if (frameWidth > 0 || offsetX > 0)
            {
                if (imageWidth < frameWidth + offsetX)
                {
                    if (frameWidth > 0)
                    {
                        if (verbose)
                        {
                            PrintLine("fw + ox > image width, changing fw to fit");
                        }
                        frameWidth = imageWidth - offsetX;
                    }
                    else
                    {
                        ErrorExit("ox > image width");
                    }
                }
            }
        }
        else if (imgW != imageWidth)
        {
            ErrorExit("inconsistent image width");
        }

        if (imgH == 0)
        {
            imgH = imageHeight;
            if (frameHeight > 0 || offsetY > 0)
            {
                if (imageHeight < frameHeight + offsetY)
                {
                    if (frameHeight > 0)
                    {
                        if (verbose)
                        {
                            PrintLine("fh + oy > image height, changing fh to fit");
                        }
                        frameHeight = imageHeight - offsetY;
                    }
                    else
                    {
                        ErrorExit("oy > image height");
                    }
                }
            }
        }
        else if (imgH != imageHeight)
        {
            ErrorExit("inconsistent image height");
        }

        ++itotalFiles;
    }

    if (itotalFiles == 0)
    {
        ErrorExit("didn't find any files to open");
    }

    if (verbose)
    {
        PrintLine("Num image files to pack: " + String(itotalFiles));
    }

    // check components
    if (components == 0)
    {
        ErrorExit("image component not detected");
    }

    // determine an efficient layout, min rows = sqrtNum/1.5 to avoid creating a single row
    int sqrtNum = (int)ceil(sqrt((float)itotalFiles));
    int rows = sqrtNum;
    int cols = (int)ceil((float)itotalFiles/(float)rows);
    int maxTiles = rows * cols;
    int minRows = (int)((float)sqrtNum/(1.5f));

    for ( int i = 1; i < minRows; ++i )
    {
        int r = sqrtNum - i;
        int c = (int)ceil((float)itotalFiles/(float)r);
        if (r * c < maxTiles)
        {
            rows = r;
            cols = c;
            maxTiles = r * c;
        }
    }

    if (verbose)
    {
        PrintLine("Packing images: row " + String(rows) + ", col " + String(cols));
    }

    // pack sequence images
    int readEndW = frameWidth>0?offsetX+frameWidth:imgW;
    int readEndH = frameHeight>0?offsetY+frameHeight:imgH;
    int writeW = frameWidth>0?frameWidth:imgW-offsetX;
    int writeH = frameHeight>0?frameHeight:imgH-offsetY;

    if (verbose)
    {
        PrintLine("Pixels to read(" + String(readEndW) + ", " + String(readEndH) + "), to write("+ 
                  String(writeW) + ", " + String(writeH) + ") per image.");
    }

    Image packedImage(context);
    packedImage.SetSize(cols * writeW, rows * writeH, depth, components);
    packedImage.Clear(Color::BLACK);

    for ( int r = 0; r < rows; ++r )
    {
        int seq = seqStart + r * cols;

        for ( int c = 0; c < cols && seq <= seqEnd; ++c, ++seq )
        {
            char buff[16];
            sprintf(buff, getSequenceDecFormat(seqFormat, hasLeadingZero), seq);
            String filename = filePath + seqPrefix + String(buff) + "." + seqExt;

            if (!fileSystem->FileExists(filename))
            {
                if (verbose)
                {
                    PrintLine("File not found: " + GetFileNameAndExtension(filename));
                }

                continue;
            }

            File file(context, filename);
            Image image(context);

            if (!image.Load(file))
            {
                if (verbose)
                {
                    PrintLine("Failed to read image: " + GetFileNameAndExtension(filename));
                }

                continue;
            }

            for ( int yr = offsetY, yw = 0; yr < readEndH; ++yr, ++yw )
            {
                for ( int xr = offsetX, xw = 0; xr < readEndW; ++xr, ++xw )
                {
                    unsigned color = image.GetPixelInt(xr, yr);
                    packedImage.SetPixelInt(xw + c * writeW, yw + r * writeH, color);
                }
            }
        }
    }

    // save file
    String ext = !outExt.Empty()?outExt:seqExt;
    String filename = filePath + seqPrefix + "SEQ." + ext;
    bool saved = false;

    if (ext.Compare("JPG", false))
    {
        saved = packedImage.SaveJPG(filename, 100);
    }
    else if (ext.Compare("PNG", false))
    {
        saved = packedImage.SavePNG(filename);
    }
    else if (ext.Compare("TGA", false))
    {
        saved = packedImage.SaveTGA(filename);
    }
    else if (ext.Compare("BMP", false))
    {
        saved = packedImage.SaveBMP(filename);
    }

    String outstr = saved ? "File saved as: " : "Failed to save: ";
    outstr += GetPath(filename) + GetFileNameAndExtension(filename);
    PrintLine(outstr);

    if (saved)
    {
        PrintLine("row " + String(rows) + ", col " + String(cols) + ", num images " + String(itotalFiles));
    }
}

