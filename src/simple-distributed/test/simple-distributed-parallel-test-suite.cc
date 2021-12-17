/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 *  Copyright 2020. Lawrence Livermore National Security, LLC.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author: Steven G. Smith <smith84@llnl.gov>
 */

#include "ns3/example-as-test.h"

#include <sstream>

using namespace ns3;

/**
 * This version of ns3::ExampleTestCase is specialized for 
 * by accepting the number of ranks as a parameter,
 * then building a `--command-template` string which
 * invokes `mpiexec` correctly to execute MPI examples.
 */
class SimpleDistributedParallelTestCase : public ExampleAsTestCase
{
public:
  /**
   * \copydoc ns3::ExampleTestCase::ExampleTestCase
   *
   * \param [in] ranks The number of ranks to use
   */
  SimpleDistributedParallelTestCase (const std::string name,
               const std::string program,
               const std::string dataDir,
               const int ranks,
               const std::string args = "");

  /** Destructor */
  virtual ~SimpleDistributedParallelTestCase (void) {};

  /**
   * Produce the `--command-template` argument which will invoke
   * `mpiexec` with the requested number of ranks.
   *
   * \returns The `--command-template` string.
   */
  std::string GetCommandTemplate (void) const;

  /**
   * Sort the output from parallel execution.
   * stdout from multiple ranks is not ordered.
   *
   * \returns Sort command
   */
  std::string
  GetPostProcessingCommand (void) const;
  
private:
  /** The number of ranks. */
  int m_ranks;            
};

SimpleDistributedParallelTestCase::SimpleDistributedParallelTestCase (const std::string name,
                          const std::string program,
                          const std::string dataDir,
                          const int ranks,
                          const std::string args /* = "" */)
  : ExampleAsTestCase (name, program, dataDir, args),
    m_ranks (ranks)
{
}

std::string
SimpleDistributedParallelTestCase::GetCommandTemplate (void) const
{
#ifdef NS3_MPI
  std::stringstream ss;
  ss << "mpiexec -n " << m_ranks  << " %s " << m_args;
  return ss.str ();
#else
  return ExampleAsTestCase::GetCommandTemplate ();
#endif
}

std::string
SimpleDistributedParallelTestCase::GetPostProcessingCommand (void) const
{
  std::string command ("| grep TEST | sort ");
  return command;
}

/**
 * \ingroup network-test
 * \ingroup tests
 *
 * \brief Parallel SimpleDistributedNetDevice TestSuite
 *
 * Runs scaling example in parallel over several parameter configurations.
 */
class SimpleDistributedParallelTestSuite : public TestSuite
{
public:
  /**
   * 
   */
  SimpleDistributedParallelTestSuite ()
  : TestSuite ("simple-distributed-parallel", UNIT)
  {
    const TestDuration duration=QUICK;
    std::string program = "simple-distributed-channel-scaling";
    const int gridSize = 10;
    std::vector<int> ranks { 1, 2, 4 };

    // Checks that distance cutoff is working in parallel.
    {
      std::vector<int> communicationPatterns { 0, 1, 2 };
      std::vector<double> distances { 2, 5, 10, 100 };

      for(auto rank : ranks)
        {
          for(auto communicationPattern : communicationPatterns)
            {
              for(auto distance : distances)
                {
                  std::stringstream args;
                  args << "--grid-size=" << gridSize << " --communication-pattern=" << communicationPattern << " --distance=" << distance;
                  
                  std::stringstream name;
                  name << "simple-distributed-channel-scaling-" << rank << "-" << communicationPattern << "-" << distance;
                  
                  AddTestCase (new SimpleDistributedParallelTestCase (name.str(), program, NS_TEST_SOURCEDIR, rank, args.str()), duration);
                }
            }
        }
    }

    // Checks that error model is working in parallel.
    {
      for(auto rank : ranks)
        {
          int communicationPattern = 2; // Broadcast from node 0
          double distance = 10.0;
          std::stringstream args;
          args << "--grid-size=" << gridSize << " --communication-pattern=" << communicationPattern << " --corruption-distance=" << distance;
                  
          std::stringstream name;
          name << "simple-distributed-channel-scaling-" << rank << "-" << communicationPattern << "-" << distance;
          
          AddTestCase (new SimpleDistributedParallelTestCase (name.str(), program, NS_TEST_SOURCEDIR, rank, args.str()), duration);
        }
    }
  }
};  // class MpiTestSuite

static SimpleDistributedParallelTestSuite g_simpleDistributedParallelTestSuite; //!< Static variable for test initialization
