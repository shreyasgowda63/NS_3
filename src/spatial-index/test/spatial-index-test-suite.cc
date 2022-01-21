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
using PointerType = ns3::SpatialIndexing::PointerType;

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
  : TestCase ("SpatialIndex test verifies that GetNodesInRange and associated methods work properly")
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
  PointerType node1 = CreateObject<Node>();
  PointerType node2 = CreateObject<Node>();
  PointerType node3 = CreateObject<Node>();
  PointerType node4 = CreateObject<Node>();
  PointerType node5 = CreateObject<Node>();

  spatialIndex->Add(node1, Vector(0,0,0));
  spatialIndex->Add(node2, Vector(1000,0,0));
  Vector n3pos = Vector(500,0,0);
  spatialIndex->Add(node3, n3pos);
  spatialIndex->Add(node4, Vector(1500,0,0)); // out of range
  spatialIndex->Add(node5, Vector(1000, 500, 0.0)); //intentionally in square but outside of circle

  auto nodes = spatialIndex->GetNodesInRange(500,n3pos, node2);
  NS_TEST_ASSERT_MSG_EQ (nodes.size(), 3, "Expected exactly 3 nodes to be in range (including self)");
  auto found = 0u;
  for (auto &N: nodes) {
    if (N == node1) found++;
    if (N == node2) found++;
    if (N == node3) found++;
    NS_TEST_ASSERT_MSG_NE(N,node4,"Unexpected node in results");
    NS_TEST_ASSERT_MSG_NE(N,node5,"Unexpected node in results");
  }
  NS_TEST_ASSERT_MSG_EQ(found,3,"The correct nodes were not found");

  //set range to 0
  nodes = spatialIndex->GetNodesInRange(1.0, n3pos, node3);
  NS_TEST_ASSERT_MSG_EQ (nodes.size(), 1, "Expected 1 node to be in 0 range");
  found = 0u;
  for (auto &N: nodes) {
    NS_TEST_ASSERT_MSG_NE(N,node1,"Unexpected node in results");
    NS_TEST_ASSERT_MSG_NE(N,node2,"Unexpected node in results");
    if (N == node3) found++;
    NS_TEST_ASSERT_MSG_NE(N,node4,"Unexpected node in results");
    NS_TEST_ASSERT_MSG_NE(N,node5,"Unexpected node in results");
  }
  NS_TEST_ASSERT_MSG_EQ(found,1,"The correct nodes were not found");

  spatialIndex->Update(node2, Vector(1001,0,0)); //moved out of range
  nodes = spatialIndex->GetNodesInRange(500.1, n3pos, node3);
  NS_TEST_ASSERT_MSG_EQ (nodes.size(), 2, "Expected 2 nodes to be in range (including self)");
  found = 0u;
  for (auto &N: nodes) {
    if (N == node1) found++;
    NS_TEST_ASSERT_MSG_NE(N,node2,"Unexpected node in results");
    if (N == node3) found++;
    NS_TEST_ASSERT_MSG_NE(N,node4,"Unexpected node in results");
    NS_TEST_ASSERT_MSG_NE(N,node5,"Unexpected node in results");
  }
  NS_TEST_ASSERT_MSG_EQ(found,2,"The correct nodes were not found");

  spatialIndex->Remove(node1);
  nodes = spatialIndex->GetNodesInRange(500.0, n3pos, node3);
  NS_TEST_ASSERT_MSG_EQ (nodes.size(), 1, "Expected 1 node to be in range (only self)");
  found = 0u;
  for (auto &N: nodes) {
    NS_TEST_ASSERT_MSG_NE(N,node1,"Unexpected node in results");
    NS_TEST_ASSERT_MSG_NE(N,node2,"Unexpected node in results");
    if (N == node3) found++;
    NS_TEST_ASSERT_MSG_NE(N,node4,"Unexpected node in results");
    NS_TEST_ASSERT_MSG_NE(N,node5,"Unexpected node in results");
  }
  NS_TEST_ASSERT_MSG_EQ(found,1,"The correct nodes were not found");
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
  : TestCase ("SpatialIndex test verifies that GetNodesInRange and associated methods work properly for BruteForce")
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
  PointerType node1 = CreateObject<Node>();
  PointerType node2 = CreateObject<Node>();
  PointerType node3 = CreateObject<Node>();
  PointerType node4 = CreateObject<Node>();
  spatialIndex->Add(node1, Vector(0,0,0));
  spatialIndex->Add(node2, Vector(1000,0,0));
  Vector n3pos = Vector(500,0,0);
  spatialIndex->Add(node3, n3pos);
  spatialIndex->Add(node4, Vector(1001,0,0)); // out of range

  std::vector<PointerType > nodes = spatialIndex->GetNodesInRange(500.0, n3pos, node3);
  NS_TEST_ASSERT_MSG_EQ (nodes.size(), 3, "Expected 3 nodes to be in range (including self)");

  //set range to 0
  nodes = spatialIndex->GetNodesInRange(0.0, n3pos, node3);
  NS_TEST_ASSERT_MSG_EQ (nodes.size(), 1, "Expected 1 nodes to be in 0 range (only self)");

  spatialIndex->Update(node2, Vector(1002,0,0)); //moved out of range
  nodes = spatialIndex->GetNodesInRange(500.0, n3pos, node3);
  NS_TEST_ASSERT_MSG_EQ (nodes.size(), 2, "Expected 2 nodes to be in range (including self)");

  spatialIndex->Remove(node1); //Remove last one in range
  nodes = spatialIndex->GetNodesInRange(500.0, n3pos, node3);
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

