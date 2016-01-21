////program for
//01: based on template.cxx




#include <vtkSmartPointer.h>
#include <vtkXMLPolyDataReader.h>
#include <vtkPolyDataMapper.h>//as input is definitly vtkPolyData othersiwe use vtkDataSetMapper
#include <vtkActor.h>
#include <vtkRenderer.h>
#include <vtkRenderWindow.h>
#include <vtkX3DExporter.h>

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

    if (argc != 3){
        std::cerr << "Usage: " << argv[0]
                  << " input"
                  << " output"
                  << std::endl;
        return EXIT_FAILURE;
        }

    if(!(strcasestr(argv[1],".vtp"))) {
        std::cerr << "The input should end with .vtp" << std::endl;
        return -1;
        }

    if(!(strcasestr(argv[2],".x3d"))) {
        std::cerr << "The output should end with .x3d" << std::endl;
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
    renderer->SetBackground(1, 1, 1);//white

    VTK_CREATE(vtkRenderWindow, renderWindow);
    //renderWindow->OffScreenRenderingOn();
    renderWindow->AddRenderer(renderer);

    VTK_CREATE(vtkX3DExporter, writer);
    writer->SetInput(renderWindow);
    writer->SetFileName(argv[2]);
    //writer->SetSpeed(5.5)//default 4
    writer->AddObserver(vtkCommand::AnyEvent, eventCallbackVTK);
    std::cerr << "X3D export... ";
    writer->Write();
    std::cerr << "done." << std::endl;

    return EXIT_SUCCESS;
    }


