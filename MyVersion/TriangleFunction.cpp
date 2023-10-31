#include <cmath>

/*
This function makes the double linear fit for the strip energy as a function of the base position.
*/

double TriangleFunction(double *u, double *par){
    /*
    Input: Memory location of variables u and par
    */
   double x = u[0];
   double height = par[0], shift = par[1];
   x -= shift;
   if (fabs(x) > 17.0) return 0.0;
   else return height*(17.0 - fabs(x));
}

