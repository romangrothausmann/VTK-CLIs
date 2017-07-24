////program to generate (fit) ellipsoids from a sphere source
//01: based on template.cxx




#include <vtkSmartPointer.h>
#include <vtkSphereSource.h>
#include <vtkMatrix4x4.h>
#include <vtkTransform.h>
#include <vtkTransformPolyDataFilter.h>
#include <vtkCellData.h>
#include <vtkIntArray.h>
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
                  << " output"
                  << " compress"
                  << " radius"
                  << " resT resP"
                  << " LatLongTess"
                  << std::endl
                  << " Ell transform params read from stdin as a single line:"
                  << " sx sy sz"
                  << " tx ty tz"
                  << " r[3x3]"
                  << " type"
                  << std::endl;
        return EXIT_FAILURE;
        }

    if(!(strcasestr(argv[1],".vtp"))) {
        std::cerr << "The output should end with .vtp" << std::endl;
        return -1;
        }


    VTK_CREATE(vtkCallbackCommand, eventCallbackVTK);
    eventCallbackVTK->SetCallback(FilterEventHandlerVTK);


    VTK_CREATE(vtkSphereSource, ell);
    ell->SetRadius(atof(argv[3]));
    ell->SetThetaResolution(atof(argv[4]));
    ell->SetPhiResolution(atof(argv[5]));
    ell->SetLatLongTessellation(atoi(argv[6]));

    std::string str;
    std::vector<double> in;
    if(std::getline(std::cin, str)) { // https://stackoverflow.com/a/7800876
	std::istringstream sstr(str);
	double n;
	while(sstr >> n)
	    in.push_back(n);
	}
    // for(int i=0; i < in.size(); ++i)
    // 	std::cerr << in[i] << ' ';

    double *IN= &in[0]; // https://stackoverflow.com/a/2923290
    double s[3], t[3];

    for(int i = 0; i < 3; i++) // https://stackoverflow.com/a/33682723
	s[i]= in[i];
    for(int i = 0; i < 3; i++)
	t[i]= in[i+3];

    VTK_CREATE(vtkMatrix4x4, m);
    m->Identity(); // https://stackoverflow.com/a/37469151
    for(int i = 0; i < 9; i++)
	m->SetElement(i%3, i/3, in[i+6]); // https://stackoverflow.com/a/7070383
    // m->PrintSelf(std::cerr, vtkIndent(2));
    
    VTK_CREATE(vtkTransform, tf);
    tf->SetMatrix(m);
    tf->Scale(s);
    tf->PostMultiply();
    tf->Translate(t);

    VTK_CREATE(vtkTransformPolyDataFilter, filter);
    filter->SetInputConnection(ell->GetOutputPort());
    filter->SetTransform(tf);
    filter->AddObserver(vtkCommand::AnyEvent, eventCallbackVTK);
    filter->Update();

    //// set all cell-scalars (for colouring) to ellType value
    VTK_CREATE(vtkIntArray, cdata);
    int n= filter->GetOutput()->GetNumberOfCells();
    cdata->SetNumberOfValues(n);
    for(int i= 0; i < n; i++)
	cdata->SetValue(i, in[15]);	
    filter->GetOutput()->GetCellData()->SetScalars(cdata);

    VTK_CREATE(vtkXMLPolyDataWriter, writer);
    writer->SetInputConnection(filter->GetOutputPort());
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


