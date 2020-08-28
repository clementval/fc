! RUN: %fc %s -o %t && %t | FileCheck %s
module mod

contains
  function add(x, y)
    real :: x
    real :: y
    real add
    add = x + y
  end function add
end module mod


program t

    use mod
    real :: r
    r = add(10.55, 5.1010)
    print *,  r

end program t

!CHECK: 15.651
