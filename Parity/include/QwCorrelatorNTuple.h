/*!
 * \file   QwCorrelatorNTuple.h
 * \brief  RNTuple output of the correlator regression results
 */

#pragma once

#ifdef HAS_RNTUPLE_SUPPORT

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "Rtypes.h"
#include "TMatrixD.h"
#include "TVectorD.h"
#include <ROOT/RNTupleModel.hxx>
#include <ROOT/RNTupleWriter.hxx>

class QwRootFile;

/**
 * \class QwCorrelatorNTuple
 * \ingroup QwAnalysis
 * \brief Writes the regression results of a correlator to an RNTuple
 *
 * Holds the same values as the correlator TTrees (lrb_std, newcorr, grand):
 * one entry per regression, with
 * - integer fields for the counters (total_count, good_count, n, ErrorFlag),
 * - one std::vector<double> field per matrix, holding the elements in
 *   row-major order (like the TTree leaf name[nrows][ncols]), plus
 *   <name>_nrows and <name>_ncols integer fields with its dimensions,
 * - one std::vector<double> field per vector.
 *
 * Usage: Begin(), Add...(), Create() at construction time, Fill() whenever
 * the TTree would be filled.  The registered objects are read at Fill() time.
 */
class QwCorrelatorNTuple {

  public:

    QwCorrelatorNTuple() = default;
    /// Copies start empty: the registered pointers refer to the source object
    QwCorrelatorNTuple(const QwCorrelatorNTuple&) { }
    /// Assignment keeps this object's own registration
    QwCorrelatorNTuple& operator=(const QwCorrelatorNTuple&) { return *this; }

    /// Start a new definition; the prefix is prepended to every field name
    void Begin(const std::string& prefix);

    void AddScalar(const std::string& name, const Int_t* value);
    void AddScalar(const std::string& name, const Long64_t* value);
    void AddMatrix(const std::string& name, const TMatrixD* matrix);
    void AddVector(const std::string& name, const TVectorD* vector);

    /// Add n, ErrorFlag and the regression matrices and vectors of a
    /// correlator, in the order of the correlator TTree branches
    template < class R >
    void AddRegression(const R& r);

    /// Create the RNTuple in the file; returns whether it is active
    bool Create(QwRootFile* file, const std::string& name);

    /// Copy the current values into the RNTuple; returns whether it filled
    bool Fill();

    bool IsActive() const { return fWriter != nullptr; }

  private:

    std::string fPrefix;
    std::unique_ptr<ROOT::RNTupleModel> fModel;
    /// Owned by the QwRootFile, which finalizes it before closing the file
    ROOT::RNTupleWriter* fWriter = nullptr;

    struct Scalar32 { const Int_t* source; std::shared_ptr<std::int32_t> value; };
    struct Scalar64 { const Long64_t* source; std::shared_ptr<std::int64_t> value; };
    struct Matrix {
      const TMatrixD* source;
      std::shared_ptr<std::vector<double>> elements;
      std::shared_ptr<std::int32_t> nrows, ncols;
    };
    struct Vector { const TVectorD* source; std::shared_ptr<std::vector<double>> elements; };

    std::vector<Scalar32> fScalars32;
    std::vector<Scalar64> fScalars64;
    std::vector<Matrix> fMatrices;
    std::vector<Vector> fVectors;

    std::shared_ptr<std::vector<double>> AddArrayField(const std::string& name, const std::string& description);
};

template < class R >
void QwCorrelatorNTuple::AddRegression(const R& r)
{
  AddScalar("n", &r.fGoodEventNumber);
  AddScalar("ErrorFlag", &r.fErrorFlag);

  AddMatrix("A",    &r.Axy);
  AddMatrix("dA",   &r.dAxy);

  AddMatrix("VPP",  &r.mVPP);
  AddMatrix("VPY",  &r.mVPY);
  AddMatrix("VYP",  &r.mVYP);
  AddMatrix("VYY",  &r.mVYY);
  AddMatrix("VYYp", &r.mVYYp);

  AddMatrix("SPP",  &r.mSPP);
  AddMatrix("SPY",  &r.mSPY);
  AddMatrix("SYP",  &r.mSYP);
  AddMatrix("SYY",  &r.mSYY);
  AddMatrix("SYYp", &r.mSYYp);

  AddMatrix("RPP",  &r.mRPP);
  AddMatrix("RPY",  &r.mRPY);
  AddMatrix("RYP",  &r.mRYP);
  AddMatrix("RYY",  &r.mRYY);
  AddMatrix("RYYp", &r.mRYYp);

  AddVector("MP",   &r.mMP);   // Parameter mean
  AddVector("MY",   &r.mMY);   // Uncorrected mean
  AddVector("MYp",  &r.mMYp);  // Corrected mean

  AddVector("dMP",  &r.mSP);   // Parameter mean error
  AddVector("dMY",  &r.mSY);   // Uncorrected mean error
  AddVector("dMYp", &r.mSYp);  // Corrected mean error
}

#endif // HAS_RNTUPLE_SUPPORT
