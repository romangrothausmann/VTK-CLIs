////program to write a VTP-file as a multi-piece VTP-file (that can be streamed) using vtkKdTree (as in vtkDistributedDataFilter but no MPI dependence)
//01: based on vtp2multi-piece_vtp.cxx




#include <vtkSmartPointer.h>
#include <vtkXMLPolyDataReader.h>
#include <vtkKdTree.h>
#include <vtkExtractCells.h>
#include <vtkIdList.h>
#include <vtkDataSetSurfaceFilter.h>//faster version of vtkGeometryFilter
#include <vtkPieceScalars.h>
#include <vtkXMLPolyDataWriter.h>

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
    reader->Update();

    VTK_CREATE(vtkKdTree, d3);
    d3->SetNumberOfRegionsOrMore(atoi(argv[4]));
    //d3->SetMinCells(); //min number of cells per region
    //d3->BuildLocatorFromPoints(reader->GetOutput()->GetPoints());
    d3->SetDataSet(reader->GetOutput());
    //d3->AddObserver(vtkCommand::AnyEvent, eventCallbackVTK); //odd progress reports
    d3->BuildLocator();
    d3->CreateCellLists(); //needs BuildLocator() before!

    int numPieces= d3->GetNumberOfRegions();
    std::cerr << "GetNumberOfRegions: " << numPieces << std::endl;

    VTK_CREATE(vtkExtractCells, filter);
    //filter->SetInputConnection(reader->GetOutputPort()); //causes pipeline update to propagate up to obbd
    filter->SetInputData(reader->GetOutput()); //avoids pipeline update to propaga
    filter->AddObserver(vtkCommand::AnyEvent, eventCallbackVTK);
    // filter->Update(); //this is called by writer

    VTK_CREATE(vtkDataSetSurfaceFilter, usg2pd);
    usg2pd->SetInputConnection(filter->GetOutputPort());
    usg2pd->AddObserver(vtkCommand::AnyEvent, eventCallbackVTK);

    VTK_CREATE(vtkPieceScalars, ps);
    ps->SetInputConnection(usg2pd->GetOutputPort());
    ps->SetScalarModeToPointData();// pointData can be rendered much faster in paraview
    ps->AddObserver(vtkCommand::AnyEvent, eventCallbackVTK);

    VTK_CREATE(vtkXMLPolyDataWriter, writer);
    writer->SetInputConnection(ps->GetOutputPort());
    writer->SetFileName(argv[2]);
    writer->SetNumberOfPieces(numPieces);
    //writer->SetGhostLevel(atoi(argv[5]));
    writer->SetDataModeToBinary();//SetDataModeToAscii()//SetDataModeToAppended()
    if(atoi(argv[3]))
        writer->SetCompressorTypeToZLib();//default
    else
        writer->SetCompressorTypeToNone();
    writer->AddObserver(vtkCommand::AnyEvent, eventCallbackVTK);

    for(int i= 0; i < numPieces; i++){
	
	VTK_CREATE(vtkIdList, cellsIds);
	cellsIds= d3->GetCellList(i); //needs CreateCellLists() before!

	if(cellsIds){
	    filter->SetCellList(cellsIds);
	    writer->SetWritePiece(i);
	    writer->SetDataModeToAppended();
	    writer->Write();
	    d3->PrintRegion(i);
	    std::cerr << "Wrote piece: " << +i+1 << "; cells: " << cellsIds->GetNumberOfIds() << std::endl;
	    }
	}

    return EXIT_SUCCESS;
    }


