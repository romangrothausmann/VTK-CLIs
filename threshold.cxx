////program for vtkThreshold
//01: based on template.cxx




#include <vtkSmartPointer.h>
#include <vtkDataObject.h>
#include <vtkDataSetAttributes.h>
#include <vtkXMLPolyDataReader.h>
#include <vtkThreshold.h>
#include <vtkGeometryFilter.h>
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

    if (argc != 9){
        std::cerr << "Usage: " << argv[0]
                  << " input"
                  << " output"
                  << " compress"
                  << " field attribute start end"
                  << " AllScalars"
                  << std::endl;

        std::cerr << "vtkDataObject field associations:" << std::endl;
        for (int i=0; i<vtkDataObject::NUMBER_OF_ASSOCIATIONS; i++)
            fprintf(stderr, "%d: %s\n", i, vtkDataObject::GetAssociationTypeAsString(i));

        std::cerr << "vtkDataSet attributes:" << std::endl;
        for (int i=0; i<vtkDataSetAttributes::NUM_ATTRIBUTES; i++)
            fprintf(stderr, "%d: %s\n", i, vtkDataSetAttributes::GetLongAttributeTypeAsString(i));

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

    std::cerr << "Verts: " << +reader->GetOutput()->GetNumberOfPoints() << " Cells: " << +reader->GetOutput()->GetNumberOfCells() << std::endl;

    VTK_CREATE(vtkThreshold, filter);
    filter->SetInputConnection(0, reader->GetOutputPort());
    filter->SetInputArrayToProcess(0, 0, 0, atoi(argv[4]), atoi(argv[5]));
    filter->ThresholdBetween(atof(argv[6]), atof(argv[7]));
    filter->SetAllScalars(atoi(argv[8]));
    filter->AddObserver(vtkCommand::AnyEvent, eventCallbackVTK);
    filter->Update();

    VTK_CREATE(vtkGeometryFilter, vtu2vtp);
    vtu2vtp->SetInputConnection(filter->GetOutputPort());
    vtu2vtp->AddObserver(vtkCommand::AnyEvent, eventCallbackVTK);
    vtu2vtp->Update();

    std::cerr << "Verts: " << +vtu2vtp->GetOutput()->GetNumberOfPoints() << " Cells: " << +vtu2vtp->GetOutput()->GetNumberOfCells() << std::endl;

    VTK_CREATE(vtkXMLPolyDataWriter, writer);
    writer->SetInputConnection(vtu2vtp->GetOutputPort());
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


