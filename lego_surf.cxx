//
// GenerateCubesFromLabels
//   Usage: GenerateCubesFromLabels InputVolume Startlabel Endlabel
//          where
//          InputVolume is a meta file containing a 3 volume of
//            discrete labels.
//          StartLabel is the first label to be processed
//          EndLabel is the last label to be processed
//          NOTE: There can be gaps in the labeling. If a label does
//          not exist in the volume, it will be skipped.
//
//
#include <vtkSmartPointer.h>

#include <vtkMetaImageReader.h>
#include <vtkImageWrapPad.h>
#include <vtkMaskFields.h>
#include <vtkThreshold.h>
#include <vtkTransformFilter.h>
#include <vtkGeometryFilter.h>
#include <vtkXMLPolyDataWriter.h>

#include <vtkTransform.h>
#include <vtkImageData.h>
#include <vtkPointData.h>
#include <vtkCellData.h>

int main (int argc, char *argv[]){
    if (argc != 5){
        cout << "Usage: "
             << argv[0]
             << " Input.mha"
             << " Output.vtp"
             << " StartLabel EndLabel"
             << endl;
        return EXIT_FAILURE;
        }

    // Create all of the classes we will need
    vtkSmartPointer<vtkMetaImageReader> reader =
        vtkSmartPointer<vtkMetaImageReader>::New();
    vtkSmartPointer<vtkImageWrapPad> pad =
        vtkSmartPointer<vtkImageWrapPad>::New();
    vtkSmartPointer<vtkMaskFields> scalarsOff =
        vtkSmartPointer<vtkMaskFields>::New();
    vtkSmartPointer<vtkThreshold> selector =
        vtkSmartPointer<vtkThreshold>::New();
    vtkSmartPointer<vtkGeometryFilter> geometry =
        vtkSmartPointer<vtkGeometryFilter>::New();
    vtkSmartPointer<vtkTransformFilter> transformModel =
        vtkSmartPointer<vtkTransformFilter>::New();
    vtkSmartPointer<vtkTransform> transform =
        vtkSmartPointer<vtkTransform>::New();
    vtkSmartPointer<vtkXMLPolyDataWriter> writer =
        vtkSmartPointer<vtkXMLPolyDataWriter>::New();

    // Define all of the variables
    unsigned int startLabel = atoi(argv[3]);
    unsigned int endLabel = atoi(argv[4]);

    // Generate cubes from labels
    // 1) Read the meta file
    // 3) Convert point data to cell data
    // 4) Output model with its image-scalars

    reader->SetFileName(argv[1]);
    reader->Update();

    // Pad the volume so that we can change the point data into cell
    // data.
    int *extent = reader->GetOutput()->GetExtent();
    pad->SetInputConnection(reader->GetOutputPort());
    pad->SetOutputWholeExtent(
        extent[0], extent[1] + 1,
        extent[2], extent[3] + 1,
        extent[4], extent[5] + 1);
    pad->Update();

    // Copy the scalar point data of the volume into the scalar cell data
    pad->GetOutput()->GetCellData()->SetScalars(
        reader->GetOutput()->GetPointData()->GetScalars());

    selector->SetInputConnection(pad->GetOutputPort());
    selector->SetInputArrayToProcess(0, 0, 0,
        vtkDataObject::FIELD_ASSOCIATION_CELLS,
        vtkDataSetAttributes::SCALARS);
    selector->ThresholdBetween(startLabel, endLabel);

    // Shift the geometry by 1/2
    transform->Translate (-.5, -.5, -.5);
    transformModel->SetTransform(transform);
    transformModel->SetInputConnection(selector->GetOutputPort());

    // Strip the point scalars from the output, keeping the cell scalars
    scalarsOff->SetInputConnection(transformModel->GetOutputPort());
    scalarsOff->CopyAttributeOff(vtkMaskFields::POINT_DATA,
        vtkDataSetAttributes::SCALARS);

    geometry->SetInputConnection(scalarsOff->GetOutputPort());

    writer->SetInputConnection(geometry->GetOutputPort());
    writer->SetFileName(argv[2]);
    writer->Write();

    return EXIT_SUCCESS;
    }
