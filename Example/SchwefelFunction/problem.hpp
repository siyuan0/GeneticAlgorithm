#ifndef INCLUDE_SCHWEFEL
#define INCLUDE_SCHWEFEL

#include "../../lib/solution.hpp"
#include "../../lib/core.hpp"
#include "../../lib/utils.hpp"
#include <cstdlib>
#include <ostream>
#include <cmath>
#include <vector>
#include <unordered_map>
#include <string>
#include <algorithm>
#include <utility>
#include <limits>

#define DIMENSION 2 // to change this define for other dimensions

namespace Schwefel
{
    static int dimension = 1;

class soln : public solution
{
private:
    float x[DIMENSION]; // using array instead of vector to have it stored on stack for faster creation/access/deletion
    float f;
    float _lbound;
    float _ubound;

    float evaluateObjective()
    { // evaluate Schwefel's function on this solution
        float tmp = 0;
        for(int i=0; i<DIMENSION; i++) 
        {
            if(x[i] < _lbound | x[i] > _ubound) return std::numeric_limits<float>::max(); // solution is outside constraints
            tmp -= x[i] * std::sin(std::sqrt(std::fabs(x[i])));
        }
        return tmp;
    }

public:
    soln(float lowerbound, float upperbound)
    {  // randomly generate a solution within the provided bounds
        _lbound = lowerbound;
        _ubound = upperbound;
        for(int i=0; i<DIMENSION; i++) x[i] = lowerbound +  float(std::rand()) / RAND_MAX * (upperbound-lowerbound);
        f = evaluateObjective();
    }

    float getEval(){ return f; }

    float getX(int i){ return x[i]; }

    void setX(int i, float val){ x[i]=val; }

    friend std::ostream& operator<< (std::ostream& stream, const soln& s)
    {   // for printing out the contents of a solution
        for(int i=0; i<DIMENSION; i++) stream << s.x[i] << ", ";
        stream << s.f;
        return stream;
    }

    std::string print()
    {
        std::stringstream ss;
        ss << "x: [";
        for(int i=0; i<DIMENSION; i++) ss << x[i] << ", ";
        ss << "] f: " << f;
        return ss.str();
    }
};

float l2(soln& s1, soln& s2)
{  // get the l2 norm of s1-s2
    float sum = 0; 
    for(int i=0; i<DIMENSION; i++) sum += std::pow(s1.getX(i) - s2.getX(i), 2);
    return std::pow(sum, 0.5);
}

std::vector<soln> getInitialPopulation(int size, std::unordered_map<std::string, float>& parameters)
{
    std::vector<soln> v;
    for(int i=0; i<size; i++) v.push_back(soln(
        parameters["min xi"],
        parameters["max xi"]
    ));
    return v;
}

std::pair<std::vector<int>, 
          std::vector<std::pair<float, int>>> getParentIdx(std::vector<soln>& population, int rangeStart, int rangeEnd, 
                              std::unordered_map<std::string, float>& parameters)
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

    // RESERVECOUT
    //     LOG("chosen parents: \n")
    //     for(int i=0; i<chosenParents.size(); i++) LOG( '\t' << population[chosenParents[i]] << '\n')
    // UNRESERVECOUT
    
    return std::pair<std::vector<int>, std::vector<std::pair<float, int>>>{chosenParents, sortingArr};
}

std::vector<soln> getChildren(std::vector<soln>& population, std::vector<int>& parentIdx, 
                              std::unordered_map<std::string, float>& parameters)
{   // each parent will randomly pair with another parent and undergo crossover
    // to produce children, ie. draw X~N(parent1, (Breeding_Variance_Scale)*||parent1-parent2||_2))
    std::vector<soln> children;
    if(parentIdx.size() < 2) return children; // no children produced

    for(int i=0; i<parentIdx.size(); i++)
    {
        int otherParentIdx = i;
        while(otherParentIdx == i) otherParentIdx = std::rand() % parentIdx.size();
        randNormal rn{0, parameters["Breeding Variance Scale"] * l2(population[i], population[otherParentIdx])};
        soln child(parameters["min xi"], parameters["max xi"]);
        for(int ii=0; ii<DIMENSION; ii++) child.setX(ii, population[i].getX(i) + rn.rand());
        children.push_back(child);
    }
    // RESERVECOUT
    //     LOG("children produced:\n")
    //     for(soln s : children) LOG( '\t' << s << '\n')
    // UNRESERVECOUT
    return children;
}

void updatePopulation(std::vector<soln>& population, std::vector<soln>& children, 
                      std::vector<std::pair<float, int>> sortedIdx, 
                      std::unordered_map<std::string, float>& parameters)
{
    for(int i=0; i<children.size(); i++)
    {
        population[sortedIdx[sortedIdx.size()-i-1].second] = children[i];
    }
    THREADPRINT("updated " << children.size() << " solutions in population\n")
}

bool endSearch(std::vector<soln> localCopyOfPopulation)
{    // cannot pass by reference since we are allowing threads to modify population freely
    return false;
}

static ProblemCtx<soln> problemCtx = {
    .getRandomSolutions = &getInitialPopulation,
    .getParentIdx = &getParentIdx,
    .getChildren = &getChildren,
    .updatePopulation = &updatePopulation,
    .endSearch = &endSearch
};

} // namespace Schwefel

#endif // INCLUDE_SCHWEFEL