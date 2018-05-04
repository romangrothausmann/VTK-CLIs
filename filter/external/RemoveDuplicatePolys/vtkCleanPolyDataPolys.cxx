/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGenericClip.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkCleanPolyDataPolys.h"

#include "vtkCell.h"
#include "vtkPoints.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkCellData.h"
#include "vtkObjectFactory.h"
#include "vtkIntArray.h"
#include "vtkCollection.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"

#include <algorithm>
#include <set>
#include <map>
#include <vector>

vtkStandardNewMacro(vtkCleanPolyDataPolys);

//----------------------------------------------------------------------------
vtkCleanPolyDataPolys::vtkCleanPolyDataPolys()
{
}

//----------------------------------------------------------------------------
vtkCleanPolyDataPolys::~vtkCleanPolyDataPolys()
{
}

//----------------------------------------------------------------------------
void vtkCleanPolyDataPolys::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
int vtkCleanPolyDataPolys::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  vtkPolyData *input = vtkPolyData::SafeDownCast(
    inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkPolyData *output = vtkPolyData::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  if (input->GetNumberOfPolys() == 0)
  {
    // set up a polyData with same data arrays as input, but
    // no points, polys or data.
    output->Allocate(1);
    output->GetPointData()->CopyAllocate(input->GetPointData(), VTK_CELL_SIZE);
    vtkPoints *pts = vtkPoints::New();
    pts->SetDataTypeToDouble();

    output->SetPoints(pts);
    pts->Delete();
    return 1;
  }

  // Copy over the original points. Assume there are no degenerate points.
  output->SetPoints(input->GetPoints());

  // remove duplicate polys.
  std::map<std::set<int>, vtkIdType > polySet;
  std::map<std::set<int>, vtkIdType >::iterator polyIter;

  // Now copy the polys.
  vtkIdList *polyPoints = vtkIdList::New();
  const int numberOfPolys = input->GetNumberOfPolys();
  vtkIdType progressStep = numberOfPolys / 100;
  if (progressStep == 0)
  {
    progressStep = 1;
  }

  output->Allocate(numberOfPolys);
  int ndeg = 0;
  int ndup = 0;

  output->GetPointData()->PassData(input->GetPointData());
  output->GetCellData()->CopyAllocate(input->GetCellData(), numberOfPolys);

  for (int id = 0; id < numberOfPolys; id++)
  {
    if (id % progressStep == 0)
    {
      this->UpdateProgress(0.8 + 0.2 * (static_cast<float>(id) / numberOfPolys));
    }

    // duplicate points do not make poly verticies or triangle
    // strips degenerate so don't remove them
    int polyType = input->GetCellType(id);
    if(polyType == VTK_POLY_VERTEX || polyType == VTK_TRIANGLE_STRIP)
    {
      input->GetCellPoints(id, polyPoints);
      vtkIdType newId = output->InsertNextCell(polyType, polyPoints);
      output->GetCellData()->CopyData(input->GetCellData(), id, newId);
      continue;
    }

    input->GetCellPoints(id, polyPoints);
    std::set<int> nn;
    std::vector<int> ptIds;
    for (int i = 0; i < polyPoints->GetNumberOfIds(); i++)
    {
      int polyPtId = polyPoints->GetId(i);
      nn.insert(polyPtId);
      ptIds.push_back(polyPtId);
    }

    // this conditional may generate non-referenced nodes
    polyIter = polySet.find(nn);

    // only copy a cell to the output if it is neither degenerate nor duplicate
    if(nn.size() == static_cast<unsigned int>(polyPoints->GetNumberOfIds()) &&
       polyIter == polySet.end())
    {
      vtkIdType newId = output->InsertNextCell(input->GetCellType(id), polyPoints);
      output->GetCellData()->CopyData(input->GetCellData(), id, newId);
      polySet[nn] = newId;
    }
    else if(polyIter != polySet.end())
    {
      ndup++; // cell has duplicate(s)
    }
  }

  if(ndup)
  {
    vtkDebugMacro(<< "vtkCleanPolyDataPolys : " << ndup
      << " duplicate polys (multiple instances of a polygon) have been"
      << " removed." << endl);

    polyPoints->Delete();
    output->Squeeze();
  }

  return 1;
}
