////program for vtkDiscreteMarchingCubes
//01: based on template.cxx and VTK/Examples/Medical/Cxx/GenerateModelsFromLabels.cxx




#include <vtkSmartPointer.h>
#include <vtkImageReader2Factory.h>
#include <vtkImageReader2.h>
#include "filters_mod/VTK/Filters/General/vtkDiscreteMarchingCubes.h"
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

    if (argc != 6){
        std::cerr << "Usage: " << argv[0]
                  << " input"
                  << " output"
                  << " compress"
                  << " StartLabel EndLabel"
                  << std::endl;
        return EXIT_FAILURE;
        }

    if(!(strcasestr(argv[2],".vtp"))) {
        std::cerr << "The output should end with .vtp" << std::endl;
        return -1;
        }

    unsigned int startLabel = atoi(argv[4]);
    if (startLabel > VTK_SHORT_MAX){
        std::cout << "ERROR: startLabel is larger than " << VTK_SHORT_MAX << std::endl;
        return EXIT_FAILURE;
        }

    unsigned int endLabel = atoi(argv[5]);
    if (endLabel > VTK_SHORT_MAX){
        std::cout << "ERROR: endLabel is larger than " << VTK_SHORT_MAX << std::endl;
        return EXIT_FAILURE;
        }


    VTK_CREATE(vtkCallbackCommand, eventCallbackVTK);
    eventCallbackVTK->SetCallback(FilterEventHandlerVTK);


    VTK_CREATE(vtkImageReader2Factory, readerFactory);
    vtkImageReader2* reader= readerFactory->CreateImageReader2(argv[1]);
    if(!reader){
        std::cerr << "Could not find an appropriate reader. Does file exist?" << std::endl;
        return -1;
        }
    reader->SetFileName(argv[1]);
    reader->AddObserver(vtkCommand::AnyEvent, eventCallbackVTK);
    reader->Update(); // does not throw an error if file cannot be read! Use CanReadFile if readerFactory ought to be avoided

    VTK_CREATE(vtkDiscreteMarchingCubes, filter);
    filter->SetInputConnection(0, reader->GetOutputPort());
    filter->GenerateValues(endLabel - startLabel + 1, startLabel, endLabel);
    filter->ComputeNeighboursOn(); // expecting own extension to vtkDiscreteMarchingCubes
    filter->AddObserver(vtkCommand::AnyEvent, eventCallbackVTK);
    filter->Update();
    std::cerr << "Verts: " << filter->GetOutput()->GetNumberOfPoints() << " Cells: " << filter->GetOutput()->GetNumberOfCells() << std::endl;

    VTK_CREATE(vtkXMLPolyDataWriter, writer);
    writer->SetInputConnection(filter->GetOutputPort());
    writer->SetFileName(argv[2]);
    writer->SetDataModeToBinary();//SetDataModeToAscii()//SetDataModeToAppended()
    if(atoi(argv[3]))
        writer->SetCompressorTypeToZLib();//default
    else
        writer->SetCompressorTypeToNone();
    writer->AddObserver(vtkCommand::AnyEvent, eventCallbackVTK);
    writer->Write();

    return EXIT_SUCCESS;
    }


