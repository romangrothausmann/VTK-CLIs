// program to apply vtkDiscreteMarchingCubes
//01: based on discrete_marching-cubes.cxx, ParallelCompositeTime/Parallel/parallel1_answer.cxx (kitware VTK-workshop), IO/MPIImage/Testing/Cxx/ParallelIso.cxx, Examples/ParallelProcessing/Generic/Cxx/ParallelIso.cxx and Examples/Infovis/Cxx/ParallelBFS.cxx



#include <vtkMPIController.h>
#include <vtkStreamingDemandDrivenPipeline.h>

#include <vtkSmartPointer.h>
#include <vtkMetaImageReader.h>//only 2 parallel readers exist (6.2): vtkMPIImageReader (RAW), vtkPNrrdReader (Nrrd)
#include <vtkImageData.h>//for GetExtent()
#include <vtkImageConstantPad.h>
#include <vtkDiscreteMarchingCubes.h>
#include <vtkWindowedSincPolyDataFilter.h>
#include <vtkXMLPPolyDataWriter.h>
#include <vtkFeatureEdges.h>

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
    int myId, numProcs;

    if (argc != 6){
	std::cerr << "Usage: " << argv[0]
		  << " input"
		  << " output"
		  << " StartLabel EndLabel"
		  << " cap-surface"
		  << std::endl;
	return EXIT_FAILURE;
	}

    vtkSmartPointer<vtkMPIController> controller =
	vtkSmartPointer<vtkMPIController>::New();
    controller->Initialize(&argc, &argv);

    // Obtain the id of the running process and the total
    // number of processes
    myId = controller->GetLocalProcessId();
    numProcs = controller->GetNumberOfProcesses();

    // Create all of the classes we will need
    vtkSmartPointer<vtkMetaImageReader> reader =
	vtkSmartPointer<vtkMetaImageReader>::New();
    vtkSmartPointer<vtkDiscreteMarchingCubes> discreteCubes =
	vtkSmartPointer<vtkDiscreteMarchingCubes>::New();
    vtkSmartPointer<vtkWindowedSincPolyDataFilter> smoother =
	vtkSmartPointer<vtkWindowedSincPolyDataFilter>::New();
    vtkSmartPointer<vtkImageConstantPad> pad =
        vtkSmartPointer<vtkImageConstantPad>::New();
    vtkSmartPointer<vtkFeatureEdges> featureEdges =
        vtkSmartPointer<vtkFeatureEdges>::New();
    vtkSmartPointer<vtkXMLPPolyDataWriter> writer =
	vtkSmartPointer<vtkXMLPPolyDataWriter>::New();

    vtkSmartPointer<vtkCallbackCommand> eventCallbackVTK = vtkSmartPointer<vtkCallbackCommand>::New();
    eventCallbackVTK->SetCallback(FilterEventHandlerVTK);


    // Define all of the variables
    unsigned int startLabel = atoi(argv[3]);
    unsigned int endLabel = atoi(argv[4]);
    vtkstd::string filePrefix = argv[2]; //"Label";
    unsigned int smoothingIterations = 20;

    reader->SetFileName(argv[1]);
    reader->UpdateInformation();

    if(atoi(argv[5])){
	int *extent = reader->GetOutput()->GetExtent(); //needs vtkImageData.h
	pad->SetInputConnection(reader->GetOutputPort());
	pad->SetOutputWholeExtent(
	    extent[0] -1, extent[1] + 1,
	    extent[2] -1, extent[3] + 1,
	    extent[4] -1, extent[5] + 1);
	pad->Update();
	discreteCubes->SetInputConnection(pad->GetOutputPort());
	}
    else
	discreteCubes->SetInputConnection(reader->GetOutputPort());

    discreteCubes->GenerateValues(
	endLabel - startLabel + 1, startLabel, endLabel);
    discreteCubes->UpdateInformation();
    discreteCubes->SetUpdateExtent(0, myId, numProcs, 0);
    discreteCubes->Update();

    vtkIdType numCells = discreteCubes->GetOutput()->GetNumberOfCells();
    cerr << "Process (" << myId << "): " 
	 << "Cells in output of contour on process after requesting"
	" 1 piece from " << numProcs << " pieces: " << numCells << endl;

    vtkIdType totalNumCells = 0;
    controller->Reduce(&numCells, &totalNumCells, 1, vtkCommunicator::SUM_OP, 0);
    if (myId == 0)
	cerr << "Process (" << myId << "): " 
	     << "Cell count after reduction: " << totalNumCells << endl;

    writer->SetNumberOfPieces(numProcs);
    writer->SetStartPiece(myId);
    writer->SetEndPiece(myId);
    writer->AddObserver(vtkCommand::AnyEvent, eventCallbackVTK);

    vtksys_stl::stringstream ss2;
    ss2 << filePrefix << "_raw.vtp";

    writer->SetInputConnection(discreteCubes->GetOutputPort());
    writer->SetFileName(ss2.str().c_str());
    writer->Write();

    controller->Finalize();

    return EXIT_SUCCESS;
    }
