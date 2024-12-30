/* ----------------------------------------------------------------------------

 * GTSAM Copyright 2010, Georgia Tech Research Corporation,
 * Atlanta, Georgia 30332-0415
 * All Rights Reserved
 * Authors: Frank Dellaert, et al. (see THANKS for the full author list)

 * See LICENSE for the license information

 * -------------------------------------------------------------------------- */

/**
 *  @file DiscreteTableConditional.h
 *  @date Dec 22, 2024
 *  @author Varun Agrawal
 */

#pragma once

#include <gtsam/discrete/DiscreteConditional.h>
#include <gtsam/discrete/Signature.h>
#include <gtsam/discrete/TableFactor.h>
#include <gtsam/inference/Conditional-inst.h>

#include <memory>
#include <string>
#include <vector>

namespace gtsam {

/**
 * Discrete Conditional Density which uses a SparseTable as the internal
 * representation, similar to the TableFactor.
 *
 * @ingroup discrete
 */
class GTSAM_EXPORT DiscreteTableConditional : public DiscreteConditional {
  Eigen::SparseVector<double> sparse_table_;

 public:
  // typedefs needed to play nice with gtsam
  typedef DiscreteTableConditional This;     ///< Typedef to this class
  typedef std::shared_ptr<This> shared_ptr;  ///< shared_ptr to this class
  typedef DiscreteConditional
      BaseConditional;  ///< Typedef to our conditional base class

  using Values = DiscreteValues;  ///< backwards compatibility

  /// @name Standard Constructors
  /// @{

  /// Default constructor needed for serialization.
  DiscreteTableConditional() {}

  /// Construct from factor, taking the first `nFrontals` keys as frontals.
  DiscreteTableConditional(size_t nFrontals, const TableFactor& f);

  /**
   * Construct from DiscreteKeys and SparseVector, taking the first
   * `nFrontals` keys as frontals, in the order given.
   */
  DiscreteTableConditional(size_t nFrontals, const DiscreteKeys& keys,
                           const Eigen::SparseVector<double>& potentials);

  /** Construct from signature */
  explicit DiscreteTableConditional(const Signature& signature);

  /**
   * Construct from key, parents, and a Signature::Table specifying the
   * conditional probability table (CPT) in 00 01 10 11 order. For
   * three-valued, it would be 00 01 02 10 11 12 20 21 22, etc....
   *
   * Example: DiscreteTableConditional P(D, {B,E}, table);
   */
  DiscreteTableConditional(const DiscreteKey& key, const DiscreteKeys& parents,
                           const Signature::Table& table)
      : DiscreteTableConditional(Signature(key, parents, table)) {}

  /**
   * Construct from key, parents, and a vector<double> specifying the
   * conditional probability table (CPT) in 00 01 10 11 order. For
   * three-valued, it would be 00 01 02 10 11 12 20 21 22, etc....
   *
   * Example: DiscreteTableConditional P(D, {B,E}, table);
   */
  DiscreteTableConditional(const DiscreteKey& key, const DiscreteKeys& parents,
                           const std::vector<double>& table)
      : DiscreteTableConditional(
            1, TableFactor(DiscreteKeys{key} & parents, table)) {}

  /**
   * Construct from key, parents, and a string specifying the conditional
   * probability table (CPT) in 00 01 10 11 order. For three-valued, it would
   * be 00 01 02 10 11 12 20 21 22, etc....
   *
   * The string is parsed into a Signature::Table.
   *
   * Example: DiscreteTableConditional P(D, {B,E}, "9/1 2/8 3/7 1/9");
   */
  DiscreteTableConditional(const DiscreteKey& key, const DiscreteKeys& parents,
                           const std::string& spec)
      : DiscreteTableConditional(Signature(key, parents, spec)) {}

  /// No-parent specialization; can also use DiscreteDistribution.
  DiscreteTableConditional(const DiscreteKey& key, const std::string& spec)
      : DiscreteTableConditional(Signature(key, {}, spec)) {}

