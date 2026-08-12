#pragma once
#include <stddef.h>

class Complex
{
public:
    Complex(int a, int b) : r(a), c(b) {}

private:

    [[maybe_unused]]
    double r, c;

};