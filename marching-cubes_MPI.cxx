////program to create a marching-cubes surface with vtkContourFilter
//01: based on marching-cubes.cxx




#include <vtkSmartPointer.h>
#include <vtkMPIController.h>
#include <vtkXMLImageDataReader.h>//seems to be the only reader that works, has outInfo->Set(CAN_PRODUCE_SUB_EXTENT(), 1)  neither vtkMetaImageReader nor vtkPNrrdReader worked, vtkMPIImageReader (RAW) not tested
#include <vtkInformation.h>//for GetOutputInformation
#include <vtkStreamingDemandDrivenPipeline.h>//for extent
#include <vtkImageData.h>//for GetExtent()
#include <vtkImageConstantPad.h>
#include <vtkContourFilter.h>//handles sub-extents of pvtp correctly, seems independent of vtkMarchingCubes (which does not handle sub-extents of pvtp correctly)
#include <vtkWindowedSincPolyDataFilter.h>
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

    if (argc != 7){
        std::cerr << "Usage: " << argv[0]
                  << " input"
                  << " output"
                  << " compress"
                  << " iso-value"
                  << " cap-surface"
                  << " smooth"
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
    reader->UpdateInformation();//needed to get extent

    vtkSmartPointer<vtkContourFilter> filter= vtkSmartPointer<vtkContourFilter>::New();
    vtkSmartPointer<vtkImageConstantPad> pad = vtkSmartPointer<vtkImageConstantPad>::New();

    if(atoi(argv[5])){
        int extent[6];
        reader->GetOutputInformation(0)->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), extent);

        fprintf(stderr, "extent: %d, %d, %d, %d, %d, %d\n", extent[0], extent[1], extent[2], extent[3], extent[4], extent[5]);

        pad->SetInputConnection(reader->GetOutputPort());
        pad->SetOutputWholeExtent(
            extent[0] -1, extent[1] + 1,
            extent[2] -1, extent[3] + 1,
            extent[4] -1, extent[5] + 1);
	pad->UpdateInformation();
	pad->SetUpdateExtent(0, myId, numProcs, 0);
	//pad->Update();
        filter->SetInputConnection(pad->GetOutputPort());
        }
    else
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

    int smoothingIterations= atoi(argv[6]);
    if(smoothingIterations){
        std::cerr << "Smoothing creates seams between pieces or cannot smooth over discontinuities!!!" << std::endl;
	vtkSmartPointer<vtkWindowedSincPolyDataFilter> smoother= vtkSmartPointer<vtkWindowedSincPolyDataFilter>::New();
	smoother->SetInputConnection(filter->GetOutputPort());
	smoother->SetNumberOfIterations(smoothingIterations);
	smoother->NonManifoldSmoothingOn();
	smoother->NormalizeCoordinatesOn();
	smoother->BoundarySmoothingOff(); // try to avoids seams but does not smooth over discontinuities!
	smoother->AddObserver(vtkCommand::AnyEvent, eventCallbackVTK);
	smoother->UpdateInformation();
	smoother->SetUpdateExtent(0, myId, numProcs, 0);
	smoother->Update();
	writer->SetInputConnection(smoother->GetOutputPort());
	}
    else{
	writer->SetInputConnection(filter->GetOutputPort());
	}

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


