#ifndef scrm_src_random_mersenne_twister
#define scrm_src_random_mersenne_twister

#include <vector>
#include <ctime> 
#include <boost/random.hpp>

#include "random_generator.h"

class MersenneTwister : public RandomGenerator
{
  public:
   MersenneTwister();
   MersenneTwister(int seed);
   ~MersenneTwister();
                         
   void initialize() {};
   double sample();

  protected:
   typedef boost::mt19937 rng_type;
   boost::uniform_01<double> unif;
   rng_type rng;
};

#endif
