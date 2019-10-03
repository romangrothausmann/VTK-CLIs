////program to export point/cell/field data to CSVs
//01: based on template.cxx




#include <vtkSmartPointer.h>
#include <vtkXMLPolyDataReader.h>
#include <vtkPolyData.h>
#include <vtkFieldData.h>
#include <vtkDataObjectToTable.h>
#include <vtkTable.h>
#include <vtkDelimitedTextWriter.h>

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

    if (argc != 4){
        std::cerr << "Usage: " << argv[0]
                  << " input"
                  << " output"
                  << " FieldType"
                  << std::endl;
        return EXIT_FAILURE;
        }

    if(!(strcasestr(argv[1],".vtp"))) {
        std::cerr << "The input should end with .vtp" << std::endl;
        return -1;
        }

    if(!(strcasestr(argv[2],".csv"))) {
        std::cerr << "The output should end with .csv" << std::endl;
        return -1;
        }


    VTK_CREATE(vtkCallbackCommand, eventCallbackVTK);
    eventCallbackVTK->SetCallback(FilterEventHandlerVTK);


    VTK_CREATE(vtkXMLPolyDataReader, reader);
    reader->SetFileName(argv[1]);
    reader->AddObserver(vtkCommand::AnyEvent, eventCallbackVTK);
    reader->Update();

    //// determin longest array
    vtkFieldData* fd= reader->GetOutput()->GetFieldData();
    int noa= fd->GetNumberOfArrays();
    vtkIdType maxNOT= 0;
    for(int i= 0; i < noa; i++){
	maxNOT= fd->GetArray(i)->GetNumberOfTuples() > maxNOT ? fd->GetArray(i)->GetNumberOfTuples() : maxNOT;
	}
    std::cerr << "maxNOT: " << maxNOT << std::endl;

    //// set length of any array to longest found
    /// works, but results are only correct for arrays whose NOT was not changed
    for(int i= 0; i < noa; i++){
	fd->GetArray(i)->SetNumberOfTuples(maxNOT);
	}

    VTK_CREATE(vtkDataObjectToTable, filter);
    filter->SetInputData(0, reader->GetOutput());
    filter->AddObserver(vtkCommand::AnyEvent, eventCallbackVTK);
    filter->SetFieldType(atoi(argv[3])); // vtkDataObjectToTable::FIELD_DATA
    filter->Update();

    // nice but bad for large datasets: filter->GetOutput()->Dump();
    std::cerr << "Rows: " << filter->GetOutput()->GetNumberOfRows() << std::endl;

    VTK_CREATE(vtkDelimitedTextWriter, writer);
    writer->SetInputConnection(filter->GetOutputPort());
    writer->SetFileName(argv[2]);
    writer->AddObserver(vtkCommand::AnyEvent, eventCallbackVTK);
    writer->Write();

    return EXIT_SUCCESS;
    }


