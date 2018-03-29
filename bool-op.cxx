////program for vtkLoopBooleanPolyDataFilter
///// newer implementation (2016, https://gitlab.kitware.com/vtk/vtk/merge_requests/1752) than vtkBooleanOperationPolyDataFilter (2011)
///// should solve some problems of vtkBooleanOperationPolyDataFilter (see article)
//01: based on enclosed_points.cxx and http://lorensen.github.io/VTKExamples/site/Cxx/PolyData/LoopBooleanPolyDataFilter/




#include <vtkSmartPointer.h>
#include <vtkXMLPolyDataReader.h>
#include <vtkLoopBooleanPolyDataFilter.h>
#include <vtkXMLPolyDataWriter.h>

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

    if (argc != 6){
        std::cerr << "Usage: " << argv[0]
                  << " input"
                  << " mask-mesh"
                  << " output"
                  << " compress"
                  << " operation"
//                  << " tolerance" // fraction of the bounding box of the enclosing surface?
                  << std::endl;
        return EXIT_FAILURE;
        }

    if(!(strcasestr(argv[1],".vtp"))) {
        std::cerr << "The input should end with .vtp" << std::endl;
        return -1;
        }

    if(!(strcasestr(argv[2],".vtp"))) {
        std::cerr << "The mask-mesh should end with .vtp" << std::endl;
        return -1;
        }

    if(!(strcasestr(argv[3],".vtp"))) {
        std::cerr << "The output should end with .vtp" << std::endl;
        return -1;
        }


    VTK_CREATE(vtkCallbackCommand, eventCallbackVTK);
    eventCallbackVTK->SetCallback(FilterEventHandlerVTK);


    VTK_CREATE(vtkXMLPolyDataReader, reader);
    reader->SetFileName(argv[1]);
    reader->AddObserver(vtkCommand::AnyEvent, eventCallbackVTK);
    reader->Update();

    VTK_CREATE(vtkXMLPolyDataReader, reader2);
    reader2->SetFileName(argv[2]);
    reader2->AddObserver(vtkCommand::AnyEvent, eventCallbackVTK);
    reader2->Update();

    VTK_CREATE(vtkLoopBooleanPolyDataFilter, filter);
    filter->SetInputConnection(0, reader->GetOutputPort());
    filter->SetInputConnection(1, reader2->GetOutputPort());
    filter->SetOperation(atoi(argv[5]));
    // filter->SetTolerance(atof(argv[6]));
    filter->AddObserver(vtkCommand::AnyEvent, eventCallbackVTK);
    filter->Update();

    VTK_CREATE(vtkXMLPolyDataWriter, writer);
    writer->SetInputConnection(filter->GetOutputPort());
    writer->SetFileName(argv[3]);
    writer->SetDataModeToBinary();//SetDataModeToAscii()//SetDataModeToAppended()
    if(atoi(argv[4]))
        writer->SetCompressorTypeToZLib();//default
    else
        writer->SetCompressorTypeToNone();
    writer->AddObserver(vtkCommand::AnyEvent, eventCallbackVTK);
    writer->Write();

    return EXIT_SUCCESS;
    }


