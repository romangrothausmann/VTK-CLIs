////program for
//01: based on template.cxx




#include <vtkSmartPointer.h>
#include <vtkMetaImageReader.h>
#include <vtkXMLPolyDataWriter.h>

#include <vtkCallbackCommand.h>
#include <vtkCommand.h>


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

    if (argc != 4){
        std::cerr << "Usage: " << argv[0]
                  << " input"
                  << " output"
                  << " compress"
                  << std::endl;
        return EXIT_FAILURE;
        }

    if(!(strcasestr(argv[1],".mha") || strcasestr(argv[1],".mhd"))) {
        std::cerr << "The second input should end with .mha or .mhd" << std::endl;
        return -1;
        }

    if(!(strcasestr(argv[2],".vtp"))) {
        std::cerr << "The output should end with .vtp" << std::endl;
        return -1;
        }

    vtkSmartPointer<vtkCallbackCommand> eventCallbackVTK = vtkSmartPointer<vtkCallbackCommand>::New();
    eventCallbackVTK->SetCallback(FilterEventHandlerVTK);

    vtkSmartPointer<vtkXMLPolyDataReader> reader= vtkSmartPointer<vtkXMLPolyDataReader>::New();
    vtkSmartPointer<> filter= vtkSmartPointer<>::New();
    vtkSmartPointer<vtkXMLPolyDataWriter> writer= vtkSmartPointer<vtkXMLPolyDataWriter>::New();

    reader->SetFileName(argv[1]);
    reader->AddObserver(vtkCommand::AnyEvent, eventCallbackVTK);
    reader->Update();

    filter->SetInputConnection(0, reader->GetOutputPort());
    filter->AddObserver(vtkCommand::AnyEvent, eventCallbackVTK);
    filter->Update();

    writer->SetInputConnection(filter->GetOutputPort());
    writer->SetFileName(argv[2]);
    writer->SetDataModeToBinary();//SetDataModeToAscii()//SetDataModeToAppended()
    if(atoi(argv[3]))
        writer->SetCompressorTypeToZLib();//default
    else
        writer->SetCompressorTypeToNone();
    writer->AddObserver(vtkCommand::AnyEvent, eventCallbackVTK);
    writer->Write();

    return EXIT_SUCCESS;
    }


