/**
 * @file    testBayesTree.cpp
 * @brief   Unit tests for Bayes Tree
 * @author  Frank Dellaert
 */

#include <boost/assign/std/list.hpp> // for operator +=
using namespace boost::assign;

#include <CppUnitLite/TestHarness.h>

#include "SymbolicBayesNet.h"
#include "GaussianBayesNet.h"
#include "Ordering.h"
#include "BayesTree-inl.h"
#include "smallExample.h"

using namespace gtsam;

typedef BayesTree<ConditionalGaussian> Gaussian;

// Conditionals for ASIA example from the tutorial with A and D evidence
SymbolicConditional::shared_ptr B(new SymbolicConditional("B")), L(
		new SymbolicConditional("L", "B")), E(
		new SymbolicConditional("E", "L", "B")), S(new SymbolicConditional("S",
		"L", "B")), T(new SymbolicConditional("T", "E", "L")), X(
		new SymbolicConditional("X", "E"));

/* ************************************************************************* */
TEST( BayesTree, Front )
{
	SymbolicBayesNet f1;
	f1.push_back(B);
	f1.push_back(L);
	SymbolicBayesNet f2;
	f2.push_back(L);
	f2.push_back(B);
	CHECK(f1.equals(f1));
	CHECK(!f1.equals(f2));
}

/* ************************************************************************* */
TEST( BayesTree, constructor )
{
	// Create using insert
	BayesTree<SymbolicConditional> bayesTree;
	bayesTree.insert(B);
	bayesTree.insert(L);
	bayesTree.insert(E);
	bayesTree.insert(S);
	bayesTree.insert(T);
	bayesTree.insert(X);

	// Check Size
	LONGS_EQUAL(6,bayesTree.size());

	// Check root
	BayesNet<SymbolicConditional> expected_root;
	expected_root.push_back(E);
	expected_root.push_back(L);
	expected_root.push_back(B);
	boost::shared_ptr<BayesNet<SymbolicConditional> > actual_root = bayesTree.root();
	CHECK(assert_equal(expected_root,*actual_root));

	// Create from symbolic Bayes chain in which we want to discover cliques
	SymbolicBayesNet ASIA;
	ASIA.push_back(X);
	ASIA.push_back(T);
	ASIA.push_back(S);
	ASIA.push_back(E);
	ASIA.push_back(L);
	ASIA.push_back(B);
	BayesTree<SymbolicConditional> bayesTree2(ASIA);
	//bayesTree2.print("bayesTree2");

	// Check whether the same
	CHECK(assert_equal(bayesTree,bayesTree2));
}

/* ************************************************************************* *
 Bayes tree for smoother with "natural" ordering:
C1 x6 x7
C2   x5 : x6
C3     x4 : x5
C4       x3 : x4
C5         x2 : x3
C6           x1 : x2
/* ************************************************************************* */
TEST( BayesTree, smoother )
{
	// Create smoother with 7 nodes
	LinearFactorGraph smoother = createSmoother(7);
	Ordering ordering;
	for (int t = 1; t <= 7; t++)
		ordering.push_back(symbol('x', t));

	// eliminate using the "natural" ordering
	GaussianBayesNet::shared_ptr chordalBayesNet = smoother.eliminate(ordering);

	// Create the Bayes tree
	Gaussian bayesTree(*chordalBayesNet);
	LONGS_EQUAL(7,bayesTree.size());

	// Get the conditional P(S6|Root)
  Vector sigma1 = repeat(2, 0.786153);
	ConditionalGaussian::shared_ptr cg(new ConditionalGaussian("x2", zero(2), eye(2), sigma1));
	BayesNet<ConditionalGaussian> expected; expected.push_back(cg);
	Gaussian::sharedClique C6 = bayesTree["x1"];
	Gaussian::sharedBayesNet actual = C6->shortcut();
	//CHECK(assert_equal(expected,*actual,1e-4));
}

/* ************************************************************************* *
 Bayes tree for smoother with "nested dissection" ordering:
 x5 x6 x4
   x3 x2 : x4
     x1 : x2
   x7 : x6
/* ************************************************************************* */
TEST( BayesTree, balanced_smoother_marginals )
{
	// Create smoother with 7 nodes
	LinearFactorGraph smoother = createSmoother(7);
	Ordering ordering;
	ordering += "x1","x3","x5","x7","x2","x6","x4";

	// eliminate using a "nested dissection" ordering
	GaussianBayesNet::shared_ptr chordalBayesNet = smoother.eliminate(ordering);
  boost::shared_ptr<VectorConfig> actualSolution = chordalBayesNet->optimize();

  VectorConfig expectedSolution;
  Vector delta = zero(2);
  expectedSolution.insert("x1",delta);
  expectedSolution.insert("x2",delta);
  expectedSolution.insert("x3",delta);
  expectedSolution.insert("x4",delta);
  expectedSolution.insert("x5",delta);
  expectedSolution.insert("x6",delta);
  expectedSolution.insert("x7",delta);
	CHECK(assert_equal(expectedSolution,*actualSolution,1e-4));

	// Create the Bayes tree
	Gaussian bayesTree(*chordalBayesNet);
	LONGS_EQUAL(7,bayesTree.size());

	// Check root clique
	//BayesNet<ConditionalGaussian> expected_root;
	//BayesNet<ConditionalGaussian> actual_root = bayesTree.root();
	//CHECK(assert_equal(expected_root,actual_root));

	// Marginal will always be axis-parallel Gaussian on delta=(0,0)
	Matrix R = eye(2);

	// Check marginal on x1
  Vector sigma1 = repeat(2, 0.786153);
	ConditionalGaussian expected1("x1", delta, R, sigma1);
	ConditionalGaussian::shared_ptr actual1 = bayesTree.marginal<LinearFactor>("x1");
	CHECK(assert_equal(expected1,*actual1,1e-4));

	// Check marginal on x2
  Vector sigma2 = repeat(2, 0.687131);
	ConditionalGaussian expected2("x2", delta, R, sigma2);
	ConditionalGaussian::shared_ptr actual2 = bayesTree.marginal<LinearFactor>("x2");
	CHECK(assert_equal(expected2,*actual2,1e-4));

	// Check marginal on x3
  Vector sigma3 = repeat(2, 0.671512);
	ConditionalGaussian expected3("x3", delta, R, sigma3);
	ConditionalGaussian::shared_ptr actual3 = bayesTree.marginal<LinearFactor>("x3");
	CHECK(assert_equal(expected3,*actual3,1e-4));
}

/* ************************************************************************* */
int main() {
	TestResult tr;
	return TestRegistry::runAllTests(tr);
}
/* ************************************************************************* */
