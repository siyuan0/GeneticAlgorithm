#ifndef INCLUDE_SCHWEFEL
#define INCLUDE_SCHWEFEL

#include "../../lib/solution.hpp"
#include "../../lib/core.hpp"
#include <cstdlib>
#include <ostream>
#include <cmath>
#include <vector>
#include <unordered_map>
#include <string>

#define DIMENSION 2 // to change this define for other dimensions

namespace Schwefel
{
    static int dimension = 1;

class soln : public solution
{
private:
    float x[DIMENSION]; // using array instead of vector to have it stored on stack for faster creation/access/deletion
    float f;

    float evaluateObjective()
    { // evaluate Schwefel's function on this solution
        float tmp = 0;
        for(int i=0; i<DIMENSION; i++) tmp -= x[i] * std::sin(std::sqrt(std::fabs(x[i])));
        return tmp;
    }

public:
    soln(float lowerbound, float upperbound)
    {  // randomly generate a solution within the provided bounds
        for(int i=0; i<DIMENSION; i++) x[i] = lowerbound +  float(std::rand()) / RAND_MAX * (upperbound-lowerbound);
        f = evaluateObjective();
    }

    float getEval()
    {
        return f;
    }

    friend std::ostream& operator<< (std::ostream& stream, const soln& s)
    {   // for printing out the contents of a solution
        stream << "x: [";
        for(int i=0; i<DIMENSION; i++) stream << s.x[i] << ", ";
        stream << "] f: " << s.f;
        return stream;
    }
};

std::vector<soln> getInitialPopulation(int size, std::unordered_map<std::string, float> parameters)
{
    std::vector<soln> v;
    for(int i=0; i<size; i++) v.push_back(soln(
        parameters["min xi"],
        parameters["max xi"]
    ));
    return v;
}

static ProblemCtx<soln> problemCtx = {
    .getRandomSolutions = &getInitialPopulation
};

} // namespace Schwefel

#endif // INCLUDE_SCHWEFEL