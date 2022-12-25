#include <vector>
#include <memory>
#include <ctime>
#include <cstdlib>
#include "solution.hpp"
#include <unordered_map>
#include <string>
#include <iostream>

struct GA_policy
{
    int numParents;
    int numPopulationReplaced;
};

class GA
{
private:
    std::vector<solution> _population;
    std::unordered_map<std::string, float> _parameters;
    GA_policy _policy; // tracks current algorithm state
    float (*_evalSolution)(solution);
    solution (*_getRandomSolution)();
public:
    GA(float (*evalSolution)(solution),
       solution (*getRandomSolution)(),
       std::unordered_map<std::string, float> parameters)
    {
        std::srand(std::time(NULL)); // seed the random number generator using current time
        _evalSolution = evalSolution;
        _getRandomSolution = getRandomSolution;
        _parameters = parameters;
        _policy = {
            static_cast<int>(_parameters["initialParentCount"]),
            static_cast<int>(_parameters["initialPopulationReplaced"])
        };
    };

    void generateInitialPopulation();

    void evalPopulation(std::shared_ptr<std::vector<solution>> population);

    std::vector<int> getParents(int numParents); // get index of parents in curr population

    std::vector<solution> getNewPopulation(int numNewPopulation); // breed and mutate

    void updatePopulation(std::vector<solution> newPopulation); // using curr and new population

    bool terminateSearch(); // determine whether search should terminate

    void optimise();
};