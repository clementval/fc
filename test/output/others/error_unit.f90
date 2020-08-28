! RUN: %fc %s -o %t && %t | FileCheck %s
program test
  use, intrinsic :: iso_fortran_env, only : error_unit
 
  print *, "Error unit ", error_unit
 
end program test 

!CHECK: Error unit                    0
