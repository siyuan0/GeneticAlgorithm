#include "core.hpp"

void GA::generateInitialPopulation()
{

}

void GA::optimise(int max_iterations, int terminateEvalValue, std::shared_ptr<std::vector<solution>> initialPopulation)
{
    if(!initialPopulation) generateInitialPopulation();
    
    
}