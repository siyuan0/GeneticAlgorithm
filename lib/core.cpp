#include "core.hpp"

void GA::generateInitialPopulation()
{
    std::cout << "generateInitialPopulation: unimplemented\n";
}

void GA::evalPopulation(std::shared_ptr<std::vector<solution>> population)
{
    std::cout << "evalPopulation: unimplemented\n";
}

std::vector<int> GA::getParents(int numParents)
{
    std::cout << "getParents: unimplemented\n";
    std::vector<int> nilreturn;
    return nilreturn;
}

std::vector<solution> GA::getNewPopulation(int numNewPopulation)
{
    std::cout << "getNewPopulation: unimplemented\n";
    std::vector<solution> nilreturn;
    return nilreturn;
}

void GA::updatePopulation(std::vector<solution> newPopulation)
{
    std::cout << "updatePopulation: unimplemented\n";
}

bool GA::terminateSearch()
{
    std::cout << "terminateSearch: unimplemented\n";
    return true;
}

void GA::optimise()
{
    generateInitialPopulation();
    int iter = 0;

    while(iter < _parameters["max_iterations"])
    {
        iter += 1;

        // perform population search
        std::vector<int> parentIdx = getParents(_policy.numParents);
        std::vector<solution> newPopulation = getNewPopulation(_policy.numPopulationReplaced);
        updatePopulation(newPopulation);
        if(terminateSearch()) break;
    }
}