////program for vtkSTLWriter
//01: based on template.cxx




#include <vtkSmartPointer.h>
#include <vtkXMLPolyDataReader.h>
#include <vtkTriangleFilter.h>
#include <vtkSTLWriter.h>

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

    if (argc != 4){
        std::cerr << "Usage: " << argv[0]
                  << " input"
                  << " output"
                  << " binary|ASCII"
                  << std::endl;
        return EXIT_FAILURE;
        }

    if(!(strcasestr(argv[1],".vtp"))) {
        std::cerr << "The output should end with .vtp" << std::endl;
        return -1;
	}

    if(!(strcasestr(argv[2],".stl"))) {
        std::cerr << "The output should end with .stl" << std::endl;
        return -1;
        }


    VTK_CREATE(vtkCallbackCommand, eventCallbackVTK);
    eventCallbackVTK->SetCallback(FilterEventHandlerVTK);


    VTK_CREATE(vtkXMLPolyDataReader, reader);
    reader->SetFileName(argv[1]);
    reader->AddObserver(vtkCommand::AnyEvent, eventCallbackVTK);
    reader->Update();

    VTK_CREATE(vtkTriangleFilter, filter);
    filter->SetInputConnection(reader->GetOutputPort());
    filter->AddObserver(vtkCommand::AnyEvent, eventCallbackVTK);

    VTK_CREATE(vtkSTLWriter, writer);
    writer->SetInputConnection(filter->GetOutputPort());
    writer->SetFileName(argv[2]);
    if(atoi(argv[3]))
        writer->SetFileTypeToBinary();
    else
        writer->SetFileTypeToASCII();
    writer->AddObserver(vtkCommand::AnyEvent, eventCallbackVTK);
    writer->Write();

    return EXIT_SUCCESS;
    }


