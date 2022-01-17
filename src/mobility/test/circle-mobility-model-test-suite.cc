/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
* Copyright (c) 2021 Charles Pandian, ProjectGuideline.com
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
 * Author: Charles Pandian<igs3000@gmail.com>
*/

// Include a header file from your module to test.
#include "ns3/circle-mobility-model.h"

// An essential include is test.h
#include "ns3/test.h"
#include <ns3/double.h>
#include "ns3/rng-seed-manager.h"


#include <cmath>
#include "ns3/simulator.h"
#include "ns3/boolean.h"
#include "ns3/double.h"
#include "ns3/config.h"

// Do not put your test classes in namespace ns3.  You may find it useful
// to use the using directive to access the ns3 namespace directly
using namespace ns3;

/**
* CircleMobilityModelTestCase1 will be used to test the CircleMobilityModel
*
*
* @section DESCRIPTION
*
* The CircleMobilityModelTestCase1 inherited from TestCase.
*/
class CircleMobilityModelTestCase1 : public TestCase
{
public:
  CircleMobilityModelTestCase1 ();
  virtual ~CircleMobilityModelTestCase1 ();

private:
  std::vector<Ptr<MobilityModel> > mobilityStack; ///< modility model
  double count; ///< count
  virtual void DoRun (void);
  void DistXCompare (double);
};

// Add some help text to this case to describe what it is intended to test
CircleMobilityModelTestCase1::CircleMobilityModelTestCase1 ()
  : TestCase ("circlemobilitymodel test case (does nothing)")
{
}

// This destructor does nothing but we include it as a reminder that
// the test case should clean up after itself
CircleMobilityModelTestCase1::~CircleMobilityModelTestCase1 ()
{
}

//
// This method is the pure virtual method from class TestCase that every
// TestCase must implement
// Tom recommended to define a few test cases once it is decided what parameters can be changed at runtime.  
// An obviously simple initial test would be to check the case in which there are no runtime parameter changes, 
// and then consider the origin to be 0,0,0, radius to be 1, and speed to be 2*pi m/s, and initial angle to be 0, 
// and then check the position at time 0.5 second (should be -1, 0, 0) and 1 second (should be 1, 0, 0).  
// Of course, since PI is irrational, we need to check these x-coordinates with some tolerance value.

//
void
CircleMobilityModelTestCase1::DoRun (void)
{
  SeedManager::SetSeed (123);

  // Total simulation time, seconds
  double totalTime = 100;
  double speed = 2.0*(22.0/7.0);
  ObjectFactory mobilityFactory;
  mobilityFactory.SetTypeId ("ns3::CircleMobilityModel");
  mobilityFactory.Set ("UseConfiguredOrigin", BooleanValue (true));
  mobilityFactory.Set ("MinOrigin", Vector3DValue (Vector3D (0, 0, 0)));
  mobilityFactory.Set ("MaxOrigin", Vector3DValue (Vector3D (0, 0, 0)));
  mobilityFactory.Set ("MinMaxRadius", Vector2DValue (Vector2D (1,1))); 
  mobilityFactory.Set ("MinMaxStartAngle", Vector2DValue (Vector2D (0, 0))); 
  mobilityFactory.Set ("MinMaxSpeed", Vector2DValue (Vector2D (speed,speed)));
  mobilityFactory.Set ("RandomizeDirection", BooleanValue (false));
  mobilityFactory.Set ("Clockwise", BooleanValue (false));

  // Populate the vector of mobility models.
  count = 10;
  for (uint32_t i = 0; i < count; i++)
    {
      // Create a new mobility model.
      Ptr<MobilityModel> model = mobilityFactory.Create ()->GetObject<MobilityModel> ();
      //model->AssignStreams (100 * (i + 1));
      // Add this mobility model to the stack.
      mobilityStack.push_back (model);
      Simulator::Schedule (Seconds (0.0), &Object::Initialize, model);
    } 

  Simulator::Schedule (Seconds (0.5), &CircleMobilityModelTestCase1::DistXCompare, this,(double) -1);
  Simulator::Schedule (Seconds (1.0), &CircleMobilityModelTestCase1::DistXCompare, this,(double) 1); 
  Simulator::Stop (Seconds (totalTime));
  Simulator::Run ();
  Simulator::Destroy ();
}

/**
* CircleMobilityModelTestCase1::DistXCompare will be used to compare the movements of the mobility model
* at a particular time with the calculated values
*
 * \ingroup mobility-test
 * \ingroup tests
 * @param par is the predicted displacement in x direction.
 * \brief DistXCompare will copare the x-displacement of a node provided by the mobility model with the calculated values 
 */
void
CircleMobilityModelTestCase1::DistXCompare (double par)
{
  double sum_x = 0;
  Vector vv;
  std::vector<Ptr<MobilityModel> >::iterator i;
  Ptr<MobilityModel> model;
  for (i = mobilityStack.begin (); i != mobilityStack.end (); ++i)
    {
      model = (*i);
      vv=model->GetPosition();
      sum_x += vv.x;
    }
  double mean_x = sum_x / count;

  NS_TEST_ASSERT_MSG_EQ_TOL (mean_x, par, 0.1, "Numbers are not equal within tolerance");
}
/**
* CircleMobilityModelTestSuite class is the TestSuite for CircleMobilityModel
* and it enables the TestCases to be run.
 * \ingroup mobility-test
 * \ingroup tests
 *
 * \brief Circle Mobility Model Test Suite
 */
struct CircleMobilityModelTestSuite : public TestSuite
{
  CircleMobilityModelTestSuite () : TestSuite ("CircleMobilityModel", UNIT)
  {
    AddTestCase (new CircleMobilityModelTestCase1, TestCase::QUICK);
  }
} g_circleMobilityModelTestSuite; ///< the test suite

