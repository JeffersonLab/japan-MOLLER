/*!
 * \file   QwDataHandlerArray.h
 * \brief  Array container for managing multiple data handlers
 * \author P. M. King
 * \date   2009-02-04
 */

#pragma once

#include <vector>
#include <map>
#include "Rtypes.h"
#include "TString.h"
#include "TDirectory.h"
#include <TTree.h>

// ROOT headers
#ifdef HAS_RNTUPLE_SUPPORT
#include "ROOT/RNTupleModel.hxx"
#endif // HAS_RNTUPLE_SUPPORT

// Qweak headers
#include "QwDataHandlerArray.h"
#include "VQwDataHandler.h"
#include "QwOptions.h"
#include "QwHelicityPattern.h"
#include "MQwPublishable.h"

// Forward declarations
class QwParityDB;
class QwPromptSummary;

/**
 * \class QwDataHandlerArray
 * \ingroup QwAnalysis
 *
 * \brief Virtual base class for the parity handlers
 *
 *   Virtual base class for the classes containing the
 *   event-based information from each parity handler.
 *   This will define the interfaces used in communicating
 *   with the CODA routines.
 *
 */
class QwDataHandlerArray:
    public std::vector<std::shared_ptr<VQwDataHandler> >,
    public MQwPublishable<QwDataHandlerArray,VQwDataHandler>
{
 private:
  typedef std::vector<std::shared_ptr<VQwDataHandler> >  HandlerPtrs;
 public:
  using HandlerPtrs::const_iterator;
  using HandlerPtrs::iterator;
  using HandlerPtrs::begin;
  using HandlerPtrs::end;
  using HandlerPtrs::size;
  using HandlerPtrs::empty;

  private:
    /// Private default constructor
    QwDataHandlerArray(); // not implement, will thrown linker error on use

  public:
    /// Constructor from helicity pattern with options
    QwDataHandlerArray(QwOptions& options, QwHelicityPattern& helicitypattern, const TString &run);
    /// Constructor from subsystem array with options
    QwDataHandlerArray(QwOptions& options, QwSubsystemArrayParity& detectors, const TString &run);
    /// Copy constructor by reference
    QwDataHandlerArray(const QwDataHandlerArray& source);
    /// Default destructor
    ~QwDataHandlerArray() override;

    /// \brief Define configuration options for global array
    static void DefineOptions(QwOptions &options);
    /// \brief Process configuration options for the datahandler array itself
    void ProcessOptions(QwOptions &options);

  /**
   * \brief Load data handlers from a parameter file.
   *
   * Parses the map file and constructs/initializes handlers, connecting
   * them to the provided source container.
   *
   * @tparam T           Source type (QwHelicityPattern or QwSubsystemArrayParity).
   * @param mapfile      Parameter file describing handlers and settings.
   * @param detectors    Source object to connect handlers to.
   * @param run          Run label used for per-run configuration.
   */
  template<class T>
  void LoadDataHandlersFromParameterFile(QwParameterFile& mapfile, T& detectors, const TString &run);

    /// \brief Add the datahandler to this array
    void push_back(VQwDataHandler* handler);
    void push_back(std::shared_ptr<VQwDataHandler> handler);

    /// \brief Get the handler with the specified name
    VQwDataHandler* GetDataHandlerByName(const TString& name);

    std::vector<VQwDataHandler*> GetDataHandlerByType(const std::string& type);

    void ConstructTreeBranches(
        QwRootFile *treerootfile,
        const std::string& treeprefix = "",
        const std::string& branchprefix = "");

    /// \brief Construct a branch and vector for this handler with a prefix
    void ConstructBranchAndVector(TTree *tree, TString& prefix, QwRootTreeBranchVector &values);

    void FillTreeBranches(QwRootFile *treerootfile);
    /// \brief Fill the vector for this handler
    void FillTreeVector(QwRootTreeBranchVector &values) const;

    /// RNTuple methods
    void ConstructNTupleFields(
        QwRootFile *treerootfile,
        const std::string& treeprefix = "",
        const std::string& branchprefix = "");

    void FillNTupleFields(QwRootFile *treerootfile);

    /// Construct the histograms for this subsystem
    void  ConstructHistograms() {
      ConstructHistograms((TDirectory*) NULL);
    };
    /// Construct the histograms for this subsystem in a folder
    void  ConstructHistograms(TDirectory *folder) {
      TString prefix = "";
      ConstructHistograms(folder, prefix);
    };
    /// \brief Construct the histograms in a folder with a prefix
    void  ConstructHistograms(TDirectory *folder, TString &prefix);
    /// \brief Fill the histograms
    void  FillHistograms();

    /// \brief Fill the database
    void FillDB(QwParityDB *db, TString type);
    //    void FillErrDB(QwParityDB *db, TString type);

    void  ClearEventData();
    void  ProcessEvent();

    void UpdateBurstCounter(Short_t burstcounter)
    {
      if (!empty()) {
	for(iterator handler = begin(); handler != end(); ++handler){
	  (*handler)->UpdateBurstCounter(burstcounter);
	}
      }
    }

    /// \brief Assignment operator
    QwDataHandlerArray& operator=  (const QwDataHandlerArray &value);
    /*
    /// \brief Addition-assignment operator
    QwDataHandlerArray& operator+= (const QwDataHandlerArray &value);
    /// \brief Subtraction-assignment operator
    QwDataHandlerArray& operator-= (const QwDataHandlerArray &value);
    /// \brief Sum of two handler arrays
    void Sum(const QwDataHandlerArray &value1, const QwDataHandlerArray &value2);
    /// \brief Difference of two handler arrays
    void Difference(const QwDataHandlerArray &value1, const QwDataHandlerArray &value2);
    /// \brief Scale this handler array
    void Scale(Double_t factor);
    */

    /// \brief Update the running sums for devices accumulated for the global error non-zero events/patterns
    void AccumulateRunningSum();

    /// \brief Update the running sums for devices accumulated for the global error non-zero events/patterns
    void AccumulateRunningSum(const QwDataHandlerArray& value, Int_t count=0, Int_t ErrorMask=0xFFFFFFF);
    /// \brief Update the running sums for devices check only the error flags at the channel level. Only used for stability checks
    void AccumulateAllRunningSum(const QwDataHandlerArray& value, Int_t count=0, Int_t ErrorMask=0xFFFFFFF);

    /// \brief Calculate the average for all good events
    void CalculateRunningAverage();

    /// \brief Report the number of events failed due to HW and event cut failures
    void PrintErrorCounters() const;

    /// \brief Print value of all channels
    void PrintValue() const;


    void WritePromptSummary(QwPromptSummary *ps, TString type);


    void ProcessDataHandlerEntry();

    void FinishDataHandler();

  protected:

    void SetPointer(QwHelicityPattern& helicitypattern) {
      fHelicityPattern = &helicitypattern;
    }
    void SetPointer(QwSubsystemArrayParity& detectors) {
      fSubsystemArray = &detectors;
    }

    /// Pointer for the original data source
    QwHelicityPattern *fHelicityPattern;
    QwSubsystemArrayParity *fSubsystemArray;

    /// Filename of the global detector map
    std::string fDataHandlersMapFile;

    Bool_t ScopeMismatch(TString name){
      name.ToLower();
      EDataHandlerArrayScope tmpscope = kUnknownScope;
      if (name=="event") tmpscope = kEventScope;
      if (name=="pattern") tmpscope = kPatternScope;
      return (fArrayScope != tmpscope);
    }
    enum EDataHandlerArrayScope {kUnknownScope=-1, kEventScope, kPatternScope};
    EDataHandlerArrayScope fArrayScope;

    std::vector<std::string> fDataHandlersDisabledByName; ///< List of disabled types
    std::vector<std::string> fDataHandlersDisabledByType; ///< List of disabled names

    Bool_t fPrintRunningSum;
    Bool_t fEnableRNTuples;

    /// Test whether this handler array can contain a particular handler
    static Bool_t CanContain(VQwDataHandler* handler) {
      return (dynamic_cast<VQwDataHandler*>(handler) != 0);
    };

}; // class QwDataHandlerArray
