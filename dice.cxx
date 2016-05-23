////program to write a VTP-file as a multi-piece VTP-file (that can be streamed) using vtkOBBDicer
//01: based on vtp2multi-piece_vtp.cxx




#include <vtkSmartPointer.h>
#include <vtkXMLPolyDataReader.h>
#include <vtkOBBDicer.h>
#include <vtkThreshold.h>
#include <vtkDataObject.h>
#include <vtkDataSetAttributes.h>
#include <vtkDataSetSurfaceFilter.h> //faster version of vtkGeometryFilter
#include <vtkPieceScalars.h>
#include <vtkXMLPolyDataWriter.h>
#include <vtkUnstructuredGrid.h>

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

    if (argc != 5){
        std::cerr << "Usage: " << argv[0]
                  << " input"
                  << " output"
                  << " compress"
                  << " #pieces"
                  << std::endl;
        return EXIT_FAILURE;
        }

    if(!(strcasestr(argv[1],".vtp"))) {
        std::cerr << "The input should end with .vtp" << std::endl;
        return -1;
        }

    if(!(strcasestr(argv[2],".vtp"))) {
        std::cerr << "The output should end with .vtp" << std::endl;
        return -1;
        }


    VTK_CREATE(vtkCallbackCommand, eventCallbackVTK);
    eventCallbackVTK->SetCallback(FilterEventHandlerVTK);


    VTK_CREATE(vtkXMLPolyDataReader, reader);
    reader->SetFileName(argv[1]);
    reader->AddObserver(vtkCommand::AnyEvent, eventCallbackVTK);

    VTK_CREATE(vtkOBBDicer, obbd);
    obbd->SetInputConnection(0, reader->GetOutputPort());
    obbd->SetDiceModeToSpecifiedNumberOfPieces();
    obbd->SetNumberOfPieces(atoi(argv[4]));
    obbd->AddObserver(vtkCommand::AnyEvent, eventCallbackVTK);
    obbd->Update();

    int numPieces= obbd->GetNumberOfActualPieces();
    std::cerr << "GetNumberOfActualPieces: " << numPieces << std::endl;

    VTK_CREATE(vtkThreshold, filter);
    //filter->SetInputConnection(obbd->GetOutputPort()); //causes pipeline update to propagate up to obbd
    filter->SetInputData(obbd->GetOutput()); //avoids pipeline update to propagate up to obbd
    filter->SetInputArrayToProcess(0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS, vtkDataSetAttributes::SCALARS);
    filter->AllScalarsOff();
    filter->AddObserver(vtkCommand::AnyEvent, eventCallbackVTK);
    // filter->Update(); //this is called by writer

    VTK_CREATE(vtkDataSetSurfaceFilter, vtu2vtp);
    vtu2vtp->SetInputConnection(filter->GetOutputPort());
    vtu2vtp->AddObserver(vtkCommand::AnyEvent, eventCallbackVTK);

    VTK_CREATE(vtkPieceScalars, ps);
    ps->SetInputConnection(vtu2vtp->GetOutputPort());
    ps->SetScalarModeToPointData();// pointData can be rendered much faster in paraview
    ps->AddObserver(vtkCommand::AnyEvent, eventCallbackVTK);

    VTK_CREATE(vtkXMLPolyDataWriter, writer);
    writer->SetInputConnection(ps->GetOutputPort());
    writer->SetFileName(argv[2]);
    writer->SetNumberOfPieces(numPieces);
    //writer->SetGhostLevel(atoi(argv[5]));
    writer->SetDataModeToAppended();
    writer->SetEncodeAppendedData(0);
    if(atoi(argv[3]))
        writer->SetCompressorTypeToZLib();//default
    else
        writer->SetCompressorTypeToNone();
    writer->AddObserver(vtkCommand::AnyEvent, eventCallbackVTK);

    for(int i= 0; i < numPieces; i++){
	filter->ThresholdBetween(i,i);
	writer->SetWritePiece(i);
	writer->SetDataModeToAppended();
	// std::stringstream sss;
	// sss.str(""); sss << argv[2] << i << ".vtp";
	// writer->SetFileName(sss.str().c_str());
	writer->Write(); //overwrites former piece, even with SetDataModeToAppended
	
	std::cout << "# cells: " << filter->GetOutput()->GetNumberOfCells() << std::endl;
        std::cerr << "Wrote piece: " << +i+1 << std::endl;
	}

    return EXIT_SUCCESS;
    }


