// program to apply vtkProbeFilter and to save result as WRL/VRML
//01: based on template.cxx




#include <vtkSmartPointer.h>
#include <vtkImageReader2Factory.h>
#include <vtkImageReader2.h>
#include <vtkXMLPolyDataReader.h>
#include <vtkProbeFilter.h>
#include <vtkColorTransferFunction.h>
#include <vtkPolyDataMapper.h>//as input is definitly vtkPolyData othersiwe use vtkDataSetMapper
#include <vtkImageData.h>//reader1->GetOutput()
#include <vtkPointData.h>//reader1->GetOutput()->GetPointData()
#include <vtkActor.h>
#include <vtkRenderer.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkX3DExporter.h>

#include <vtkCallbackCommand.h>
#include <vtkCommand.h>

#include <GL/glx.h>
#include <GL/gl.h>
int checkOpenGL();


void FilterEventHandlerVTK(vtkObject* caller, long unsigned int eventId, void* clientData, void* callData){

    vtkAlgorithm *filter= static_cast<vtkAlgorithm*>(caller);

    switch(eventId){
    case vtkCommand::ProgressEvent:
        fprintf(stderr, "\r%s progress: %5.1f%%", filter->GetClassName(), 100.0 * filter->GetProgress());//stderr is flushed directly
        break;
    case vtkCommand::EndEvent:
        std::cerr << std::endl << std::flush;
        break;
        }
    }


int main (int argc, char *argv[]){

    if (argc != 7){
        std::cerr << "Usage: " << argv[0]
                  << " input-surf input-vol"
                  << " output"
                  << " lut-start lut-end"
                  << " render"
                  << std::endl;
        return EXIT_FAILURE;
        }

    if(!(strcasestr(argv[1],".vtp"))) {
        std::cerr << "The first input should end with .vtp" << std::endl;
        return -1;
        }

    if(!(strcasestr(argv[2],".mha") || strcasestr(argv[2],".mhd"))) {
        std::cerr << "The second input should end with .mha or .mhd" << std::endl;
        return -1;
        }

    if(!(strcasestr(argv[3],".x3d"))) {
        std::cerr << "The output should end with .x3d" << std::endl;
        return -1;
        }


    vtkSmartPointer<vtkCallbackCommand> eventCallbackVTK = vtkSmartPointer<vtkCallbackCommand>::New();
    eventCallbackVTK->SetCallback(FilterEventHandlerVTK);


    vtkSmartPointer<vtkXMLPolyDataReader> reader0= vtkSmartPointer<vtkXMLPolyDataReader>::New();
    reader0->SetFileName(argv[1]);
    reader0->AddObserver(vtkCommand::AnyEvent, eventCallbackVTK);
    reader0->Update();

    vtkSmartPointer<vtkImageReader2Factory> readerFactory= vtkSmartPointer<vtkImageReader2Factory>::New();    
    vtkImageReader2* reader1= readerFactory->CreateImageReader2(argv[2]);
    if(!reader1){
        std::cerr << "Could not find an appropriate reader. Does file exist?" << std::endl;
        return -1;
        }	
    reader1->SetFileName(argv[2]);
    reader1->AddObserver(vtkCommand::AnyEvent, eventCallbackVTK);
    reader1->Update();//does not throw an error if file cannot be read! Use CanReadFile if readerFactory ought to be avoided

    vtkSmartPointer<vtkProbeFilter> filter= vtkSmartPointer<vtkProbeFilter>::New();
    filter->SetInputConnection(reader0->GetOutputPort());
    filter->SetSourceConnection(reader1->GetOutputPort());
    filter->PassPointArraysOff();
    filter->PassCellArraysOff();
    filter->PassFieldArraysOff();
    filter->ComputeToleranceOn();
    filter->AddObserver(vtkCommand::AnyEvent, eventCallbackVTK);
    filter->Update();

    vtkSmartPointer<vtkColorTransferFunction> lut= vtkSmartPointer<vtkColorTransferFunction>::New();
    lut->SetColorSpaceToRGB();
    lut->AddRGBPoint(atof(argv[4]),0,0,1);//blue
    lut->AddRGBPoint(atof(argv[5]),1,0,0);//red
    lut->SetScaleToLinear();

    vtkSmartPointer<vtkPolyDataMapper> mapper= vtkSmartPointer<vtkPolyDataMapper>::New();
    mapper->SetInputConnection(filter->GetOutputPort());
    mapper->SetLookupTable(lut);
    mapper->SetScalarModeToUsePointFieldData();//only then is SelectColorArray used
    mapper->SelectColorArray(reader1->GetOutput()->GetPointData()->GetArrayName(0));//"MetaImage"
    //mapper->SelectColorArray(filter->GetValidPointMaskArrayName());//"vtkValidPointMask"
    mapper->ScalarVisibilityOn();//seems to be default
    mapper->UseLookupTableScalarRangeOn();//essential!

    vtkSmartPointer<vtkActor> actor= vtkSmartPointer<vtkActor>::New();
    actor->SetMapper(mapper);

    vtkSmartPointer<vtkRenderer> renderer= vtkSmartPointer<vtkRenderer>::New();
    renderer->AddActor(actor);
    renderer->SetBackground(1, 1, 1);//white

    vtkSmartPointer<vtkRenderWindow> renderWindow= vtkSmartPointer<vtkRenderWindow>::New();
    //renderWindow->OffScreenRenderingOn();
    renderWindow->AddRenderer(renderer);

    vtkSmartPointer<vtkRenderWindowInteractor> renderWindowInteractor= vtkSmartPointer<vtkRenderWindowInteractor>::New();
    renderWindowInteractor->SetRenderWindow(renderWindow);

    ////rendering not needed for vtkVRMLExporter
    if(atoi(argv[6])){
        if(checkOpenGL()){
            std::cerr << "OpenGL context is direct. Rendering should be save in conjunction with vglrun." << std::endl;
            renderWindow->Render();
            renderWindowInteractor->Start();
            }
        else
            std::cerr << "OpenGL context is indirect. Not rendering to prevent crashes of Xvnc when vglrun is used later on!" << std::endl;
        }

    vtkSmartPointer<vtkX3DExporter> writer= vtkSmartPointer<vtkX3DExporter>::New();
    writer->SetInput(renderWindow);
    writer->SetFileName(argv[3]);
    //writer->SetSpeed(5.5)//default 4
    writer->AddObserver(vtkCommand::AnyEvent, eventCallbackVTK);
    std::cerr << "X3D export... ";
    writer->Write();
    std::cerr << "done." << std::endl;

    return EXIT_SUCCESS;
    }


