////program to create a hull from a set of planes
//01: based on template.cxx and FacetAnalyser.cxx




#include <vtkSmartPointer.h>
#include <vtkDoubleArray.h>
#include <vtkPlanes.h>
#include <vtkHull.h>
#include <vtkCleanPolyData.h>
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

    if (argc < 10){
        std::cerr << "Usage: " << argv[0]
                  << " output"
                  << " compress"
                  << " as-is(0)|outer-hull(1)"
                  << " origin_x origin_y origin_z normal_x normal_y normal_z ..."
                  << std::endl
                  << " NOTE: Normals MUST be NORMALIZED!"
                  << std::endl;
        return EXIT_FAILURE;
        }

    if(!(strcasestr(argv[1],".vtp"))) {
        std::cerr << "The output should end with .vtp" << std::endl;
        return -1;
        }


    VTK_CREATE(vtkCallbackCommand, eventCallbackVTK);
    eventCallbackVTK->SetCallback(FilterEventHandlerVTK);

    vtkSmartPointer<vtkPoints> points= vtkSmartPointer<vtkPoints>::New();
    vtkSmartPointer<vtkDoubleArray> normals= vtkSmartPointer<vtkDoubleArray>::New();
    normals->SetNumberOfComponents(3);

    for(int i= 4; i < argc; i+=6){
        points->InsertNextPoint(atof(argv[i+0]), atof(argv[i+1]), atof(argv[i+2]));

        double c[3];
        c[0]= atof(argv[i+3]);
        c[1]= atof(argv[i+4]);
        c[2]= atof(argv[i+5]);
        normals->InsertNextTuple(c);

        fprintf(stderr, "%d: %6.2f;%6.2f;%6.2f     %6.2f;%6.2f;%6.2f\n", (i-4)/6,  atof(argv[i+0]), atof(argv[i+1]), atof(argv[i+2]), atof(argv[i+3]), atof(argv[i+4]), atof(argv[i+5]));
        }
    //points->Print(std::cerr);
    std::cerr << std::endl;

    vtkSmartPointer<vtkPlanes> planes= vtkSmartPointer<vtkPlanes>::New();
    planes->SetPoints(points);
    planes->SetNormals(normals);

    vtkSmartPointer<vtkCleanPolyData> cleanFilter= vtkSmartPointer<vtkCleanPolyData>::New();
    vtkSmartPointer<vtkHull> hull= vtkSmartPointer<vtkHull>::New();
    hull->AddObserver(vtkCommand::AnyEvent, eventCallbackVTK);

    hull->SetPlanes(planes);
    vtkSmartPointer<vtkPolyData> polydata0 = vtkSmartPointer<vtkPolyData>::New();
    polydata0->SetPoints(points);

    if(atoi(argv[3])){
        hull->SetInputData(polydata0);
        hull->Update();
        cleanFilter->SetInputConnection(hull->GetOutputPort());
        fprintf(stderr, "Hull contains %d points and %d faces.\n", hull->GetOutput()->GetNumberOfPoints(), hull->GetOutput()->GetNumberOfCells());
        }
    else {
        vtkSmartPointer<vtkPolyData> polydata1 = vtkSmartPointer<vtkPolyData>::New();
        hull->GenerateHull(polydata1, polydata0->GetBounds());//replaced SetInputData and Update
        //hull->GenerateHull(polydata1, -10000, 10000, -10000, 10000, -10000, 10000);//replaced SetInputData and Update
        cleanFilter->SetInputData(polydata1);
        fprintf(stderr, "Hull contains %d points and %d faces.\n", polydata1->GetNumberOfPoints(), polydata1->GetNumberOfCells());
        }

    cleanFilter->PointMergingOn();//this is why it's done
    cleanFilter->ConvertPolysToLinesOn();
    cleanFilter->SetTolerance(0.000001);//small tolerance needed for most hulls
    cleanFilter->ToleranceIsAbsoluteOff();//relative tolerance
    cleanFilter->AddObserver(vtkCommand::AnyEvent, eventCallbackVTK);
    cleanFilter->Update();


    VTK_CREATE(vtkXMLPolyDataWriter, writer);
    writer->SetInputConnection(cleanFilter->GetOutputPort());
    writer->SetFileName(argv[1]);
    writer->SetDataModeToBinary();//SetDataModeToAscii()//SetDataModeToAppended()
    if(atoi(argv[2]))
        writer->SetCompressorTypeToZLib();//default
    else
        writer->SetCompressorTypeToNone();
    writer->AddObserver(vtkCommand::AnyEvent, eventCallbackVTK);
    writer->Write();

    return EXIT_SUCCESS;
    }