  /**
   * @brief construct P(X|Y) = f(X,Y)/f(Y) from f(X,Y) and f(Y)
   * Assumes but *does not check* that f(Y)=sum_X f(X,Y).
   */
  DiscreteTableConditional(const TableFactor& joint,
                           const TableFactor& marginal);

  /**
   * @brief construct P(X|Y) = f(X,Y)/f(Y) from f(X,Y) and f(Y)
   * Assumes but *does not check* that f(Y)=sum_X f(X,Y).
   * Makes sure the keys are ordered as given. Does not check orderedKeys.
   */
  DiscreteTableConditional(const TableFactor& joint,
                           const TableFactor& marginal,
                           const Ordering& orderedKeys);

  /**
   * @brief Combine two conditionals, yielding a new conditional with the union
   * of the frontal keys, ordered by gtsam::Key.
   *
   * The two conditionals must make a valid Bayes net fragment, i.e.,
   * the frontal variables cannot overlap, and must be acyclic:
   * Example of correct use:
   *   P(A,B) = P(A|B) * P(B)
   *   P(A,B|C) = P(A|B) * P(B|C)
   *   P(A,B,C) = P(A,B|C) * P(C)
   * Example of incorrect use:
   *   P(A|B) * P(A|C) = ?
   *   P(A|B) * P(B|A) = ?
   * We check for overlapping frontals, but do *not* check for cyclic.
   */
  DiscreteTableConditional operator*(
      const DiscreteTableConditional& other) const;

  /// @}
  /// @name Testable
  /// @{

  /// GTSAM-style print
  void print(
      const std::string& s = "Discrete Conditional: ",
      const KeyFormatter& formatter = DefaultKeyFormatter) const override;

  /// GTSAM-style equals
  bool equals(const DiscreteFactor& other, double tol = 1e-9) const override;

  /// @}
  /// @name Standard Interface
  /// @{

  /// Log-probability is just -error(x).
  double logProbability(const DiscreteValues& x) const { return -error(x); }

  /// print index signature only
  void printSignature(
      const std::string& s = "Discrete Conditional: ",
      const KeyFormatter& formatter = DefaultKeyFormatter) const {
    static_cast<const BaseConditional*>(this)->print(s, formatter);
  }

  /** Convert to a likelihood factor by providing value before bar. */
  TableFactor::shared_ptr likelihood(const DiscreteValues& frontalValues) const;

  /** Single variable version of likelihood. */
  TableFactor::shared_ptr likelihood(size_t frontal) const;

  /**
   * @brief Return assignment for single frontal variable that maximizes value.
   * @param parentsValues Known assignments for the parents.
   * @return maximizing assignment for the frontal variable.
   */
  size_t argmax(const DiscreteValues& parentsValues = DiscreteValues()) const;

  /// @}
  /// @name Advanced Interface
  /// @{

  /// Return all assignments for frontal variables.
  std::vector<DiscreteValues> frontalAssignments() const;

  /// Return all assignments for frontal *and* parent variables.
  std::vector<DiscreteValues> allAssignments() const;

  /// @}
  /// @name HybridValues methods.
  /// @{

  using BaseConditional::operator();  ///< HybridValues version

  /**
   * Calculate log-probability log(evaluate(x)) for HybridValues `x`.
   * This is actually just -error(x).
   */
  double logProbability(const HybridValues& x) const override {
    return -error(x);
  }

  /// @}

 private:
#if GTSAM_ENABLE_BOOST_SERIALIZATION
  /** Serialization function */
  friend class boost::serialization::access;
  template <class Archive>
  void serialize(Archive& ar, const unsigned int /*version*/) {
    ar& BOOST_SERIALIZATION_BASE_OBJECT_NVP(BaseConditional);
  }
#endif
};
// DiscreteTableConditional

// traits
template <>
struct traits<DiscreteTableConditional>
    : public Testable<DiscreteTableConditional> {};

}  // namespace gtsam
