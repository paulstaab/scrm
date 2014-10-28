/*
 * scrm is an implementation of the Sequential-Coalescent-with-Recombination Model.
 * 
 * Copyright (C) 2013, 2014 Paul R. Staab, Sha (Joe) Zhu and Gerton Lunter
 * 
 * This file is part of scrm.
 * 
 * scrm is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.

*/


#include "param.h"

Param::Param(const std::string &arg) { 
  std::istringstream iss(arg);

  std::string token;
  char *tmp;
  while(iss >> token) {
    tmp = new char[token.size() + 1];
    copy(token.begin(), token.end(), tmp);
    tmp[token.size()] = '\0';
    argv_vec_.push_back(tmp);
  }
  argv_vec_.push_back(0);

  directly_called_ = true;
  argv_ = &argv_vec_[0];
  argc_ = argv_vec_.size() - 1;
  init();
}

std::ostream& operator<< (std::ostream& stream, const Param& param) {
  stream << "scrm";
  for (int i = 1; i < param.argc_; ++i) {
    stream << " " << param.argv_[i];
  }
  return stream;
}

/*! 
 * \brief Read in ms parameters and jonvert to scrm parameters
 * The first parameters followed by -eG, -eg, -eN, -en, -em, -ema, -es 
 * and -ej options in ms are time t in unit of 4N_0 generations. 
 * In scrm, we define time t in number of generations. 
 */
