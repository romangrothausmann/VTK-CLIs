////program for vtkQuadricClustering
///point data only preserved with restrict-output-verts-to-input-verts (SetUseInputPoints)
//01: based on template.cxx




#include <vtkSmartPointer.h>
#include <vtkXMLPolyDataReader.h>
#include <vtkTriangleFilter.h>
#include <vtkQuadricClustering.h>
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

    if (argc != 8){
        std::cerr << "Usage: " << argv[0]
                  << " input"
                  << " output"
                  << " compress"
                  << " X-extent_devisions"
                  << " Y-extent_devisions"
                  << " Z-extent_devisions"
                  << " restrict-output-verts-to-input-verts"
                  << std::endl;
        std::cerr << std::endl
		  << "point data only preserved with restrict-output-verts-to-input-verts" 
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

    VTK_CREATE(vtkTriangleFilter, triangulate);
    triangulate->SetInputConnection(reader->GetOutputPort());

    VTK_CREATE(vtkQuadricClustering, filter);
    filter->SetInputConnection(0, triangulate->GetOutputPort());
    filter->SetNumberOfXDivisions(atoi(argv[4]));
    filter->SetNumberOfYDivisions(atoi(argv[5]));
    filter->SetNumberOfZDivisions(atoi(argv[6]));
    filter->SetUseInputPoints(atoi(argv[7]));
    filter->CopyCellDataOff();
    filter->UseFeatureEdgesOff();
    filter->UseFeaturePointsOff();
    filter->UseInternalTrianglesOn();
    filter->PreventDuplicateCellsOn();
    filter->AddObserver(vtkCommand::AnyEvent, eventCallbackVTK);
    filter->Update();

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


