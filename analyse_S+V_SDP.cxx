////program to calculate the surface of a (labled) mesh
//01: based on template.cxx
//02: changed to vtkMassProperties, works even for open surfaces (code for surface area seems not to relate on closeness assumption); slightly slower than former version based on vtkMeshQuality


#include <vtkSmartPointer.h>
#include <vtkXMLPolyDataReader.h>
#include <vtkInformation.h>
#include <vtkTriangleFilter.h>
#include <vtkThreshold.h>
#include <vtkGeometryFilter.h>
#include <vtkCellData.h>//GetCellData()
#include <vtkDataArray.h>//GetScalars()
#include <vtkDataSetAttributes.h>//GetRange()
#include <vtkMassProperties.h>

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

    if (argc != 3){
        std::cerr << "Usage: " << argv[0]
                  << " input"
                  << " chunks"
		  << std::endl;
        std::cerr << "Value calculation based on the discrete form of the divergence theorem." << std::endl;
        std::cerr << "Apart from the surface area, the calculations assume a closed mesh!" << std::endl;
        return EXIT_FAILURE;
        }

    if(!(strcasestr(argv[1],".vtp"))) {
        std::cerr << "The input should end with .vtp" << std::endl;
        return -1;
        }


    VTK_CREATE(vtkCallbackCommand, eventCallbackVTK);
    eventCallbackVTK->SetCallback(FilterEventHandlerVTK);


    VTK_CREATE(vtkXMLPolyDataReader, reader);
    reader->SetFileName(argv[1]);
    reader->AddObserver(vtkCommand::AnyEvent, eventCallbackVTK);
    reader->UpdateInformation();

    int numPieces= atoi(argv[2]);

    vtkInformation* outInfo = reader->GetOutputInformation(0);
    // Check if reader can handle piece requests (for unstructured) or sub-extents (for structured)
    if (!outInfo->Get(vtkAlgorithm::CAN_HANDLE_PIECE_REQUEST()) &&
	!outInfo->Get(vtkAlgorithm::CAN_PRODUCE_SUB_EXTENT())){
	std::cout << "Reader cannot stream data!" << std::endl;
	numPieces= 1;
	}

    VTK_CREATE(vtkTriangleFilter, triangle);
    triangle->SetInputConnection(reader->GetOutputPort());
    triangle->AddObserver(vtkCommand::AnyEvent, eventCallbackVTK);
    triangle->Update();

    VTK_CREATE(vtkThreshold, thr);
    thr->SetInputConnection(0, triangle->GetOutputPort());
    thr->SetInputArrayToProcess(0, 0, 0,         
	vtkDataObject::FIELD_ASSOCIATION_CELLS,
        vtkDataSetAttributes::SCALARS);
    thr->AddObserver(vtkCommand::AnyEvent, eventCallbackVTK);

    VTK_CREATE(vtkGeometryFilter, vtu2vtp);
    vtu2vtp->SetInputConnection(thr->GetOutputPort());
    vtu2vtp->AddObserver(vtkCommand::AnyEvent, eventCallbackVTK);

    VTK_CREATE(vtkMassProperties, filter);
    filter->SetInputConnection(vtu2vtp->GetOutputPort());
    filter->AddObserver(vtkCommand::AnyEvent, eventCallbackVTK);

    // filter->UpdateInformation();
    // outInfo = filter->GetOutputInformation(0);
    // if (!outInfo->Get(vtkAlgorithm::CAN_HANDLE_PIECE_REQUEST()) &&
    // 	!outInfo->Get(vtkAlgorithm::CAN_PRODUCE_SUB_EXTENT())){
    // 	std::cout << "vtkMassProperties cannot stream data!" << std::endl;
    // 	numPieces= 1;
    // 	}

    
    std::cout << "#index\tV\tS\tnSI" << std::endl;

    int iRange[2];
    vtkDataArray* tmpArray= reader->GetOutput()->GetCellData()->GetScalars();
    if(tmpArray){
	double dRange[2];
	tmpArray->GetRange(dRange);
	iRange[0]= (int)dRange[0];
	iRange[1]= (int)dRange[1];

	fprintf(stderr, "range of cell scalars: %d - %d\n", iRange[0], iRange[1]);
	}
    if(iRange[1]-iRange[0]){
	for (unsigned int i= iRange[0]; i <= iRange[1]; i++){
	    ////ToDo: insert check cell array if any scalar of i exist

	    thr->ThresholdBetween(i, i);

	    double tV= 0;
	    double tS= 0;
	    double tI= 0;
	    for(int myId= 0; myId < numPieces; myId++){
		filter->SetUpdateExtent(0, myId, numPieces, 0);
		filter->Update();

		tV+= filter->GetVolume();
		tS+= filter->GetSurfaceArea();
		tI+= filter->GetNormalizedShapeIndex();

		fprintf(stderr, "%5.1f%% (%d/%d): V: %f; S: %f\n", (myId+1)*100.0/numPieces, myId+1, numPieces, tV, tS);
		}

	    std::cout
		<< i << "\t"
		<< tV << "\t"
		<< tS << "\t"
		<< tI
		<< std::endl;
	    }
	}
    else{
	filter->SetInputConnection(triangle->GetOutputPort());

	    double tV= 0;
	    double tS= 0;
	    double tI= 0;
	for(int myId= 0; myId < numPieces; myId++){
	    filter->SetUpdateExtent(0, myId, numPieces, 0);
	    filter->Update();

	    tV+= filter->GetVolume();
	    tS+= filter->GetSurfaceArea();
	    tI+= filter->GetNormalizedShapeIndex();
	    }

	std::cout
	    << "all" << "\t"
	    << tV << "\t"
	    << tS << "\t"
	    << tI
	    << std::endl;
	}

    return EXIT_SUCCESS;
    }