void Param::parse(Model &model) {
  model = Model();

  size_t sample_size = 0;
  double par_bool = 0.0;
  double time = 0.0;
  size_t source_pop, sink_pop;

  // Placeholders for summary statistics.
  // Statistics are added only after all parameters are parse, so that they will
  // be added in the correct order.
  SegSites* seg_sites = NULL;
  bool tmrca = false,
       newick_trees = false,
       orientedForest = false,
       sfs = false; 


  // The minimal time at which -eM, -eN, -eG, -eI, -ema and -es are allowed to happen. Is
  // increased by using -es.
  double min_time = 0.0;

  std::string argv_i = "";
  argc_i = 0;

  if (argc_ == 0) return;
  if (!directly_called_) {
    dout << "Indirectly called" << std::endl;
  } else {
    // Check that have have at least one argument
    if (argc_ == 1) throw std::invalid_argument("To few command line arguments.");

    // Check if we need to print the help & version (only valid one argument commands)
    argv_i = argv_[1];
    if (argv_i == "-h" || argv_i == "--help") {
      this->set_help(true);
      return;
    }
    if (argv_i == "-v" || argv_i == "--version") {
      this->set_version(true);
      return;
    }

    // Check that have have at least two arguments
    if (argc_ == 2) throw std::invalid_argument("To few command line arguments.");
  }

  while( argc_i < argc_ ){
    argv_i = argv_[argc_i];

    if (argc_i == 0) {
      sample_size = readNextInput<size_t>();
      model.set_loci_number(readNextInput<size_t>());
    }

    // ------------------------------------------------------------------
    // Mutation 
    // ------------------------------------------------------------------
    else if (argv_i == "-t" || argv_i == "-st") {
      // Position
      if (argv_i == "-st") time = readNextInput<double>(); 
      else time = 0.0;

      model.setMutationRate(readNextInput<double>(), true, true, time);
      if (directly_called_ && seg_sites == NULL){
        seg_sites = new SegSites();
      }
    }

    // ------------------------------------------------------------------
    // Recombination 
    // ------------------------------------------------------------------
    else if (argv_i == "-r") {
      par_bool = readNextInput<double>();
      model.setLocusLength(readNextInput<size_t>());
      model.setRecombinationRate(par_bool, true, true, 0.0);
    }

    else if (argv_i == "-sr") {
      time = readNextInput<double>(); // Position
      model.setRecombinationRate(readNextInput<double>(), true, true, time);
    }

    // ------------------------------------------------------------------
    // Subpopulations 
    // ------------------------------------------------------------------
    // Set number of subpopulations and samples at time 0
    else if (argv_i == "-I") {
      model.set_population_number(readNextInput<size_t>());
      std::vector<size_t> sample_size;
      for (size_t i = 0; i < model.population_number(); ++i) {
        sample_size.push_back(readNextInput<size_t>());
      }
      model.addSampleSizes(0.0, sample_size);
      // there might or might not follow a symmetric migration rate
      try {
        model.addSymmetricMigration(0.0, readNextInput<double>()/(model.population_number()-1), true, true);
      } catch (std::invalid_argument e) {
        --argc_i;
      }
    }

    // Add samples at arbitrary times
    else if (argv_i == "-eI") {
      time = readNextInput<double>();
      if (time < min_time) {
        throw std::invalid_argument(std::string("If you use '-eI' in a model with population merges ('-es'),") +
                                    std::string("then you need to sort both arguments by the time."));
      }
      std::vector<size_t> sample_size;
      for (size_t i = 0; i < model.population_number(); ++i) {
        sample_size.push_back(readNextInput<size_t>());
      }
      model.addSampleSizes(time, sample_size, true);
    }

    // ------------------------------------------------------------------
    // Populations sizes 
    // ------------------------------------------------------------------
    else if (argv_i == "-eN" || argv_i == "-N") {
      if (argv_i == "-eN") time = readNextInput<double>();
      else time = 0.0;
      if (time < min_time) {
        throw std::invalid_argument(std::string("If you use '-N' or '-eN' in a model with population merges ('-es'),") +
                                    std::string("then you need to sort both arguments by time."));
      }
      model.addPopulationSizes(time, readNextInput<double>(), true, true); 
      if (time != 0.0) model.addGrowthRates(time, 0.0, true);
    }

    else if (argv_i == "-en" || argv_i == "-n") {
      if (argv_i == "-en") time = readNextInput<double>();
      else time = 0.0;
      size_t pop = readNextInput<size_t>() - 1;
      model.addPopulationSize(time, pop, readNextInput<double>(), true, true);
      if (time != 0.0) model.addGrowthRate(time, pop, 0.0, true);
    }

    // ------------------------------------------------------------------
    // Exponential Growth 
    // ------------------------------------------------------------------
    else if (argv_i == "-G" || argv_i == "-eG") {
      if (argv_i == "-eG") time = readNextInput<double>();
      else time = 0.0;
      if (time < min_time) {
        throw std::invalid_argument(std::string("If you use '-G' or '-eG' in a model with population merges ('-es'),") +
                                    std::string("then you need to sort both arguments by time."));
      }
      model.addGrowthRates(time, readNextInput<double>(), true, true); 
    }

    else if (argv_i == "-g" || argv_i == "-eg") {
      if (argv_i == "-eg") time = readNextInput<double>();
      else time = 0.0;
      size_t pop = readNextInput<size_t>() - 1;
      model.addGrowthRate(time, pop, readNextInput<double>(), true, true); 
    }

    // ------------------------------------------------------------------
    // Migration
    // ------------------------------------------------------------------
    else if (argv_i == "-ma" || argv_i == "-ema") {
      if (argv_i == "-ema") {
        time = readNextInput<double>();
      }
      else time = 0.0;
      if (time < min_time) {
        throw std::invalid_argument(std::string("If you use '-ma' or '-ema' in a model with population merges ('-es'),") +
                                    std::string("then you need to sort both arguments by time."));
      }
      std::vector<double> migration_rates;
      for (size_t i = 0; i < model.population_number(); ++i) {
        for (size_t j = 0; j < model.population_number(); ++j) {
          if (i==j) {
            migration_rates.push_back(0.0);
            ++argc_i;
          }
          else migration_rates.push_back(readNextInput<double>());
        }
      }
      model.addMigrationRates(time, migration_rates, true, true);
    }

    else if (argv_i == "-m" || argv_i == "-em") {
      if (argv_i == "-em") {
        time = readNextInput<double>();
      }
      else time = 0.0;
      if (time < min_time) {
        throw std::invalid_argument(std::string("If you use '-m' or '-em' in a model with population merges ('-es'),") +
                                    std::string("then you need to sort both arguments by time."));
      }
      source_pop = readNextInput<size_t>() - 1;
      sink_pop = readNextInput<size_t>() - 1;

      model.addMigrationRate(time, source_pop, sink_pop, readNextInput<double>(), true, true);
    }

    else if (argv_i == "-M" || argv_i == "-eM") {
      if (argv_i == "-eM") {
        time = readNextInput<double>();
      }
      else time = 0.0;
      if (time < min_time) {
        throw std::invalid_argument(std::string("If you use '-M' or '-eM' in a model with population merges ('-es'),") +
                                    std::string("then you need to sort both arguments by time."));
      }
      model.addSymmetricMigration(time, readNextInput<double>()/(model.population_number()-1), true, true);
    }

    // ------------------------------------------------------------------
    // Population merges
    // ------------------------------------------------------------------
    else if (argv_i == "-es") {
      time = readNextInput<double>();
      if (time < min_time) {
        throw std::invalid_argument(std::string("You must sort multiple population merges ('-es'),") +
                                    std::string("by the time they occur."));
      }
      min_time = time;
      source_pop = readNextInput<size_t>() - 1;
      sink_pop = model.population_number();
      double fraction = readNextInput<double>();

      model.addPopulation();
      model.addSingleMigrationEvent(time, source_pop, sink_pop, fraction, true); 
    }


    // ------------------------------------------------------------------
    // Population splits
    // ------------------------------------------------------------------
    else if (argv_i == "-ej") {
      time = readNextInput<double>();
      source_pop = readNextInput<size_t>() - 1;
      sink_pop = readNextInput<size_t>() - 1;
      model.addSingleMigrationEvent(time, source_pop, sink_pop, 1.0, true); 
      for (size_t i = 0; i < model.population_number(); ++i) {
        if (i == source_pop) continue;
        model.addMigrationRate(time, i, source_pop, 0.0, true);
      }
    }

    // ------------------------------------------------------------------
    // Pruning 
    // ------------------------------------------------------------------
    else if (argv_i == "-l"){
      model.set_exact_window_length(readNextInput<size_t>());
    }

    // ------------------------------------------------------------------
    // Read initial trees from file
    // ------------------------------------------------------------------
    else if ( argv_i == "-init" ){
      this->read_init_genealogy_ = true;
      std::string tmp_string = readNextInput<std::string>();
      std::ifstream in_file( tmp_string.c_str() );
      std::string gt_str;
      if ( in_file.good() ){
        getline ( in_file, gt_str );
        while ( gt_str.size() > 0 ){
          init_genealogy.push_back( gt_str );
          getline ( in_file, gt_str );
        }
      } else {
        throw std::invalid_argument("Invalid input file. " + tmp_string );
      }
      in_file.close();
    }

    // ------------------------------------------------------------------
    // Summary Statistics
    // ------------------------------------------------------------------
    else if (argv_i == "-T") {
      newick_trees = true;
    }

    else if (argv_i == "-O"){
      orientedForest = true;
    }

    else if (argv_i == "-SC" || argv_i == "--SC") {
      std::string tmp_string = readNextInput<std::string>();
      if (tmp_string.compare("rel") == 0) model.setSequenceScaling(relative);
      else if (tmp_string.compare("abs") == 0) model.setSequenceScaling(absolute);
      else if (tmp_string.compare("ms") == 0) model.setSequenceScaling(ms);
      else throw 
        std::invalid_argument(std::string("Unknown sequence scaling argument: ") +
                              tmp_string +
                              std::string(". Valid are 'rel', 'abs' or 'ms'."));
    }

    else if (argv_i == "-L") {
      tmrca = true;
    }

    else if (argv_i == "-oSFS") {
      sfs = true;
    }

    // ------------------------------------------------------------------
    // Seeds
    // ------------------------------------------------------------------
    else if (argv_i == "-seed" || argv_i == "--seed") {
      std::vector<size_t> seeds(3, 0);
      // Always require one seed
      seeds.at(0) = readNextInput<size_t>();
      try {
        // Maybe read in up to 3 seeds (ms compatibility)
        for (size_t i = 1; i < 3; i++) seeds.at(i) = readNextInput<size_t>();
      } catch (std::invalid_argument e) {
        --argc_i;
      }

      if (seeds.at(1) != 0 || seeds.at(2) != 0) {
        // Mangles the seed together into a single int stored in the vectors
        // first entry.
        std::seed_seq{seeds.at(0), seeds.at(1), seeds.at(2)}.
             generate(seeds.begin(), seeds.begin()+1);
      }
      set_random_seed(seeds.at(0));
    }
    

    // ------------------------------------------------------------------
    // Help & Version
    // ------------------------------------------------------------------
    else if (argv_i == "-h" || argv_i == "--help") {
      this->set_help(true);
      return;
    }

    else if (argv_i == "-v" || argv_i == "--version") {
      this->set_version(true);
      return;
    }

    // ------------------------------------------------------------------
    // Unsupported ms arguments
    // ------------------------------------------------------------------
    else if (argv_i == "-c") {
      throw std::invalid_argument("scrm does not support gene conversion.");
    }

    else if (argv_i == "-s") {
      throw std::invalid_argument("scrm does not support simulating a fixed number of mutations.");
    }

    else {  
      throw std::invalid_argument(std::string("unknown/unexpected argument: ") + argv_i);
    }

    ++argc_i;
  }

  if (model.sample_size() == 0) {
    model.addSampleSizes(0.0, std::vector<size_t>(1, sample_size));
  } 
  //else if (model.sample_size() != sample_size && directly_called_) {
    //throw std::invalid_argument("Sum of samples not equal to the total sample size");
  //}
  else if (model.sample_size() != sample_size ) {
    throw std::invalid_argument("Sum of samples not equal to the total sample size");
  }

  // Add summary statistics in order of their output
  if (newick_trees) model.addSummaryStatistic(new NewickTree());
  if (orientedForest) model.addSummaryStatistic(new OrientedForest(model.sample_size()));
  if (tmrca) model.addSummaryStatistic(new TMRCA());
  if (seg_sites != NULL) model.addSummaryStatistic(seg_sites);
  if (sfs) {
    if (seg_sites == NULL) 
      throw std::invalid_argument("You need to give a mutation rate ('-t') to simulate a SFS"); 
    model.addSummaryStatistic(new FrequencySpectrum(seg_sites, model));
  }

  model.finalize();
  model.resetTime();
}

