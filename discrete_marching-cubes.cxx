// program to apply vtkDiscreteMarchingCubes
//01: based on discret_marching_cubes05.cxx




#include <vtkMetaImageReader.h>
#include <vtkImageAccumulate.h>
#include <vtkDiscreteMarchingCubes.h>
#include <vtkWindowedSincPolyDataFilter.h>
#include <vtkThreshold.h>
#include <vtkGeometryFilter.h>
#include <vtkXMLPolyDataWriter.h>
#include <vtkSmartPointer.h>
#include <vtkImageConstantPad.h>
#include <vtkMetaImageWriter.h>
#include <vtkFeatureEdges.h>
#include <vtkAppendPolyData.h>

#include <vtkCallbackCommand.h>
#include <vtkCommand.h>

#include <vtkImageData.h>
#include <vtkPointData.h>
#include <vtkUnstructuredGrid.h>
#include <vtksys/ios/sstream>


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
	int *extent = reader->GetOutput()->GetExtent();
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
    discreteCubes->AddObserver(vtkCommand::AnyEvent, eventCallbackVTK);
    discreteCubes->Update();

    writer->SetInputConnection(discreteCubes->GetOutputPort());

    vtksys_stl::stringstream ss2;
    ss2 << filePrefix << "_raw.vtp";

    writer->SetFileName(ss2.str().c_str());
    writer->AddObserver(vtkCommand::AnyEvent, eventCallbackVTK);
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

    smoother->SetInputConnection(discreteCubes->GetOutputPort());
    smoother->SetNumberOfIterations(smoothingIterations);
    smoother->NonManifoldSmoothingOn();
    smoother->NormalizeCoordinatesOn();
    smoother->AddObserver(vtkCommand::AnyEvent, eventCallbackVTK);
    smoother->Update();

    vtksys_stl::stringstream ss;
    ss << filePrefix << "_sws.vtp";

    writer->SetInputConnection(smoother->GetOutputPort());
    writer->SetFileName(ss.str().c_str());
    writer->Write();

    return EXIT_SUCCESS;
    }
