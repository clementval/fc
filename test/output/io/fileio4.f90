program readtest
  character ::msg(7)
  integer i

  open  (1, file = '../input/4.dat', status = 'old')
  read (1, *) msg
  print *, msg
  close(1)

end program readtest
! RUN: %fc %s -o %t && %t | FileCheck %s
!CHECK: aofcftf
