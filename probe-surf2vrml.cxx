// program to apply vtkProbeFilter and to save result as WRL/VRML
//01: based on template.cxx




#include <vtkSmartPointer.h>
#include <vtkMetaImageReader.h>
#include <vtkXMLPolyDataReader.h>
#include <vtkProbeFilter.h>
#include <vtkLookupTable.h>
#include <vtkPolyDataMapper.h>
#include <vtkActor.h>
#include <vtkRenderer.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkVRMLExporter.h>

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

    if(!(strcasestr(argv[3],".vrml") || strcasestr(argv[3],".wrl"))) {
        std::cerr << "The output should end with .wrl or .vrml" << std::endl;
        return -1;
        }


    vtkSmartPointer<vtkCallbackCommand> eventCallbackVTK = vtkSmartPointer<vtkCallbackCommand>::New();
    eventCallbackVTK->SetCallback(FilterEventHandlerVTK);


    vtkSmartPointer<vtkXMLPolyDataReader> reader0= vtkSmartPointer<vtkXMLPolyDataReader>::New();
    reader0->SetFileName(argv[1]);
    reader0->AddObserver(vtkCommand::AnyEvent, eventCallbackVTK);
    reader0->Update();

    vtkSmartPointer<vtkMetaImageReader> reader1= vtkSmartPointer<vtkMetaImageReader>::New();
    reader1->SetFileName(argv[2]);
    reader1->AddObserver(vtkCommand::AnyEvent, eventCallbackVTK);
    reader1->Update();

    vtkSmartPointer<vtkProbeFilter> filter= vtkSmartPointer<vtkProbeFilter>::New();
    filter->SetInputConnection(0, reader0->GetOutputPort());
    filter->SetInputConnection(1, reader1->GetOutputPort());
    filter->AddObserver(vtkCommand::AnyEvent, eventCallbackVTK);
    filter->Update();

    vtkSmartPointer<vtkLookupTable> lut= vtkSmartPointer<vtkLookupTable>::New();
    //lut->SetNumberOfColors(100);
    //lut->SetHueRange(1,0.0);//rainbow
    //lut->SetHueRange(0.0, 0.667);// This creates a red to blue lut.
    lut->SetHueRange(0.667, 0.0);// This creates a blue to red lut.

    lut->SetRange(atof(argv[4]), atof(argv[5]));
    lut->Build();

    vtkSmartPointer<vtkPolyDataMapper> mapper= vtkSmartPointer<vtkPolyDataMapper>::New();
    mapper->SetInputConnection(filter->GetOutputPort());
    mapper->SetLookupTable(lut);

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

    vtkSmartPointer<vtkVRMLExporter> writer= vtkSmartPointer<vtkVRMLExporter>::New();
    writer->SetInput(renderWindow);
    writer->SetFileName(argv[3]);
    //writer->SetSpeed(5.5)//default 4
    writer->AddObserver(vtkCommand::AnyEvent, eventCallbackVTK);
    std::cerr << "VRML export... ";
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
