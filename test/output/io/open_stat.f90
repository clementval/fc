! RUN: %fc %s -o %t && %t | FileCheck %s
program test
  integer                               :: iostat1 = -1

  print *, "Before ", iostat1
  open(unit=10, file = "1.in", iostat=iostat1)
  print *, "After ", iostat1
  close(10)

end program test

!CHECK: Before                       -1

!CHECK: After                         0
