/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright 2020 National Technology & Engineering Solutions of Sandia,
 * LLC (NTESS). Under the terms of Contract DE-NA0003525 with NTESS,
 * the U.S. Government retains certain rights in this software.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301,
 * USA.
 *
 */

// Include a header file from your module to test.
#include "ns3/spatial-index.h"
#include "ns3/kdtree-index.h"
#include "ns3/bruteforce.h"

// An essential include is test.h
#include "ns3/test.h"

// Do not put your test classes in namespace ns3.  You may find it useful
// to use the using directive to access the ns3 namespace directly
using namespace ns3;

/**
 * \brief First test case for spatial indexing
 */
class SpatialIndexTestCase1 : public TestCase
{
public:
  SpatialIndexTestCase1 ();
  virtual ~SpatialIndexTestCase1 ();

private:
  virtual void DoRun (void);
};

// Add some help text to this case to describe what it is intended to test
SpatialIndexTestCase1::SpatialIndexTestCase1 ()
  : TestCase ("SpatialIndex test verifies that getNodesInRange and associated methods work properly")
{
}

// Destructor
SpatialIndexTestCase1::~SpatialIndexTestCase1 ()
{
}

//
// This method is the pure virtual method from class TestCase that every
// TestCase must implement
//
void
SpatialIndexTestCase1::DoRun (void)
{
  Ptr<SpatialIndexing> spatialIndex = new KDTreeSpatialIndexing();
  Ptr<Node> node1 = CreateObject<Node>();
  Ptr<Node> node2 = CreateObject<Node>();
  Ptr<Node> node3 = CreateObject<Node>();
  Ptr<Node> node4 = CreateObject<Node>();
  Ptr<Node> node5 = CreateObject<Node>();

  spatialIndex->add(node1, Vector(0,0,0));
  spatialIndex->add(node2, Vector(1000,0,0));
  Vector n3pos = Vector(500,0,0);
  spatialIndex->add(node3, n3pos);
  spatialIndex->add(node4, Vector(1500,0,0)); // out of range
  spatialIndex->add(node5, Vector(1000, 500, 0.0)); //intentionally in square but outside of circle

  auto nodes = spatialIndex->getNodesInRange(500.0, n3pos, node3);
  NS_TEST_ASSERT_MSG_EQ (nodes.size(), 3, "Expected exactly 3 nodes to be in range (including self)");

  //set range to 0
  nodes = spatialIndex->getNodesInRange(1.0, n3pos, node3);
  NS_TEST_ASSERT_MSG_EQ (nodes.size(), 1, "Expected 1 node to be in 0 range");
  spatialIndex->update(node2, Vector(1001,0,0)); //moved out of range
  nodes = spatialIndex->getNodesInRange(500.0, n3pos, node3);
  NS_TEST_ASSERT_MSG_EQ (nodes.size(), 2, "Expected 2 nodes to be in range (including self)");

  spatialIndex->remove(node1);
  nodes = spatialIndex->getNodesInRange(500.0, n3pos, node3);
  NS_TEST_ASSERT_MSG_EQ (nodes.size(), 1, "Expected 1 node to be in range (only self)");
}

/**
 * \brief Test suite for brute force spatial indexing (clipping)
 */
class SpatialIndexTestCaseBruteForce : public TestCase
{
public:
  SpatialIndexTestCaseBruteForce ();
  virtual ~SpatialIndexTestCaseBruteForce ();

private:
  virtual void DoRun (void);
};

SpatialIndexTestCaseBruteForce::SpatialIndexTestCaseBruteForce ()
  : TestCase ("SpatialIndex test verifies that getNodesInRange and associated methods work properly for BruteForce")
{
}

// Destructor
SpatialIndexTestCaseBruteForce::~SpatialIndexTestCaseBruteForce ()
{
}

//
// This method is the pure virtual method from class TestCase that every
// TestCase must implement
//
void
SpatialIndexTestCaseBruteForce::DoRun (void)
{
  Ptr<SpatialIndexing> spatialIndex = new BruteForceSpatialIndexing();
  Ptr<Node> node1 = CreateObject<Node>();
  Ptr<Node> node2 = CreateObject<Node>();
  Ptr<Node> node3 = CreateObject<Node>();
  Ptr<Node> node4 = CreateObject<Node>();
  spatialIndex->add(node1, Vector(0,0,0));
  spatialIndex->add(node2, Vector(1000,0,0));
  Vector n3pos = Vector(500,0,0);
  spatialIndex->add(node3, n3pos);
  spatialIndex->add(node4, Vector(1001,0,0)); // out of range

  std::vector<Ptr<const Node> > nodes = spatialIndex->getNodesInRange(500.0, n3pos, node3);
  NS_TEST_ASSERT_MSG_EQ (nodes.size(), 3, "Expected 3 nodes to be in range (including self)");

  //set range to 0
  nodes = spatialIndex->getNodesInRange(0.0, n3pos, node3);
  NS_TEST_ASSERT_MSG_EQ (nodes.size(), 1, "Expected 1 nodes to be in 0 range (only self)");

  spatialIndex->update(node2, Vector(1002,0,0)); //moved out of range
  nodes = spatialIndex->getNodesInRange(500.0, n3pos, node3);
  NS_TEST_ASSERT_MSG_EQ (nodes.size(), 2, "Expected 2 nodes to be in range (including self)");

  spatialIndex->remove(node1); //remove last one in range
  nodes = spatialIndex->getNodesInRange(500.0, n3pos, node3);
  NS_TEST_ASSERT_MSG_EQ (nodes.size(), 1, "Expected 1 node to be in range (only self)");
}


/**
 * \brief Test suite for spatial indexing
 */
class SpatialIndexTestSuite : public TestSuite
{
public:
  SpatialIndexTestSuite ();
};

SpatialIndexTestSuite::SpatialIndexTestSuite ()
  : TestSuite ("spatial-index", UNIT)
{
  // TestDuration for TestCase can be QUICK, EXTENSIVE or TAKES_FOREVER
  AddTestCase (new SpatialIndexTestCase1, TestCase::QUICK);
  AddTestCase (new SpatialIndexTestCaseBruteForce, TestCase::QUICK);
}

// Do not forget to allocate an instance of this TestSuite
static SpatialIndexTestSuite spatialIndexTestSuite;

