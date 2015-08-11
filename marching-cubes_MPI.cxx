////program to create a marching-cubes surface with vtkContourFilter
//01: based on marching-cubes.cxx




#include <vtkSmartPointer.h>
#include <vtkMPIController.h>
#include <vtkXMLImageDataReader.h>//seems to be the only reader that works, has outInfo->Set(CAN_PRODUCE_SUB_EXTENT(), 1)  neither vtkMetaImageReader nor vtkPNrrdReader worked, vtkMPIImageReader (RAW) not tested
#include <vtkContourFilter.h>
#include <vtkXMLPPolyDataWriter.h>

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
                  << " compress"
                  << " iso-value"
                  << std::endl;
        return EXIT_FAILURE;
        }

    if(!(strcasestr(argv[1],".vti"))) {
        std::cerr << "The input should end with .vti" << std::endl;
        return -1;
        }

    if(!(strcasestr(argv[2],".pvtp"))) {
        std::cerr << "The output should end with .pvtp" << std::endl;
        return -1;
        }


    vtkSmartPointer<vtkCallbackCommand> eventCallbackVTK = vtkSmartPointer<vtkCallbackCommand>::New();
    eventCallbackVTK->SetCallback(FilterEventHandlerVTK);

    vtkSmartPointer<vtkMPIController> controller= vtkSmartPointer<vtkMPIController>::New();
    controller->Initialize(&argc, &argv);
    int myId = controller->GetLocalProcessId();
    int numProcs = controller->GetNumberOfProcesses();

    vtkSmartPointer<vtkXMLImageDataReader> reader= vtkSmartPointer<vtkXMLImageDataReader>::New();
    reader->SetFileName(argv[1]);
    reader->AddObserver(vtkCommand::AnyEvent, eventCallbackVTK);
    //reader->UpdateInformation();//not needed

    vtkSmartPointer<vtkContourFilter> filter= vtkSmartPointer<vtkContourFilter>::New();
    filter->SetInputConnection(reader->GetOutputPort());
    filter->SetValue(0, atof(argv[4]));
    filter->ComputeScalarsOff();
    filter->ComputeGradientsOff();
    filter->AddObserver(vtkCommand::AnyEvent, eventCallbackVTK);
    filter->UpdateInformation();
    filter->SetUpdateExtent(0, myId, numProcs, 0);
    filter->Update();

    vtkSmartPointer<vtkXMLPPolyDataWriter> writer= vtkSmartPointer<vtkXMLPPolyDataWriter>::New();
    writer->SetInputConnection(filter->GetOutputPort());
    writer->SetFileName(argv[2]);
    writer->SetNumberOfPieces(numProcs);
    writer->SetStartPiece(myId);
    writer->SetEndPiece(myId);
    writer->SetDataModeToBinary();//SetDataModeToAscii()//SetDataModeToAppended()
    if(atoi(argv[3]))
        writer->SetCompressorTypeToZLib();//default
    else
        writer->SetCompressorTypeToNone();
    writer->AddObserver(vtkCommand::AnyEvent, eventCallbackVTK);
    writer->Write();

    controller->Finalize();

    return EXIT_SUCCESS;
    }


