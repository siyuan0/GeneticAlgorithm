#include <vector>
#include <memory>
#include <ctime>
#include <cstdlib>
#include "solution.hpp"

class GA
{
private:
    std::shared_ptr<std::vector<solution>> _population;
    float (*_evalSolution)(solution);
    solution (*_getRandomSolution)();
public:
    GA(float (*evalSolution)(solution),
       solution (*getRandomSolution)())
    {
        std::srand(std::time(NULL)); // seed the random number generator using current time
        _evalSolution = evalSolution;
        _getRandomSolution = getRandomSolution;
    };

    void generateInitialPopulation();

    void evalPopulation(std::shared_ptr<std::vector<solution>> population);

    void getParents();

    void getNewPopulation(); // breed and mutate

    void updatePopulation(); // using curr and new population

    void optimise();

    void optimise(std::shared_ptr<std::vector<solution>> initial_population); // overload for provided initial population
};