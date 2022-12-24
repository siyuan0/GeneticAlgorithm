#ifndef INCLUDE_SCHWEFEL
#define INCLUDE_SCHWEFEL

#include "../../lib/solution.hpp"
#include <cstdlib>
#include <ostream>

#define DIMENSION 1 // to override this define for other dimensions

namespace Schwefel
{
    static int dimension = 1;

class soln;



class soln : solution
{
private:
    float x[DIMENSION]; // using array instead of vector to have it stored on stack for faster creation/access/deletion
public:
    soln(float lowerbound, float upperbound)
    {  // randomly generate a solution within the provided bounds
        for(int i=0; i<DIMENSION; i++) x[i] = lowerbound + std::rand() / RAND_MAX * (upperbound-lowerbound);
    }

    friend std::ostream& operator<< (std::ostream& stream, const soln& s)
    {   // for printing out the contents of a solution
        stream << '[';
        for(int i=0; i<DIMENSION; i++) stream << s.x[i] << ',';
        stream << ']';
    }
};
} // namespace Schwefel

#endif // INCLUDE_SCHWEFEL