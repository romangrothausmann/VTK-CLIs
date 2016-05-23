////program to test streamed reading of a VTP-file
//01: based on template.cxx




#include <vtkSmartPointer.h>
#include <vtkXMLPolyDataReader.h>
#include <vtkInformation.h>
#include <vtkStreamingDemandDrivenPipeline.h>
#include <vtkPolyData.h>
#include <vtkWindowedSincPolyDataFilter.h>
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

    if (argc != 7){
        std::cerr << "Usage: " << argv[0]
                  << " input"
                  << " output"
                  << " NumberOfIterations NonManifoldSmoothing NormalizeCoordinates"
                  << " chunks"
                  << std::endl;
        return EXIT_FAILURE;
        }

    if(!(strcasestr(argv[1],".vtp"))) {
        std::cerr << "The input should end with .vtp" << std::endl;
        return -1;
        }

    if(!(strcasestr(argv[2],".vtp"))) {
        std::cerr << "The input should end with .vtp" << std::endl;
        return -1;
        }


    VTK_CREATE(vtkCallbackCommand, eventCallbackVTK);
    eventCallbackVTK->SetCallback(FilterEventHandlerVTK);


    VTK_CREATE(vtkXMLPolyDataReader, reader);
    reader->SetFileName(argv[1]);
    reader->AddObserver(vtkCommand::AnyEvent, eventCallbackVTK);
    reader->UpdateInformation();

    int numProcs= atoi(argv[6]);

    vtkInformation* outInfo = reader->GetOutputInformation(0);
    // Check if reader can handle piece requests (for unstructured) or sub-extents (for structured)
    if (!outInfo->Get(vtkAlgorithm::CAN_HANDLE_PIECE_REQUEST()) &&
	!outInfo->Get(vtkAlgorithm::CAN_PRODUCE_SUB_EXTENT())){
	std::cout << "Reader cannot stream data!" << std::endl;
	numProcs= 1;
	}
    // std::cout << "Pieces " << outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES()) << std::endl;


    VTK_CREATE(vtkWindowedSincPolyDataFilter, filter);
    filter->SetInputConnection(reader->GetOutputPort());
    filter->SetNumberOfIterations(atoi(argv[3]));
    filter->SetNonManifoldSmoothing(atoi(argv[4]));
    filter->SetNormalizeCoordinates(atoi(argv[5]));
    filter->AddObserver(vtkCommand::AnyEvent, eventCallbackVTK);

    VTK_CREATE(vtkXMLPolyDataWriter, writer);
    writer->SetInputConnection(filter->GetOutputPort());
    writer->SetFileName(argv[2]);
    writer->SetNumberOfPieces(numProcs);
    //writer->SetGhostLevel(atoi(argv[]));
    writer->SetDataModeToBinary();//SetDataModeToAscii()//SetDataModeToAppended()

    writer->AddObserver(vtkCommand::AnyEvent, eventCallbackVTK);
    writer->Write();
    
    return EXIT_SUCCESS;
    }


