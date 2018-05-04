/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCleanPolyDataPolys.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// .NAME vtkCleanPolyDataPolys - remove duplicate/degenerate polygons
//
// .SECTION Description
// Merges duplicate polygons. Assumes the input grid does not contain duplicate
// points. You may want to run vtkCleanPolyData first to assert it. If
// duplicated polygons are found they are removed in the output.
//
// .SECTION See Also
// vtkCleanPolyData

#ifndef __vtkCleanPolyDataPolys_h
#define __vtkCleanPolyDataPolys_h

#include "vtkPolyDataAlgorithm.h"

class vtkCleanPolyDataPolys: public vtkPolyDataAlgorithm
{
public:
  static vtkCleanPolyDataPolys *New();

  vtkTypeMacro(vtkCleanPolyDataPolys, vtkPolyDataAlgorithm);

  void PrintSelf(ostream& os, vtkIndent indent);

protected:
  vtkCleanPolyDataPolys();
  ~vtkCleanPolyDataPolys();

  virtual int RequestData(vtkInformation *, vtkInformationVector **,
                          vtkInformationVector *);

private:
  vtkCleanPolyDataPolys(const vtkCleanPolyDataPolys&); // Not implemented
  void operator=(const vtkCleanPolyDataPolys&); // Not implemented
};

#endif
