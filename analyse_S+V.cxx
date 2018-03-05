////program to calculate the surface of a (labled) mesh
//01: based on template.cxx
//02: changed to vtkMassProperties, works even for open surfaces (code for surface area seems not to relate on closeness assumption); slightly slower than former version based on vtkMeshQuality
03: do not print last values if analysis failed: https://lorensen.github.io/VTKExamples/site/Cxx/Utilities/ObserveError/ http://www.vtk.org/Wiki/VTK/Tutorials/Callbacks http://www.vtk.org/Wiki/VTK/Examples/Cxx/Interaction/CallData


#include <vtkSmartPointer.h>
#include <vtkXMLPolyDataReader.h>
#include <vtkTriangleFilter.h>
#include <vtkThreshold.h>
#include <vtkGeometryFilter.h>
#include <vtkCellData.h>//GetCellData()
#include <vtkDataArray.h>//GetScalars()
#include <vtkDataSetAttributes.h>//GetRange()
#include <vtkMassProperties.h>

#include <vtkCallbackCommand.h>
#include <vtkCommand.h>


#define VTK_CREATE(type, name) vtkSmartPointer<type> name = vtkSmartPointer<type>::New()


class ErrorObserver : public vtkCommand
{
public:
  ErrorObserver():
    Error(false),
    Warning(false),
    ErrorMessage(""),
    WarningMessage("") {}
  static ErrorObserver *New()
  {
  return new ErrorObserver;
  }
  bool GetError() const
  {
  return this->Error;
  }
  bool GetWarning() const
  {
  return this->Warning;
  }
  void Clear()
  {
  this->Error = false;
  this->Warning = false;
  this->ErrorMessage = "";
  this->WarningMessage = "";
  }
  virtual void Execute(vtkObject *vtkNotUsed(caller),
                       unsigned long event,
                       void *calldata)
  {
  switch(event)
  {
    case vtkCommand::ErrorEvent:
      ErrorMessage = static_cast<char *>(calldata);
      this->Error = true;
      break;
    case vtkCommand::WarningEvent:
      WarningMessage = static_cast<char *>(calldata);
      this->Warning = true;
      break;
  }
  }
  std::string GetErrorMessage()
  {
  return ErrorMessage;
  }
std::string GetWarningMessage()
{
  return WarningMessage;
}
private:
  bool        Error;
  bool        Warning;
  std::string ErrorMessage;
  std::string WarningMessage;
};

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

    if (argc != 2){
        std::cerr << "Usage: " << argv[0]
                  << " input"
                  << std::endl;
        std::cerr << "Value calculation based on the discrete form of the divergence theorem." << std::endl;
        std::cerr << "Apart from the surface area, the calculations assume a closed mesh!" << std::endl;
        return EXIT_FAILURE;
        }

    if(!(strcasestr(argv[1],".vtp"))) {
        std::cerr << "The input should end with .vtp" << std::endl;
        return -1;
        }


    VTK_CREATE(vtkCallbackCommand, eventCallbackVTK);
    eventCallbackVTK->SetCallback(FilterEventHandlerVTK);


    VTK_CREATE(vtkXMLPolyDataReader, reader);
    reader->SetFileName(argv[1]);
    reader->AddObserver(vtkCommand::AnyEvent, eventCallbackVTK);
    reader->Update();

    VTK_CREATE(vtkTriangleFilter, triangle);
    triangle->SetInputConnection(reader->GetOutputPort());
    triangle->AddObserver(vtkCommand::AnyEvent, eventCallbackVTK);
    triangle->Update();

    VTK_CREATE(vtkThreshold, thr);
    thr->SetInputConnection(0, triangle->GetOutputPort());
    thr->SetInputArrayToProcess(0, 0, 0,         
	vtkDataObject::FIELD_ASSOCIATION_CELLS,
        vtkDataSetAttributes::SCALARS);
    thr->AddObserver(vtkCommand::AnyEvent, eventCallbackVTK);

    VTK_CREATE(vtkGeometryFilter, vtu2vtp);
    vtu2vtp->SetInputConnection(thr->GetOutputPort());
    vtu2vtp->AddObserver(vtkCommand::AnyEvent, eventCallbackVTK);

    VTK_CREATE(vtkMassProperties, filter);
    filter->SetInputConnection(vtu2vtp->GetOutputPort());
    filter->AddObserver(vtkCommand::AnyEvent, eventCallbackVTK);

    std::cout << "#index\tV\tS\tnSI" << std::endl;

    int iRange[2];
    vtkDataArray* tmpArray= reader->GetOutput()->GetCellData()->GetScalars();
    if(tmpArray){
	double dRange[2];
	tmpArray->GetRange(dRange);
	iRange[0]= (int)dRange[0];
	iRange[1]= (int)dRange[1];

	fprintf(stderr, "range of cell scalars: %d - %d\n", iRange[0], iRange[1]);
	}
    if(iRange[1]-iRange[0]){
	for (unsigned int i= iRange[0]; i <= iRange[1]; i++){
	    ////ToDo: insert check cell array if any scalar of i exist

	    thr->ThresholdBetween(i, i);
	    filter->Update();

	    if(!errorObserver->GetError())
	    std::cout
		<< i << "\t"
		<< filter->GetVolume() << "\t"
		<< filter->GetSurfaceArea() << "\t"
		<< filter->GetNormalizedShapeIndex()
		<< std::endl;
	    }
	}
    else{
	filter->SetInputConnection(triangle->GetOutputPort());
	filter->Update();

	std::cout
	    << "all" << "\t"
	    << filter->GetVolume() << "\t"
	    << filter->GetSurfaceArea() << "\t"
	    << filter->GetNormalizedShapeIndex()
	    << std::endl;
	}

    return EXIT_SUCCESS;
    }


