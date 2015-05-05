// program to apply vtkDiscreteMarchingCubes
//01: based on discrete_marching-cubes.cxx, Examples/ParallelProcessing/Generic/Cxx/ParallelIso.cxx and Examples/Infovis/Cxx/ParallelBFS.cxx



#include <mpi.h>
#include <vtkMPIController.h>
#include <vtkStreamingDemandDrivenPipeline.h>

#include <vtkSmartPointer.h>
#include <vtkMetaImageReader.h>
#include <vtkImageData.h>//for GetExtent()
#include <vtkImageConstantPad.h>
#include <vtkDiscreteMarchingCubes.h>
#include <vtkWindowedSincPolyDataFilter.h>
#include <vtkXMLPPolyDataWriter.h>
#include <vtkFeatureEdges.h>

#include <vtkCallbackCommand.h>
#include <vtkCommand.h>


struct ParallelArgs{
  int argc;
  char** argv;
};

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


void processMPI(vtkMultiProcessController* controller, void* arg){
    int argc; char **argv;
    int myId, numProcs;
 
    ParallelArgs* args = reinterpret_cast<ParallelArgs*>(arg);
    argc= args->argc;
    argv= args->argv;

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

    if(atoi(argv[5])){
	int *extent = reader->GetOutput()->GetExtent(); //needs vtkImageData.h
	pad->SetInputConnection(reader->GetOutputPort());
	pad->SetOutputWholeExtent(
	    extent[0] -1, extent[1] + 1,
	    extent[2] -1, extent[3] + 1,
	    extent[4] -1, extent[5] + 1);
	discreteCubes->SetInputConnection(pad->GetOutputPort());
	}
    else
	discreteCubes->SetInputConnection(reader->GetOutputPort());

    discreteCubes->GenerateValues(
	endLabel - startLabel + 1, startLabel, endLabel);

    smoother->SetInputConnection(discreteCubes->GetOutputPort());
    smoother->SetNumberOfIterations(smoothingIterations);
    smoother->NonManifoldSmoothingOn();
    smoother->NormalizeCoordinatesOn();
    smoother->SetUpdateExtent(0, myId, numProcs, 0);

    // // Tell the pipeline which piece we want to update.
    // vtkStreamingDemandDrivenPipeline* exec =
    // 	vtkStreamingDemandDrivenPipeline::SafeDownCast(smoother->GetExecutive());
    // exec->SetUpdateNumberOfPieces(exec->GetOutputInformation(0), numProcs);
    // exec->SetUpdatePiece(exec->GetOutputInformation(0), myId);

    smoother->Update();

    writer->SetNumberOfPieces(numProcs);
    writer->SetStartPiece(myId);
    writer->SetEndPiece(myId);
    writer->AddObserver(vtkCommand::AnyEvent, eventCallbackVTK);

    vtksys_stl::stringstream ss2;
    ss2 << filePrefix << "_raw.vtp";

    writer->SetInputConnection(discreteCubes->GetOutputPort());
    writer->SetFileName(ss2.str().c_str());
    writer->Write();

    vtksys_stl::stringstream ss;
    ss << filePrefix << "_sws.vtp";

    writer->SetInputConnection(smoother->GetOutputPort());
    writer->SetFileName(ss.str().c_str());
    writer->Write();

    }


int main (int argc, char *argv[]){
 
    if (argc != 6){
	std::cerr << "Usage: " << argv[0]
		  << " input"
		  << " output"
		  << " StartLabel EndLabel"
		  << " cap-surface"
		  << std::endl;
	return EXIT_FAILURE;
	}

    // This is here to avoid false leak messages from vtkDebugLeaks when
    // using mpich. It appears that the root process which spawns all the
    // main processes waits in MPI_Init() and calls exit() when
    // the others are done, causing apparent memory leaks for any objects
    // created before MPI_Init().
    MPI_Init(&argc, &argv);

    vtkSmartPointer<vtkMPIController> controller =
	vtkSmartPointer<vtkMPIController>::New();
    controller->Initialize(&argc, &argv, 1);

    ParallelArgs args;
    args.argc = argc;
    args.argv = argv;

    controller->SetSingleMethod(processMPI, &args);
    controller->SingleMethodExecute();

    controller->Finalize();
    controller->Delete();

    return EXIT_SUCCESS;
    }

