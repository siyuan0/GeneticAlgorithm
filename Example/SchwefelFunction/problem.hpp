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
#include <algorithm>
#include <utility>

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

std::vector<int> getParentIdx(std::vector<soln>& population, int rangeStart, int rangeEnd, 
                              std::unordered_map<std::string, float> parameters)
{  /* parents chosen by ranking selection
      p(selected)=(S*(N+1-2*R_i) + 2*(R_i-1))/(N*(N-1)) 
      where S = selection pressure, N is size of population considered,
      R_i is the solution rank */ 
    
    // get the rank of each solution
    float N = rangeEnd - rangeStart;
    std::vector<std::pair<float, int>> sortingArr;
    for(int i=rangeStart; i<rangeEnd; i++) sortingArr.push_back(std::pair<float, int>(population[i].getEval(), i));
    struct {
        bool operator()(std::pair<float, int> s1, std::pair<float, int> s2) const {return s1.first < s2.first;}
    } compareFunc;
    std::sort(sortingArr.begin(), sortingArr.end(), compareFunc);

    // probabilistically choose parents
    std::vector<int> chosenParents;
    for(int i=0; i<sortingArr.size(); i++)
    {
        float rank = i+1;
        float acceptCriteria = 
            (parameters["selection pressure"] * (N + 1 - 2 * rank) + 2 * (rank-1)) /
            (N * (N-1));
        if(std::rand() < RAND_MAX * acceptCriteria) chosenParents.push_back(sortingArr[i].second);
    }

    RESERVECOUT
    LOG("chosen parents: \n")
    for(int i=0; i<chosenParents.size(); i++) LOG( '\t' << population[chosenParents[i]] << '\n')
    UNRESERVECOUT
    
    return chosenParents;
}

static ProblemCtx<soln> problemCtx = {
    .getRandomSolutions = &getInitialPopulation,
    .getParentIdx = &getParentIdx
};

} // namespace Schwefel

#endif // INCLUDE_SCHWEFEL