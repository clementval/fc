! RUN: %fc %s -o %t && %t | FileCheck %s
program test
  integer, dimension(10, 10) :: array
  array(1:10,1:10) = 10
  print *,array
end program test

!CHECK: 10           10           10           10           10           10           10          
