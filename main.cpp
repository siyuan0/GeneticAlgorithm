#include <iostream>
#include <fstream>
#include <string>
#include <unordered_map>
#include "lib/core.hpp"
#include "third_party/nlohmann/json.hpp"
#include "Example/SchwefelFunction/problem.hpp"

int main(int argc, 
         char *argv[]) {
    if(argc<=1)
    {
        std::cout << "missing paramters.json file\n";
    }else if(argc==2)
    {
        std::ifstream f(argv[1]);
        nlohmann::json data = nlohmann::json::parse(f);
        auto jmap = data.get<std::unordered_map<std::string, float>>();

        GA<Schwefel::soln> GAinst(Schwefel::problemCtx, jmap);
        GAinst.optimise();
        GAinst.printToFile("population.txt");
    }else
    {
        std::cout << "too many arguments\n";
    }

    return 0;
}