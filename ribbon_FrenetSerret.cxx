////program to create a ribbon of a path using vtkFrenetSerretFrame
//01: based on template.cxx




#include <vtkSmartPointer.h>
#include <vtkXMLPolyDataReader.h>
#include "vtkFrenetSerretFrame.h"
#include <vtkRibbonFilter.h>
#include <vtkPointData.h>
#include <vtkFloatArray.h>
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
                  << " width"
                  << " angle"
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

    VTK_CREATE(vtkFrenetSerretFrame, FSFrame);
    FSFrame->SetInputConnection(0, reader->GetOutputPort());
    FSFrame->ComputeBinormalOff();
    // FSFrame->SetViewUp();

    VTK_CREATE(vtkRibbonFilter, filter);
    filter->SetInputConnection(0, FSFrame->GetOutputPort());
    filter->SetInputArrayToProcess(1, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS, "FSNormals");//normals are expected on index 1, (index 0 is for scalars) l 138 vtkRibbonFilter.cxx
    filter->SetWidth(atof(argv[4]));
    filter->SetAngle(atof(argv[5]));
    filter->SetGenerateTCoordsToNormalizedLength();//normalized: s in [0;1] (same as t below)
    filter->AddObserver(vtkCommand::AnyEvent, eventCallbackVTK);
    filter->Update();

    vtkFloatArray* TCoords= vtkFloatArray::SafeDownCast(filter->GetOutput()->GetPointData()->GetTCoords());

    if(!TCoords){
	std::cerr << "TCoords missing!" << std::endl;
        return EXIT_FAILURE;
        }

    //// vtkRibbonFilter only produces s-coords, as points alternate on each side just use mod(2) to genterate t-coords ;-)
    for(vtkIdType i= 0; i < filter->GetOutput()->GetNumberOfPoints(); i++)
	TCoords->SetTuple2(i, TCoords->GetTuple2(i)[0], (double)(i % 2)); //adjust texture image to fit this TCoord

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


