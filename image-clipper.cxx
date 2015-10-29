////program for
//01: based on template.cxx




#include <vtkSmartPointer.h>
#include <vtkMetaImageReader.h>
#include <vtkPlane.h>
#include <vtkImplicitFunctionToImageStencil.h>
#include <vtkImageData.h>
#include <vtkImageStencil.h>
#include <vtkImageMask.h>
#include <vtkMetaImageWriter.h>

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

    if (argc != 11){
        std::cerr << "Usage: " << argv[0]
                  << " input"
                  << " output"
                  << " compress"
                  << " origin_x origin_y origin_z normal_x normal_y normal_z"
                  << " mask-value"
                  << std::endl;
        return EXIT_FAILURE;
        }

    if(!(strcasestr(argv[1],".mha") || strcasestr(argv[1],".mhd"))) {
        std::cerr << "The input should end with .mha or .mhd" << std::endl;
        return -1;
        }

    if(!(strcasestr(argv[2],".mha") || strcasestr(argv[2],".mhd"))) {
        std::cerr << "The output should end with .mha or .mhd" << std::endl;
        return -1;
        }


    VTK_CREATE(vtkCallbackCommand, eventCallbackVTK);
    eventCallbackVTK->SetCallback(FilterEventHandlerVTK);


    VTK_CREATE(vtkMetaImageReader, reader);
    reader->SetFileName(argv[1]);
    reader->AddObserver(vtkCommand::AnyEvent, eventCallbackVTK);
    reader->Update();

    VTK_CREATE(vtkPlane, plane);
    plane->SetOrigin(atof(argv[4]), atof(argv[5]), atof(argv[6]));
    plane->SetNormal(atof(argv[7]), atof(argv[8]), atof(argv[9]));

    VTK_CREATE(vtkImplicitFunctionToImageStencil, filter);
    filter->SetInput(plane);
    filter->SetInformationInput(reader->GetOutput());
    filter->AddObserver(vtkCommand::AnyEvent, eventCallbackVTK);
    filter->Update();

    //// Create an empty (3D) image of appropriate size.
    VTK_CREATE(vtkImageData, image);
    image->SetSpacing(reader->GetOutput()->GetSpacing());
    image->SetOrigin(reader->GetOutput()->GetOrigin());
    image->SetExtent(reader->GetOutput()->GetExtent());
    image->AllocateScalars(VTK_UNSIGNED_CHAR,1);//vtk-6.1.0

    //// apply the image stencil to the empty image
    VTK_CREATE(vtkImageStencil, stencil);
    stencil->SetInputData(image);
    stencil->SetStencilConnection(filter->GetOutputPort());
    //stencil->ReverseStencilOn();
    stencil->SetBackgroundValue(255);
    stencil->AddObserver(vtkCommand::AnyEvent, eventCallbackVTK);
    stencil->Update();

    //// mask the input image with the stencil
    VTK_CREATE(vtkImageMask, mask);
    mask->SetImageInputData(reader->GetOutput()); //mask->SetImageInput(reader->GetOutput()); //5.10.1
    mask->SetMaskInputData(stencil->GetOutput()); //mask->SetMaskInput(stencil->GetOutput()); //5.10.1
    mask->SetMaskedOutputValue(atof(argv[10]));
    //mask->NotMaskOn();
    mask->AddObserver(vtkCommand::AnyEvent, eventCallbackVTK);
    mask->Update();


    VTK_CREATE(vtkMetaImageWriter, writer);
    writer->SetInputConnection(mask->GetOutputPort());
    writer->SetFileName(argv[2]);
    writer->SetFileDimensionality(3);
    writer->SetCompression(atoi(argv[3]));
    writer->AddObserver(vtkCommand::AnyEvent, eventCallbackVTK);
    writer->Write();

    return EXIT_SUCCESS;
    }


