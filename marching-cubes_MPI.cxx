////program to create a marching-cubes surface with vtkContourFilter
//01: based on marching-cubes.cxx




#include <vtkSmartPointer.h>
#include <vtkMPIController.h>
#include <vtkXMLImageDataReader.h>//seems to be the only reader that works, has outInfo->Set(CAN_PRODUCE_SUB_EXTENT(), 1)  neither vtkMetaImageReader nor vtkPNrrdReader worked, vtkMPIImageReader (RAW) not tested
#include <vtkContourFilter.h>//handles sub-extents of pvtp correctly, seems independent of vtkMarchingCubes (which does not handle sub-extents of pvtp correctly)
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

    vtkIdType numCells = filter->GetOutput()->GetNumberOfCells();
    vtkIdType numPoints = filter->GetOutput()->GetNumberOfPoints();
    cerr << "Process (" << myId << "): "
	 << "# points: " << numPoints
         << "# cells: " << numCells << endl;

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

    vtkIdType totalNumPoints = 0;
    vtkIdType totalNumCells = 0;
    controller->Reduce(&numPoints, &totalNumPoints, 1, vtkCommunicator::SUM_OP, 0);//does this stop thread 0 until totalNumCells is fully acquired? If so should only be used after all processing is done!
    controller->Reduce(&numCells, &totalNumCells, 1, vtkCommunicator::SUM_OP, 0);//does this stop thread 0 until totalNumCells is fully acquired? If so should only be used after all processing is done!
    if (myId == 0)
        cerr << endl << "Process (" << myId << ") total: "
	     << "# points: " << totalNumPoints
             << "# cells: " << totalNumCells << endl;

    controller->Finalize();

    return EXIT_SUCCESS;
    }


