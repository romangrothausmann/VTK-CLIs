////program to test streamed reading of a VTI-file
//01: based on template.cxx




#include <vtkSmartPointer.h>
#include <vtkXMLImageDataReader.h>
#include <vtkStreamingDemandDrivenPipeline.h>
#include <vtkImageData.h>

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
        return EXIT_FAILURE;
        }

    if(!(strcasestr(argv[1],".vti"))) {
        std::cerr << "The input should end with .vti" << std::endl;
        return -1;
        }


    VTK_CREATE(vtkCallbackCommand, eventCallbackVTK);
    eventCallbackVTK->SetCallback(FilterEventHandlerVTK);


    VTK_CREATE(vtkXMLImageDataReader, reader);
    reader->SetFileName(argv[1]);
    reader->AddObserver(vtkCommand::AnyEvent, eventCallbackVTK);
    reader->UpdateInformation();

    int numProcs= atoi(argv[2]);
    vtkStreamingDemandDrivenPipeline* exec= vtkStreamingDemandDrivenPipeline::SafeDownCast(reader->GetExecutive());
    // exec->SetUpdateNumberOfPieces(exec->GetOutputInformation(0), numProcs);
    if (!exec){ // || exec->GetMaximumNumberOfPieces() < numProcs){
        std::cerr << "Streaming not possible!" << std::endl;
        return -1;
        }

    unsigned long  totalPolyData= 0;
    for(int myId= 0; myId < numProcs; myId++){
        // reader->SetUpdateExtent(0, myId, numProcs, 0);
        // reader->Update();

        // exec->SetUpdatePiece(exec->GetOutputInformation(0), myId);
        // exec->Update();

        exec->SetUpdateExtent(0, myId, numProcs, 0);
        reader->Update();
        //exec->Update();

        unsigned long  subtotalPolyData= reader->GetOutput()->GetNumberOfCells();
        totalPolyData+= subtotalPolyData;

        fprintf(stderr, "%5.1f%% (%d/%d): sub-total cells: %ld; accumulated total cells: %ld\n", myId*100.0/numProcs, myId, numProcs, subtotalPolyData, totalPolyData);
        }

    std::cout << "Total mesh cells: " << totalPolyData << std::endl;

    return EXIT_SUCCESS;
    }


