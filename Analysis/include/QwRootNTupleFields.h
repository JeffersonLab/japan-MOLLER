/*!
 * \file   QwRootNTupleFields.h
 * \brief  Helper for declaring the RNTuple fields of a channel's values
 */

#pragma once

#ifdef HAS_RNTUPLE_SUPPORT

#include <memory>
#include <string>
#include <vector>

#include "Rtypes.h"
#include "TString.h"
#include <ROOT/RNTupleModel.hxx>

/**
 * \class QwRootNTupleFields
 * \ingroup QwAnalysis
 * \brief Declares the RNTuple fields that hold a contiguous block of values
 *
 * QwRootNTuple binds the field of fieldPtrs[i] to values[i] (see
 * QwRootNTuple::InitializeWriter), so every value pushed here gets a matching
 * fieldPtrs entry.  Two layouts are supported:
 *
 * - kFieldPerValue (default): one Double_t field per value, named
 *   basename + suffix (e.g. bpm1c10XP_hw_sum).
 * - kArrayPerChannel (--rntuple-channel-arrays): one std::array<double,N>
 *   field per channel, named basename, like the per-channel leaflist branch
 *   of the TTree.  The field description lists the element names separated
 *   by ':' (e.g. "hw_sum:block0:..."); element names are the suffixes
 *   without the leading '_', and "value" for an empty suffix.  Channels with
 *   a single value keep a Double_t field named basename + suffix.
 *
 * Both layouts store the same values for every entry; only the column
 * organization differs.
 */
class QwRootNTupleFields {

  public:

    enum class Layout { kFieldPerValue, kArrayPerChannel };

    static void SetLayout(Layout layout) { fgLayout = layout; }
    static Layout GetLayout() { return fgLayout; }

    /// Element name stored in the description of an array field for a suffix
    static std::string ElementName(const std::string& suffix);

    /// \brief Add the fields for values[values.size() .. + suffixes.size())
    ///
    /// Appends suffixes.size() zero-initialized values and the same number of
    /// fieldPtrs entries.
    static void AddChannel(
        std::unique_ptr<ROOT::RNTupleModel>& model,
        const TString& basename,
        const std::vector<std::string>& suffixes,
        std::vector<Double_t>& values,
        std::vector<std::shared_ptr<Double_t>>& fieldPtrs);

  private:

    static Layout fgLayout;
};

#endif // HAS_RNTUPLE_SUPPORT
