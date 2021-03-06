// program to apply vtkDiscreteMarchingCubes
//01: based on discret_marching_cubes05.cxx
//deprecated, use mc_discrete + smooth-ws



#include <vtkSmartPointer.h>
#include <vtkMetaImageReader.h>
#include <vtkInformation.h>//for GetOutputInformation
#include <vtkStreamingDemandDrivenPipeline.h>//for extent
#include <vtkImageData.h>//for GetExtent()
#include <vtkImageConstantPad.h>
#include "filters_mod/VTK/Filters/General/vtkDiscreteMarchingCubes.h"
#include <vtkWindowedSincPolyDataFilter.h>
#include <vtkXMLPolyDataWriter.h>
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

    if (argc != 6){
        std::cerr << "Usage: " << argv[0]
                  << " input"
                  << " output"
                  << " StartLabel EndLabel"
                  << " cap-surface"
                  << std::endl;
        return EXIT_FAILURE;
        }

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
    vtkSmartPointer<vtkXMLPolyDataWriter> writer =
        vtkSmartPointer<vtkXMLPolyDataWriter>::New();

    vtkSmartPointer<vtkCallbackCommand> eventCallbackVTK = vtkSmartPointer<vtkCallbackCommand>::New();
    eventCallbackVTK->SetCallback(FilterEventHandlerVTK);


    // Define all of the variables
    unsigned int startLabel = atoi(argv[3]);
    unsigned int endLabel = atoi(argv[4]);
    vtkstd::string filePrefix = argv[2]; //"Label";
    unsigned int smoothingIterations = 20;

    reader->SetFileName(argv[1]);
    reader->AddObserver(vtkCommand::AnyEvent, eventCallbackVTK);
    reader->Update();

    if(atoi(argv[5])){
        int extent[6];
        reader->GetOutputInformation(0)->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), extent);

        fprintf(stderr, "extent: %d, %d, %d, %d, %d, %d\n", extent[0], extent[1], extent[2], extent[3], extent[4], extent[5]);

        pad->SetInputConnection(reader->GetOutputPort());
        pad->SetOutputWholeExtent(
            extent[0] -1, extent[1] + 1,
            extent[2] -1, extent[3] + 1,
            extent[4] -1, extent[5] + 1);
        pad->AddObserver(vtkCommand::AnyEvent, eventCallbackVTK);
        pad->Update();
        discreteCubes->SetInputConnection(pad->GetOutputPort());
        }
    else
        discreteCubes->SetInputConnection(reader->GetOutputPort());

    discreteCubes->GenerateValues(
        endLabel - startLabel + 1, startLabel, endLabel);
    discreteCubes->ComputeNeighboursOn();//expecting own extension to vtkDiscreteMarchingCubes
    discreteCubes->AddObserver(vtkCommand::AnyEvent, eventCallbackVTK);
    discreteCubes->Update();

    std::cerr << "Verts: " << discreteCubes->GetOutput()->GetNumberOfPoints() << " Cells: " << discreteCubes->GetOutput()->GetNumberOfCells() << std::endl;

    writer->AddObserver(vtkCommand::AnyEvent, eventCallbackVTK);

    std::stringstream ss2;
    ss2 << filePrefix << "_raw.vtp";

    writer->SetInputConnection(discreteCubes->GetOutputPort());
    writer->SetFileName(ss2.str().c_str());
    writer->Write();


    smoother->SetInputConnection(discreteCubes->GetOutputPort());
    smoother->SetNumberOfIterations(smoothingIterations);
    smoother->NonManifoldSmoothingOn();
    smoother->NormalizeCoordinatesOn();
    smoother->AddObserver(vtkCommand::AnyEvent, eventCallbackVTK);
    smoother->Update();

    std::stringstream ss;
    ss << filePrefix << "_sws.vtp";

    writer->SetInputConnection(smoother->GetOutputPort());
    writer->SetFileName(ss.str().c_str());
    writer->Write();


    ///check if surface is closed
    std::cout << "Checking if surface is closed..." << std::endl;
    featureEdges->FeatureEdgesOff();
    featureEdges->BoundaryEdgesOn();
    featureEdges->NonManifoldEdgesOff();
    featureEdges->SetInputConnection(discreteCubes->GetOutputPort());
    featureEdges->AddObserver(vtkCommand::AnyEvent, eventCallbackVTK);
    featureEdges->Update();

    int numberOfEdges = featureEdges->GetOutput()->GetNumberOfCells();
    if(numberOfEdges > 0)
        std::cout << "Surface is not closed. # open edges: " << numberOfEdges << std::endl;
    else
        std::cout << "Surface is closed. # open edges: " << numberOfEdges << std::endl;

    ///check if surface has non-manifold edges
    std::cout << "Checking if surface has non-manifold edges..." << std::endl;
    featureEdges->BoundaryEdgesOff();
    featureEdges->NonManifoldEdgesOn();
    featureEdges->Update();

    numberOfEdges = featureEdges->GetOutput()->GetNumberOfCells();
    if(numberOfEdges > 0)
        std::cout << "Surface has non-manifold edges. # of non-manifold edges: " << numberOfEdges << std::endl;
    else
        std::cout << "Surface has only manifold edges. # of non-manifold edges: " << numberOfEdges << std::endl;

    return EXIT_SUCCESS;
    }
