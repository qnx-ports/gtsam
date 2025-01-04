/* ----------------------------------------------------------------------------

 * GTSAM Copyright 2010, Georgia Tech Research Corporation,
 * Atlanta, Georgia 30332-0415
 * All Rights Reserved
 * Authors: Frank Dellaert, et al. (see THANKS for the full author list)

 * See LICENSE for the license information

 * -------------------------------------------------------------------------- */

/**
 *  @file TableDistribution.cpp
 *  @date Dec 22, 2024
 *  @author Varun Agrawal
 */

#include <gtsam/base/Testable.h>
#include <gtsam/base/debug.h>
#include <gtsam/discrete/Ring.h>
#include <gtsam/discrete/Signature.h>
#include <gtsam/discrete/TableDistribution.h>
#include <gtsam/hybrid/HybridValues.h>

#include <algorithm>
#include <cassert>
#include <random>
#include <set>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

using namespace std;
using std::pair;
using std::stringstream;
using std::vector;
namespace gtsam {

/// Normalize sparse_table
static Eigen::SparseVector<double> normalizeSparseTable(
    const Eigen::SparseVector<double>& sparse_table) {
  return sparse_table / sparse_table.sum();
}

/* ************************************************************************** */
TableDistribution::TableDistribution(const TableFactor& f)
    : BaseConditional(f.keys().size(),
                      DecisionTreeFactor(f.discreteKeys(), ADT())),
      table_(f / (*f.sum(f.keys().size()))) {}

/* ************************************************************************** */
TableDistribution::TableDistribution(
    const DiscreteKeys& keys, const Eigen::SparseVector<double>& potentials)
    : BaseConditional(keys.size(), keys, DecisionTreeFactor(keys, ADT())),
      table_(TableFactor(keys, normalizeSparseTable(potentials))) {}

/* ************************************************************************** */
TableDistribution::TableDistribution(const DiscreteKeys& keys,
                                     const std::vector<double>& potentials)
    : BaseConditional(keys.size(), keys, DecisionTreeFactor(keys, ADT())),
      table_(TableFactor(
          keys, normalizeSparseTable(TableFactor::Convert(keys, potentials)))) {
}

/* ************************************************************************** */
TableDistribution::TableDistribution(const DiscreteKeys& keys,
                                     const std::string& potentials)
    : BaseConditional(keys.size(), keys, DecisionTreeFactor(keys, ADT())),
      table_(TableFactor(
          keys, normalizeSparseTable(TableFactor::Convert(keys, potentials)))) {
}

/* **************************************************************************
 */
TableDistribution::TableDistribution(const TableFactor& joint,
                                     const TableFactor& marginal)
    : BaseConditional(joint.size() - marginal.size(),
                      joint.discreteKeys() & marginal.discreteKeys(), ADT()),
      table_(joint / marginal) {}

/* ************************************************************************** */
TableDistribution::TableDistribution(const TableFactor& joint,
                                     const TableFactor& marginal,
                                     const Ordering& orderedKeys)
    : TableDistribution(joint, marginal) {
  keys_.clear();
  keys_.insert(keys_.end(), orderedKeys.begin(), orderedKeys.end());
}

/* ************************************************************************** */
void TableDistribution::print(const string& s,
                              const KeyFormatter& formatter) const {
  cout << s << " P( ";
  for (const_iterator it = beginFrontals(); it != endFrontals(); ++it) {
    cout << formatter(*it) << " ";
  }
  cout << "):\n";
  table_.print("", formatter);
  cout << endl;
}

/* ************************************************************************** */
bool TableDistribution::equals(const DiscreteFactor& other, double tol) const {
  auto dtc = dynamic_cast<const TableDistribution*>(&other);
  if (!dtc) {
    return false;
  } else {
    const DiscreteConditional& f(
        static_cast<const DiscreteConditional&>(other));
    return table_.equals(dtc->table_, tol) &&
           DiscreteConditional::BaseConditional::equals(f, tol);
  }
}

/* ****************************************************************************/
DiscreteConditional::shared_ptr TableDistribution::max(
    const Ordering& keys) const {
  auto m = *table_.max(keys);

  return std::make_shared<TableDistribution>(m);
}

/* ****************************************************************************/
void TableDistribution::setData(const DiscreteConditional::shared_ptr& dc) {
  if (auto dtc = std::dynamic_pointer_cast<TableDistribution>(dc)) {
    this->table_ = dtc->table_;
  } else {
    this->table_ = TableFactor(dc->discreteKeys(), *dc);
  }
}

/* ****************************************************************************/
DiscreteConditional::shared_ptr TableDistribution::prune(
    size_t maxNrAssignments) const {
  TableFactor pruned = table_.prune(maxNrAssignments);

  return std::make_shared<TableDistribution>(this->discreteKeys(),
                                             pruned.sparseTable());
}

}  // namespace gtsam