void Param::printHelp(std::ostream& out) {
  out << "scrm - Fast & accurate coalescent simulations" << std::endl; 
  out << "Version " << VERSION << std::endl << std::endl; 

  out << "Usage" << std::endl; 
  out << "--------------------------------------------------------" << std::endl; 
  out << "Call scrm follow by two integers and an arbitrary number" << std::endl; 
  out << "of options described below:" << std::endl; 
  out << std::endl << "  scrm <n_samp> <n_loci> [...]" << std::endl << std::endl; 
  out << "Here, n_samp is the total number of samples and n_loci" << std::endl; 
  out << "is the number of independent loci to simulate." << std::endl; 
  out << std::endl << "Options" << std::endl; 
  out << "--------------------------------------------------------" << std::endl; 
  out << "A detailed description of these options and their parameters" << std::endl; 
  out << "is provided is the manual." << std::endl << std::endl;

  out << "Recombination:" << std::endl;
  out << "  -r <R> <L>       Set recombination rate to R and locus length to L." << std::endl;
  out << "  -sr <p> <R>      Change the recombination rate R at sequence position p." << std::endl;
  out << "  -l <l>           Set the approximation window length to l." << std::endl;

  out << std::endl << "Population Structure:" << std::endl;
  out << "  -I <npop> <s1> ... <sn> [<M>]   Use an island model with npop populations," <<std::endl
      << "                   where s1 to sn individuals are sampled each population." << std::endl
      << "                   Optionally assume a symmetric migration rate of M." << std::endl;
  out << "  -eI <t> <s1> ... <sn> [<M>]     Sample s1 to sn indiviuals from their" << std::endl
      << "                   corresponding populations at time t." << std::endl;
  out << "  -M <M>           Assume a symmetric migration rate of M/(npop-1)." << std::endl;
  out << "  -eM <t> <M>      Change the symmetric migration rate to M/(npop-1) at time t." 
      << std::endl;
  out << "  -m <i> <j> <M>   Set the migration rate from population j to population i to M" 
      << std::endl;
  out << "  -em <t> <i> <j> <M>   Set the migration rate from population j to" << std::endl
      << "                   population i to M at time t." << std::endl;
  out << "  -ma <M11> <M21> ...   Sets the (backwards) migration matrix." << std::endl;
  out << "  -ema <t> <M11> <M21> ...    Changes the migration matrix at time t" << std::endl;
  out << "  -es <t> <i> <p>  Population admixture. Replaces a fraction of 1-p of" << std::endl
      << "                   population i with individuals a from population npop + 1" << std::endl
      << "                   which is ignored afterwards (forward in time). " << std::endl;
  out << "  -ej <t> <i> <j>  Speciation event at time t. Creates population j" << std::endl
      << "                   from individuals of population i." << std::endl;

  out << std::endl << "Population Size Changes:" << std::endl;
  out << "  -n <i> <n>       Set the present day size of population i to n*N0." << std::endl;
  out << "  -en <t> <i> <n>  Change the size of population i to n*N0 at time t." << std::endl;
  out << "  -eN <t> <n>      Set the present day size of all populations to n*N0." << std::endl;
  out << "  -g <i> <a>       Set the exponential growth rate of population i to a." << std::endl;
  out << "  -eg <t> <i> <a>  Change the exponential growth rate of population i to a" << std::endl
      << "                   at time t." << std::endl;
  out << "  -G <a>           Set the exponential growth rate of all populations to a." << std::endl;
  out << "  -eG <t> <a>      Change the exponential growth rate of all populations to a" << std::endl
      << "                   at time t." << std::endl;

  out << std::endl << "Summary Statistics:" << std::endl;
  out << "  -t <theta>       Set the mutation rate to theta = 4N0*mu, where mu is the " << std::endl
      << "                   neutral mutation rate per locus." << std::endl;
  out << "  -T               Print the simulated local genealogies in Newick format." << std::endl;
  out << "  -O               Print the simulated local genealogies in Oriented Forest format." << std::endl;
  out << "  -L               Print the TMRCA and the local tree length for each segment." << std::endl;
  out << "  -oSFS            Print the Site Frequency Spectrum for each locus." << std::endl;
  out << "  -SC [ms|rel|abs] Scaling of sequence positions. Either" << std::endl 
      << "                   relative (rel) to the locus length between 0 and 1," << std::endl 
      << "                   absolute (abs) in base pairs or as in ms (default)." << std::endl;

  out << std::endl << "Other:" << std::endl;
  out << "  -seed <SEED> [<SEED2> <SEED3>]   The random seed to use. Takes up three" << std::endl 
      << "                   integer numbers." << std::endl;
  out << "  -v, --version    Prints the version of scrm." << std::endl;
  out << "  -h, --help       Prints this text." << std::endl;

  out << std::endl << "Examples" << std::endl;
  out << "--------------------------------------------------------" << std::endl;
  out << "Five independent sites for 10 individuals using Kingman's Coalescent:" <<std::endl;
  out << "  scrm 10 5 -t 10" << std::endl << std::endl;

  out << "A sequence of 10kb from 4 individuals under the exact ARG:" <<std::endl; 
  out << "  scrm 4 1 -t 10 -r 4 10000" << std::endl << std::endl;

  out << "A sequence of 100Mb using the SMC' approximation:" << std::endl;
  out << "  scrm 4 1 -t 10 -r 4000 100000000 -l 0" << std::endl << std::endl;

  out << "Same as above, but with essentially correct linkage:" << std::endl;
  out << "  scrm 4 1 -t 10 -r 4000 100000000 -l 100000" << std::endl << std::endl;
}
