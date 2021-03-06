// program to apply vtkDiscreteMarchingCubes
//01: based on discrete_marching-cubes.cxx, ParallelCompositeTime/Parallel/parallel1_answer.cxx (kitware VTK-workshop), IO/MPIImage/Testing/Cxx/ParallelIso.cxx, Examples/ParallelProcessing/Generic/Cxx/ParallelIso.cxx and Examples/Infovis/Cxx/ParallelBFS.cxx



#include <vtkMPIController.h>
#include <vtkStreamingDemandDrivenPipeline.h>

#include <vtkSmartPointer.h>
#include <vtkXMLImageDataReader.h>//seems to be the only reader that works, has outInfo->Set(CAN_PRODUCE_SUB_EXTENT(), 1)  neither vtkMetaImageReader nor vtkPNrrdReader worked, vtkMPIImageReader (RAW) not tested
#include <vtkInformation.h>//for GetOutputInformation
#include <vtkStreamingDemandDrivenPipeline.h>//for extent
#include <vtkImageData.h>//for GetExtent()
#include <vtkImageConstantPad.h>
#include "filters_mod/VTK/Filters/General/vtkDiscreteMarchingCubes.h"//does not handle sub-extents of pvtp correctly, nor does vtkMarchingCubes but vtkContourFilter does (seems to be independent of vtkMarchingCubes)
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
    vtkSmartPointer<vtkXMLImageDataReader> reader =
        vtkSmartPointer<vtkXMLImageDataReader>::New();
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
    reader->AddObserver(vtkCommand::AnyEvent, eventCallbackVTK);

    if(atoi(argv[5])){
        int extent[6];
        reader->GetOutputInformation(0)->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), extent);

        fprintf(stderr, "extent: %d, %d, %d, %d, %d, %d\n", extent[0], extent[1], extent[2], extent[3], extent[4], extent[5]);

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
    discreteCubes->ComputeNeighboursOn();//expecting own extension to vtkDiscreteMarchingCubes
    discreteCubes->UpdateInformation();
    discreteCubes->SetUpdateExtent(0, myId, numProcs, 0);
    discreteCubes->AddObserver(vtkCommand::AnyEvent, eventCallbackVTK);
    discreteCubes->Update();//essential to SetUpdateExtent before on same filter

    vtkIdType numCells = discreteCubes->GetOutput()->GetNumberOfCells();
    vtkIdType numPoints = discreteCubes->GetOutput()->GetNumberOfPoints();
    cerr << "Process (" << myId << "): "
	 << "# points: " << numPoints
         << "# cells: " << numCells << endl;

    writer->SetNumberOfPieces(numProcs);
    writer->SetStartPiece(myId);
    writer->SetEndPiece(myId);
    writer->AddObserver(vtkCommand::AnyEvent, eventCallbackVTK);

    std::stringstream ss2;
    ss2 << filePrefix << "_raw.pvtp";

    writer->SetInputConnection(discreteCubes->GetOutputPort());
    writer->SetFileName(ss2.str().c_str());
    writer->Write();

    smoother->SetInputConnection(discreteCubes->GetOutputPort());
    smoother->SetNumberOfIterations(smoothingIterations);
    smoother->NonManifoldSmoothingOn();
    smoother->NormalizeCoordinatesOn();
    smoother->UpdateInformation();
    smoother->SetUpdateExtent(0, myId, numProcs, 0);
    smoother->AddObserver(vtkCommand::AnyEvent, eventCallbackVTK);
    smoother->Update();//essential to SetUpdateExtent before on same filter

    numCells = smoother->GetOutput()->GetNumberOfCells();
    numPoints = smoother->GetOutput()->GetNumberOfPoints();
    cerr << "Process (" << myId << "): "
	 << "# points: " << numPoints
         << "# cells: " << numCells << endl;

    std::stringstream ss;
    ss << filePrefix << "_sws.pvtp";

    writer->SetInputConnection(smoother->GetOutputPort());
    writer->SetFileName(ss.str().c_str());
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
