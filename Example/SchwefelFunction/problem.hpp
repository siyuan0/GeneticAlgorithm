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

#define DIMENSION 6 // to change this define for number of dimensions considered

namespace Schwefel
{
    static int num_of_evaluations = 0; // track the total number of evaluations of Schwefel's function

    static thread_local std::mt19937 randomGen; // a copy is created for each thread, so that we don't share
                                                // the random generator which slows thread execution

class soln : public solution
{
private:
    float x[DIMENSION]; // using array instead of vector to have it stored on stack for faster creation/access/deletion
    float f;
    float _lbound;
    float _ubound;

    float evaluateObjective()
    {   // evaluate Schwefel's function on this solution
        num_of_evaluations += 1;
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
    {  // randomly generate a soln within the provided constraints
        _lbound = lowerbound;
        _ubound = upperbound;
        std::uniform_real_distribution<float> urand{lowerbound, upperbound};
        for(int i=0; i<DIMENSION; i++) x[i] = urand(Schwefel::randomGen);
        f = evaluateObjective();
    }

    soln()
    {   // default constructor
        _lbound = 0;
        _ubound = 0;
        for(int i=0; i<DIMENSION; i++) x[i] = 0;
        f = 0;
    }

    void doEval(){ f = evaluateObjective(); }

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
    {   // same goal as operator<<, but slightly more formatted
        std::stringstream ss;
        ss << "x: [";
        for(int i=0; i<DIMENSION-1; i++) ss << x[i] << ", ";
        ss << x[DIMENSION-1] << "] f: " << f;
        return ss.str();
    }
};

float l2(soln& s1, soln& s2)
{  // get the l2 norm of s1-s2
    float sum = 0; 
    for(int i=0; i<DIMENSION; i++) sum += std::pow(s1.getX(i) - s2.getX(i), 2);
    return std::pow(sum, 0.5);
}

// set the random gen for this thread
void setThreadRandomGenerator(std::mt19937 gen){randomGen = gen;} 

soln getBestSoln(std::vector<soln>& population)
{   // return the best soln in the population
    int idx_best = 0;
    for(int i=0; i<population.size(); i++)
    {
        if(population[i].getEval() < population[idx_best].getEval()) idx_best = i;
    }
    return population[idx_best];
}

std::vector<soln> getInitialPopulation(int size, std::unordered_map<std::string, float>& parameters)
{   // randomly initialise initial population
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
        if(intRand(0, RAND_MAX, randomGen) < RAND_MAX * acceptCriteria) chosenParents.push_back(sortingArr[i].second);
    }

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
        // randomly choose the other parent from the parent candidates
        int otherParentIdx = i;
        while(otherParentIdx == i) otherParentIdx = intRand(0, parentIdx.size(), randomGen);

        // sample from Normal Dist for new children soln
        std::normal_distribution<float> rn{0, parameters["Breeding Variance Scale"] * l2(population[i], population[otherParentIdx])};
        soln child(parameters["min xi"], parameters["max xi"]);
        Schwefel::num_of_evaluations -= 1; // there was an extra evaluation done when initialising soln
        for(int ii=0; ii<DIMENSION; ii++)
        {
            // repeat until drawn xi does not violate problem constraints
            float newx = parameters["max xi"] + 1;
            while((newx > parameters["max xi"]) | (newx < parameters["min xi"]))
            {
                newx = population[i].getX(ii) + rn(randomGen);
            }
            child.setX(ii, newx);
        }
        child.doEval();
        children.push_back(child);
    }
    
    return children;
}

void updatePopulation(std::vector<soln>& population, std::vector<soln>& children, 
                      std::vector<std::pair<float, int>> sortedIdx, 
                      std::unordered_map<std::string, float>& parameters)
{   // update the current population by replacing the worst soln with new child soln
    for(int i=0; i<children.size(); i++)
    {
        population[sortedIdx[sortedIdx.size()-i-1].second] = children[i];
    }
}

bool endSearch(std::unordered_map<std::string, float>& parameters)
{   // end when computational budget is exceeded
    return Schwefel::num_of_evaluations > parameters["max_eval"];
}

// store the problem specific methods for the GA core to run on
static ProblemCtx<soln> problemCtx = {
    .setRandomGenerator = &setThreadRandomGenerator,
    .getRandomSolutions = &getInitialPopulation,
    .getParentIdx = &getParentIdx,
    .getChildren = &getChildren,
    .updatePopulation = &updatePopulation,
    .endSearch = &endSearch
};

} // namespace Schwefel

#endif // INCLUDE_SCHWEFEL