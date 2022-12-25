#ifndef INCLUDE_SCHWEFEL
#define INCLUDE_SCHWEFEL

#include "../../lib/solution.hpp"
#include <cstdlib>
#include <ostream>
#include <cmath>

#define DIMENSION 1 // to override this define for other dimensions

namespace Schwefel
{
    static int dimension = 1;

class soln : solution
{
private:
    float x[DIMENSION]; // using array instead of vector to have it stored on stack for faster creation/access/deletion
    float f;
    bool evaluated;

    float evaluateObjective()
    { // evaluate Schwefel's function on this solution
        float tmp = 0;
        for(int i=0; i<DIMENSION; i++) tmp -= x[i] * std::sin(std::sqrt(std::fabs(x[i])));
        return tmp;
    }

public:
    soln(float lowerbound, float upperbound)
    {  // randomly generate a solution within the provided bounds
        for(int i=0; i<DIMENSION; i++) x[i] = lowerbound + std::rand() / RAND_MAX * (upperbound-lowerbound);
        evaluated = false;
    }

    float getEval()
    {
        if(!evaluated)
        {
            f = evaluateObjective();
            evaluated = true;
            return f;
        }else
        {
            return f;
        }
    }

    friend std::ostream& operator<< (std::ostream& stream, const soln& s)
    {   // for printing out the contents of a solution
        stream << "x: [";
        for(int i=0; i<DIMENSION; i++) stream << s.x[i] << ',';
        stream << "] f: ";
        if(s.evaluated)
        {
            stream << s.f;
        }else
        {
            stream << "unevaluated";
        }
    }
};
} // namespace Schwefel

#endif // INCLUDE_SCHWEFEL