int checkOpenGL(){
    ////see https://www.opengl.org/discussion_boards/showthread.php/165856-Minimal-GLX-OpenGL3-0-example?p=1178905&viewfull=1#post1178905  and  glxdemos/glxspheres.c
    ///basicly only the result of glXIsDirect is needed, however it needs some variables...

    Display *dpy= XOpenDisplay(0);

    int fbcount;

    //// these three cause a segfault
    // static int attributeList[]= { GLX_RENDER_TYPE, GLX_RGBA_BIT, GLX_DOUBLEBUFFER, GLX_RED_SIZE, 1, GLX_GREEN_SIZE, 1, GLX_BLUE_SIZE, 1, None };
    // GLXFBConfig *fbc = glXChooseFBConfig(dpy, DefaultScreen(dpy), attributeList, &fbcount);
    // XVisualInfo *vi = glXGetVisualFromFBConfig(dpy, fbc[0]);

    //// these work
    static int attributeList[] = { GLX_RGBA, GLX_DOUBLEBUFFER, GLX_RED_SIZE, 1, GLX_GREEN_SIZE, 1, GLX_BLUE_SIZE, 1, None };
    GLXFBConfig *fbc = glXChooseFBConfig(dpy, DefaultScreen(dpy), 0, &fbcount);
    XVisualInfo *vi = glXChooseVisual(dpy, DefaultScreen(dpy), attributeList);

    XSetWindowAttributes swa;
    swa.colormap= XCreateColormap(dpy, RootWindow(dpy, vi->screen), vi->visual, AllocNone);
    swa.border_pixel= 0;
    swa.event_mask= StructureNotifyMask;
    Window win= XCreateWindow(dpy, RootWindow(dpy, vi->screen), 0, 0, 100, 100, 0, vi->depth, InputOutput, vi->visual, CWBorderPixel|CWColormap|CWEventMask, &swa);

    XMapWindow(dpy, win);

    GLXContext ctx= glXCreateContext(dpy, vi, 0, GL_TRUE);

    //fprintf(stderr, "OpenGL Renderer: %s\n", glGetString(GL_RENDERER));//returns Null
    return(glXIsDirect(dpy, ctx));
    }
