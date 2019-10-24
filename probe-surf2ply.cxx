// program to apply vtkProbeFilter and to save result as PLY which supports colors, does not need scene and can be imported into blender
//01: based on probe-surf2x3d.cxx
// https://lorensen.github.io/VTKExamples/site/Cxx/IO/WritePLY/
// https://lorensen.github.io/VTKExamples/site/Cxx/Visualization/AssignCellColorsFromLUT/




#include <vtkSmartPointer.h>
#include <vtkImageReader2Factory.h>
#include <vtkImageReader2.h>
#include <vtkXMLPolyDataReader.h>
#include <vtkProbeFilter.h>
#include <vtkColorTransferFunction.h>
#include <vtkUnsignedCharArray.h>
#include <vtkPolyData.h>
#include <vtkDoubleArray.h>
#include <vtkImageData.h>//reader1->GetOutput()
#include <vtkPointData.h>
#include <vtkPLYWriter.h>

#include <vtkCallbackCommand.h>
#include <vtkCommand.h>


void FilterEventHandlerVTK(vtkObject* caller, long unsigned int eventId, void* clientData, void* callData){

    vtkAlgorithm *filter= static_cast<vtkAlgorithm*>(caller);

    switch(eventId){
    case vtkCommand::ProgressEvent:
        fprintf(stderr, "\r%s progress: %5.1f%%", filter->GetClassName(), 100.0 * filter->GetProgress());//stderr is flushed directly
        break;
    case vtkCommand::EndEvent:
        std::cerr << std::endl << std::flush;
        break;
        }
    }


int main (int argc, char *argv[]){

    if (argc != 6){
        std::cerr << "Usage: " << argv[0]
                  << " input-surf input-vol"
                  << " output"
                  << " lut-start lut-end"
                  << std::endl;
        return EXIT_FAILURE;
        }

    if(!(strcasestr(argv[1],".vtp"))) {
        std::cerr << "The first input should end with .vtp" << std::endl;
        return -1;
        }

    if(!(strcasestr(argv[2],".mha") || strcasestr(argv[2],".mhd"))) {
        std::cerr << "The second input should end with .mha or .mhd" << std::endl;
        return -1;
        }

    if(!(strcasestr(argv[3],".ply"))) {
        std::cerr << "The output should end with .ply" << std::endl;
        return -1;
        }


    vtkSmartPointer<vtkCallbackCommand> eventCallbackVTK = vtkSmartPointer<vtkCallbackCommand>::New();
    eventCallbackVTK->SetCallback(FilterEventHandlerVTK);


    vtkSmartPointer<vtkXMLPolyDataReader> reader0= vtkSmartPointer<vtkXMLPolyDataReader>::New();
    reader0->SetFileName(argv[1]);
    reader0->AddObserver(vtkCommand::AnyEvent, eventCallbackVTK);
    reader0->Update();

    vtkSmartPointer<vtkImageReader2Factory> readerFactory= vtkSmartPointer<vtkImageReader2Factory>::New();    
    vtkImageReader2* reader1= readerFactory->CreateImageReader2(argv[2]);
    if(!reader1){
        std::cerr << "Could not find an appropriate reader. Does file exist?" << std::endl;
        return -1;
        }	
    reader1->SetFileName(argv[2]);
    reader1->AddObserver(vtkCommand::AnyEvent, eventCallbackVTK);
    reader1->Update();//does not throw an error if file cannot be read! Use CanReadFile if readerFactory ought to be avoided

    vtkSmartPointer<vtkProbeFilter> filter= vtkSmartPointer<vtkProbeFilter>::New();
    filter->SetInputConnection(reader0->GetOutputPort());
    filter->SetSourceConnection(reader1->GetOutputPort());
    filter->PassPointArraysOff();
    filter->PassCellArraysOff();
    filter->PassFieldArraysOff();
    filter->ComputeToleranceOn();
    filter->AddObserver(vtkCommand::AnyEvent, eventCallbackVTK);
    filter->Update();

    vtkSmartPointer<vtkColorTransferFunction> lut= vtkSmartPointer<vtkColorTransferFunction>::New();
    lut->SetColorSpaceToRGB();
    lut->AddRGBPoint(atof(argv[4]),0,0,1);//blue
    lut->AddRGBPoint(atof(argv[5]),1,0,0);//red
    lut->SetScaleToLinear();

    //// create color array from LUT: https://lorensen.github.io/VTKExamples/site/Cxx/Visualization/AssignCellColorsFromLUT/
    vtkSmartPointer<vtkUnsignedCharArray> colorData= vtkSmartPointer<vtkUnsignedCharArray>::New();
    colorData->SetName("Colors"); // naming possibly essential: https://lorensen.github.io/VTKExamples/site/Cxx/IO/WritePLY/#description
    colorData->SetNumberOfComponents(3);

    for (vtkIdType i= 0; i < filter->GetOutput()->GetNumberOfPoints(); i++){
	double rgb[3];
	unsigned char ucrgb[3];
	
	lut->GetColor(
	    vtkDoubleArray::SafeDownCast(
		filter->GetOutput()->GetPointData()->GetArray(
		    reader1->GetOutput()->GetPointData()->GetArrayName(0)
		    )
		)->GetValue(i), rgb);
	for (size_t j = 0; j < 3; ++j){
	    ucrgb[j] = static_cast<unsigned char>(rgb[j] * 255);
	    }
	colorData->InsertNextTuple3(ucrgb[0], ucrgb[1], ucrgb[2]);
	}
    
    filter->GetOutput()->GetPointData()->SetScalars(colorData);

    vtkSmartPointer<vtkPLYWriter> writer= vtkSmartPointer<vtkPLYWriter>::New();
    writer->SetInputData(filter->GetOutput());
    writer->SetFileName(argv[3]);
    writer->SetArrayName("Colors"); // essential: https://lorensen.github.io/VTKExamples/site/Cxx/IO/WritePLY/#description
    writer->EnableAlphaOff();
    writer->AddObserver(vtkCommand::AnyEvent, eventCallbackVTK);
    std::cerr << "PLY export... ";
    writer->Write();
    std::cerr << "done." << std::endl;

    return EXIT_SUCCESS;
    }
