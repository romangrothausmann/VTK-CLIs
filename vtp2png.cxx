////program to render VTPs to PNG
//01: based on template.cxx and http://www.vtk.org/Wiki/VTK/Examples/Cxx/Utilities/Screenshot




#include <vtkSmartPointer.h>
#include <vtkXMLPolyDataReader.h>
#include <vtkPolyDataMapper.h>
#include <vtkActor.h>
#include <vtkRenderer.h>
#include <vtkRenderWindow.h>
#include <vtkWindowToImageFilter.h>
#include <vtkPNGWriter.h>

#include <vtkCallbackCommand.h>
#include <vtkCommand.h>


#define VTK_CREATE(type, name) vtkSmartPointer<type> name = vtkSmartPointer<type>::New()


void FilterEventHandlerVTK(vtkObject* caller, long unsigned int eventId, void* clientData, void* callData){

    vtkAlgorithm *filter= static_cast<vtkAlgorithm*>(caller);

    switch(eventId){
    case vtkCommand::ProgressEvent:
        fprintf(stderr, "\r%s progress: %5.1f%%", filter->GetClassName(), 100.0 * filter->GetProgress());//stderr is flushed directly
        break;
    case vtkCommand::EndEvent:
        std::cerr << std::endl << std::flush;
        break;

        //// VTK does not throw errors (http://public.kitware.com/pipermail/vtkusers/2009-February/050805.html) use Error-Events: http://www.cmake.org/Wiki/VTK/Examples/Cxx/Utilities/ObserveError
    case vtkCommand::ErrorEvent:
        std::cerr << "Error: " << static_cast<char*>(callData) << std::endl << std::flush;
        break;
    case vtkCommand::WarningEvent:
        std::cerr << "Warning: " << static_cast<char*>(callData) << std::endl << std::flush;
        break;
        }
    }


int main (int argc, char *argv[]){

    if (argc != 5){
        std::cerr << "Usage: " << argv[0]
                  << " input"
                  << " output"
                  << " width height"
                  << std::endl;
        return EXIT_FAILURE;
        }

    if(!(strcasestr(argv[1],".vtp"))) {
        std::cerr << "The input should end with .vtp" << std::endl;
        return -1;
        }

    if(!(strcasestr(argv[2],".png"))) {
        std::cerr << "The output should end with .png" << std::endl;
        return -1;
        }


    VTK_CREATE(vtkCallbackCommand, eventCallbackVTK);
    eventCallbackVTK->SetCallback(FilterEventHandlerVTK);


    VTK_CREATE(vtkXMLPolyDataReader, reader);
    reader->SetFileName(argv[1]);
    reader->AddObserver(vtkCommand::AnyEvent, eventCallbackVTK);
    reader->Update();

    VTK_CREATE(vtkPolyDataMapper, mapper);
    mapper->SetInputConnection(reader->GetOutputPort());

    VTK_CREATE(vtkActor, actor);
    actor->SetMapper(mapper);

    VTK_CREATE(vtkRenderer, renderer);
    renderer->AddActor(actor);
    renderer->SetBackground(1,1,1); // white

    VTK_CREATE(vtkRenderWindow, renderWindow);
    renderWindow->AddRenderer(renderer);
    renderWindow->SetSize(atoi(argv[3]), atoi(argv[4]));
    renderWindow->SetAlphaBitPlanes(1); // enable alpha channel
    renderWindow->Render();

    VTK_CREATE(vtkWindowToImageFilter, filter);
    filter->SetInput(renderWindow);
    // filter->SetMagnification(atof(argv[3])); // stiching is buggy, set renderWindow size instead
    filter->SetInputBufferTypeToRGBA();
    filter->ReadFrontBufferOff(); // read from the back buffer

    filter->AddObserver(vtkCommand::AnyEvent, eventCallbackVTK);
    filter->Update();

    VTK_CREATE(vtkPNGWriter, writer);
    writer->SetInputConnection(filter->GetOutputPort());
    writer->SetFileName(argv[2]);
    // writer->SetCompressionLevel(atoi(argv[3]));
    // writer->AddObserver(vtkCommand::AnyEvent, eventCallbackVTK); // reports no progress
    writer->Write();

    return EXIT_SUCCESS;
    }


