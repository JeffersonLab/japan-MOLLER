/*!
 * \file   VQwSubsystem.h
 * \brief  Definition of the pure virtual base class of all subsystems
 *
 * \author P. M. King
 * \date   2007-05-08 15:40
 */

#pragma once

// System headers
#include <iostream>
#include <vector>

// ROOT headers
#include "Rtypes.h"
#include "TString.h"
#include "TDirectory.h"
#include "TTree.h"

// RNTuple headers
#ifdef HAS_RNTUPLE_SUPPORT
#include "ROOT/RNTupleModel.hxx"
#endif // HAS_RNTUPLE_SUPPORT

// Qweak headers
#include "MQwHistograms.h"
#include "MQwPublishable.h"
// Note: the factory header is included here because every subsystem
// has to register itself with a subsystem factory.
#include "QwFactory.h"

// Forward declarations
class VQwHardwareChannel;
class QwSubsystemArray;
class QwParameterFile;


/**
 *  \class   VQwSubsystem
 *  \ingroup QwAnalysis
 *  \brief   Base class for subsystems implementing container-delegation pattern
 *
 * VQwSubsystem serves as the foundation for all analysis subsystems and
 * implements the container-delegation architectural pattern. Unlike individual
 * data elements that use the dual-operator pattern, subsystems delegate
 * arithmetic operations to their contained elements and avoid virtual operators.
 *
 * \par Container-Delegation Pattern:
 * VQwSubsystem uses a fundamentally different approach from VQwDataElement:
 *
 * **Key Design Principles:**
 * - **No virtual operators** in subsystem base classes
 * - **Single operator versions**: Only type-specific operators needed
 * - **Delegation to elements**: Operators iterate over contained objects
 * - **Type safety via typeid**: Runtime type checking without inheritance conflicts
 *
 * \par Implementation Pattern:
 * \code
 * VQwSubsystem& operator+=(const VQwSubsystem& value) {
 *   // Iterate over contained elements
 *   for(size_t i=0; i<fElements.size(); i++) {
 *     VQwDataElement* elem1 = this->GetElement(i);
 *     VQwDataElement* elem2 = value.GetElement(i);
 *     if (typeid(*elem1) == typeid(*elem2)) {
 *       *elem1 += *elem2;  // Delegates to element operators
 *     } else {
 *       // Handle type mismatch
 *     }
 *   }
 *   return *this;
 * }
 * \endcode
 *
 * \par Subsystem Architecture:
 * - **CODA Integration**: ProcessEvBuffer(), ProcessConfigurationBuffer()
 * - **Event Processing**: ProcessEvent(), ClearEventData()
 * - **Data Management**: LoadChannelMap(), LoadInputParameters()
 * - **Output Generation**: ConstructHistograms(), ConstructBranch()
 * - **Factory Registration**: MQwSubsystemCloneable integration
 *
 * \par Specialized Abstract Bases:
 * Some hierarchies introduce specialized bases between VQwSubsystem and
 * concrete implementations (e.g., VQwBPM, VQwBCM, VQwClock) to enable
 * polymorphic dispatch for specific detector types while maintaining
 * the container-delegation pattern.
 *
 * \par Representative Example:
 * QwBeamLine demonstrates the complete subsystem implementation:
 * - Container management for BPMs, BCMs, and other beam devices
 * - Type-safe delegation to heterogeneous element collections
 * - CODA buffer processing with ROC/Bank mapping
 * - Event-level processing and running statistics
 * - Integration with QwSubsystemArrayParity via factory pattern
 *
 * \par Composition over Inheritance:
 * The container-delegation pattern provides:
 * - **Type safety**: Runtime checks without virtual operator conflicts
 * - **Flexibility**: Support for heterogeneous element collections
 * - **Performance**: Direct delegation without virtual dispatch overhead
 * - **Maintainability**: Clear separation between container and element concerns
 *
 * \dot
 * digraph example {
 *   node [shape=box, fontname=Helvetica, fontsize=10];
 *   VQwSubsystem [ label="VQwSubsystem\n(container-delegation)" URL="\ref VQwSubsystem"];
 *   VQwSubsystemParity [ label="VQwSubsystemParity" URL="\ref VQwSubsystemParity"];
 *   QwBeamLine [ label="QwBeamLine\n(canonical example)" URL="\ref QwBeamLine"];
 *   VQwSubsystem -> VQwSubsystemParity;
 *   VQwSubsystemParity -> QwBeamLine;
 * }
 * \enddot
 */
class VQwSubsystem: virtual public VQwSubsystemCloneable, public MQwHistograms, public MQwPublishable_child<QwSubsystemArray,VQwSubsystem> {

 public:

  /// Constructor with name
  VQwSubsystem(const TString& name)
  : MQwHistograms(),
    MQwPublishable_child<QwSubsystemArray, VQwSubsystem>(),
    fSystemName(name), fEventTypeMask(0x0), fIsDataLoaded(kFALSE),
    fCurrentROC_ID(-1), fCurrentBank_ID(-1) {
    ClearAllBankRegistrations();
  }
  /// Copy constructor by object
  VQwSubsystem(const VQwSubsystem& orig)
  : MQwHistograms(orig),
    MQwPublishable_child<QwSubsystemArray, VQwSubsystem>(),
    fPublishList(orig.fPublishList),
    fROC_IDs(orig.fROC_IDs),
    fBank_IDs(orig.fBank_IDs),
    fMarkerWords(orig.fMarkerWords)
  {
    fSystemName = orig.fSystemName;
    fIsDataLoaded = orig.fIsDataLoaded;
    fCurrentROC_ID = orig.fCurrentROC_ID;
    fCurrentBank_ID = orig.fCurrentBank_ID;
  }

  /// Default destructor
  ~VQwSubsystem() override { }


  /// \brief Define options function (note: no virtual static functions in C++)
  static void DefineOptions() { /* No default options defined */ };
  /// Process the command line options
  virtual void ProcessOptions(QwOptions & /*options*/) { };


  TString GetName() const {return fSystemName;};
  Bool_t  HasDataLoaded() const  {return fIsDataLoaded;}

  /// \brief Get the sibling with specified name
  /**
   * Get a sibling subsystem by name from the parent array.
   *
   * @param name Name of the sibling subsystem.
   * @return Pointer to the sibling, or NULL if not found.
   */
  VQwSubsystem* GetSibling(const std::string& name) const;


 public:

  virtual std::vector<TString> GetParamFileNameList();
  virtual std::map<TString, TString> GetDetectorMaps();

  /// \brief Try to publish an internal variable matching the submitted name
  Bool_t PublishByRequest(TString /*device_name*/) override{
    return kFALSE; // when not implemented, this returns failure
  };

  /// \brief Publish all variables of the subsystem
  Bool_t PublishInternalValues() const override {
    return kTRUE; // when not implemented, this returns success
  };

 protected:

  std::vector<std::vector<TString> > fPublishList;

 public:

  /// \brief Parse parameter file to find the map files
  /**
   * Parse a parameter file and dispatch to the appropriate loaders
   * based on key-value pairs (map, param, eventcut, geom, cross, mask).
   *
   * @param file Parameter file to read and parse.
   * @return 0 on success; non-zero on error.
   */
  virtual Int_t LoadDetectorMaps(QwParameterFile& file);
  /// Mandatory map file definition
  virtual Int_t LoadChannelMap(TString mapfile) = 0;
  /// Mandatory parameter file definition
  virtual Int_t LoadInputParameters(TString mapfile) = 0;
  /// Optional geometry definition
  virtual Int_t LoadGeometryDefinition(TString /*mapfile*/) { return 0; };
  /// Optional crosstalk definition
  virtual Int_t LoadCrosstalkDefinition(TString /*mapfile*/) { return 0; };
  /// Optional event cut file
  virtual Int_t LoadEventCuts(TString /*mapfile*/) { return 0; };

  /// Set event type mask
  void SetEventTypeMask(const UInt_t mask) { fEventTypeMask = mask; };
  /// Get event type mask
  UInt_t GetEventTypeMask() const { return fEventTypeMask; };


  virtual void  ClearEventData() = 0;

  virtual Int_t ProcessConfigurationBuffer(const ROCID_t roc_id, const BankID_t bank_id, UInt_t* buffer, UInt_t num_words) = 0;

  virtual Int_t ProcessEvBuffer(const UInt_t event_type, const ROCID_t roc_id, const BankID_t bank_id, UInt_t* buffer, UInt_t num_words){
    /// TODO:  Subsystems should be changing their ProcessEvBuffer routines to take the event_type as the first
    ///  argument.  But in the meantime, default to just calling the non-event-type-aware ProcessEvBuffer routine.
    if (((0x1 << (event_type - 1)) & this->GetEventTypeMask()) == 0) return 0;
    else return this->ProcessEvBuffer(roc_id, bank_id, buffer, num_words);
  };
  /// TODO:  The non-event-type-aware ProcessEvBuffer routine should be replaced with the event-type-aware version.
  virtual Int_t ProcessEvBuffer(const ROCID_t roc_id, const BankID_t bank_id, UInt_t* buffer, UInt_t num_words) = 0;

  virtual void  ProcessEvent() = 0;
  /*! \brief Request processed data from other subsystems for internal
   *         use in the second event processing stage.  Not all derived
   *         classes will require data from other subsystems.
   */
  virtual void  ExchangeProcessedData() { };
  /*! \brief Process the event data again, including data from other
   *         subsystems.  Not all derived classes will require
   *         a second stage of event data processing.
   */
  virtual void  ProcessEvent_2() { };


  /// \brief Perform actions at the end of the event loop
  virtual void  AtEndOfEventLoop(){QwDebug << fSystemName << " at end of event loop" << QwLog::endl;};


  // Not all derived classes will have the following functions
  virtual void  RandomizeEventData(int /*helicity*/ = 0, double /*time*/ = 0.0) { };
  virtual void  EncodeEventData(std::vector<UInt_t> & /*buffer*/) { };


  /// \name Objects construction and maintenance
  // @{
  /// Construct the objects for this subsystem
  virtual void  ConstructObjects() {
    TString tmpstr("");
    ConstructObjects((TDirectory*) NULL, tmpstr);
  };
  /// Construct the objects for this subsystem in a folder
  virtual void  ConstructObjects(TDirectory *folder) {
    TString tmpstr("");
    ConstructObjects(folder, tmpstr);
  };
  /// Construct the objects for this subsystem with a prefix
  virtual void  ConstructObjects(TString &prefix) {
    ConstructObjects((TDirectory*) NULL, prefix);
  };
  /// \brief Construct the objects for this subsystem in a folder with a prefix
  virtual void  ConstructObjects(TDirectory * /*folder*/, TString & /*prefix*/) { };
  // @}


  /// \name Histogram construction and maintenance
  // @{
  /// Construct the histograms for this subsystem
  virtual void  ConstructHistograms() {
    TString tmpstr("");
    ConstructHistograms((TDirectory*) NULL, tmpstr);
  };
  /// Construct the histograms for this subsystem in a folder
  virtual void  ConstructHistograms(TDirectory *folder) {
    TString tmpstr("");
    ConstructHistograms(folder, tmpstr);
  };
  /// Construct the histograms for this subsystem with a prefix
  virtual void  ConstructHistograms(TString &prefix) {
    ConstructHistograms((TDirectory*) NULL, prefix);
  };
  /// \brief Construct the histograms for this subsystem in a folder with a prefix
  virtual void  ConstructHistograms(TDirectory *folder, TString &prefix) = 0;
  /// \brief Fill the histograms for this subsystem
  virtual void  FillHistograms() = 0;
  // @}


  /// \name Tree and branch construction and maintenance
  /// The methods should exist for all subsystems and are therefore defined
  /// as pure virtual.
  // @{
  /// \brief Construct the branch and tree vector
  /**
   * Construct the branch and fill the provided values vector.
   * @param tree   Output ROOT tree to which branches are added.
   * @param prefix Name prefix for all branch names.
   * @param values Vector that will be filled by FillTreeVector.
   */
  virtual void ConstructBranchAndVector(TTree *tree, TString& prefix, QwRootTreeBranchVector &values) = 0;
  /// \brief Construct the branch and tree vector
  virtual void ConstructBranchAndVector(TTree *tree, QwRootTreeBranchVector &values) {
    TString tmpstr("");
    ConstructBranchAndVector(tree,tmpstr,values);
  };
  /// \brief Construct the branch and tree vector
  /**
   * Construct the branches for this subsystem.
   * @param tree   Output ROOT tree.
   * @param prefix Name prefix for all branch names.
   */
  virtual void ConstructBranch(TTree *tree, TString& prefix) = 0;
  /// \brief Construct the branch and tree vector based on the trim file
  /**
   * Construct the branches for this subsystem using a trim file.
   * @param tree      Output ROOT tree.
   * @param prefix    Name prefix for all branch names.
   * @param trim_file Trim file describing which branches to construct.
   */
  virtual void ConstructBranch(TTree *tree, TString& prefix, QwParameterFile& trim_file) = 0;
  /// \brief Fill the tree vector
  /**
   * Fill the tree export vector with the current event values.
   * @param values Output vector to be filled.
   */
  virtual void FillTreeVector(QwRootTreeBranchVector &values) const = 0;

#ifdef HAS_RNTUPLE_SUPPORT
  /// \brief Construct the RNTuple fields and vector
  /**
   * Construct the RNTuple fields and fill the export vector.
   * @param model     Output RNTuple model.
   * @param prefix    Name prefix for field names.
   * @param values    Export values vector.
   * @param fieldPtrs Shared pointers to field backing storage.
   */
  virtual void ConstructNTupleAndVector(ROOT::RNTupleModel *model, TString& prefix, std::vector<Double_t>& values, std::vector<std::shared_ptr<Double_t>>& fieldPtrs) = 0;
  /// \brief Construct the RNTuple fields and vector
  virtual void ConstructNTupleAndVector(ROOT::RNTupleModel *model, std::vector<Double_t>& values, std::vector<std::shared_ptr<Double_t>>& fieldPtrs) {
    TString tmpstr("");
    ConstructNTupleAndVector(model, tmpstr, values, fieldPtrs);
  };
  /// \brief Fill the RNTuple vector
  /**
   * Fill the RNTuple vector with the current event values.
   * @param values Output vector to be filled.
   */
  virtual void FillNTupleVector(std::vector<Double_t>& values) const = 0;
#endif // HAS_RNTUPLE_SUPPORT
  // @}




  /// \name Expert tree construction and maintenance
  /// These functions are not purely virtual, since not every subsystem is
  /// expected to implement them.  They are intended for expert output to
  /// trees.
  // @{
  /// \brief Construct the tree for this subsystem
  virtual void  ConstructTree() {
    TString tmpstr("");
    ConstructTree((TDirectory*) NULL, tmpstr);
  };
  /// \brief Construct the tree for this subsystem in a folder
  virtual void  ConstructTree(TDirectory *folder) {
    TString tmpstr("");
    ConstructTree(folder, tmpstr);
  };
  /// \brief Construct the tree for this subsystem with a prefix
  virtual void  ConstructTree(TString &prefix) {
    ConstructTree((TDirectory*) NULL, prefix);
  };
  /// \brief Construct the tree for this subsystem in a folder with a prefix
  virtual void  ConstructTree(TDirectory * /*folder*/, TString & /*prefix*/) { return; };
  /// \brief Fill the tree for this subsystem
  virtual void  FillTree() { return; };
  /// \brief Delete the tree for this subsystem
  virtual void  DeleteTree() { return; };
  // @}

  /// \brief Print some information about the subsystem
  /**
   * Print some information about the subsystem (name, ROCs/banks, parent).
   */
  virtual void  PrintInfo() const;

  /// \brief Assignment
  /// Note: Must be called at the beginning of all subsystems routine
  /// call to operator=(VQwSubsystem *value) by VQwSubsystem::operator=(value)
  virtual VQwSubsystem& operator=(VQwSubsystem *value);


  virtual void PrintDetectorMaps(Bool_t status) const;

 protected:

  /*! \brief Clear all registration of ROC and Bank IDs for this subsystem
   */
  /**
   * Clear all registration of ROC and Bank IDs for this subsystem and
   * reset current ROC/bank IDs to null.
   */
  void  ClearAllBankRegistrations();

  /*! \brief Tell the object that it will decode data from this ROC and sub-bank
   */
  /**
   * Register that this subsystem will decode data from a specific ROC/bank.
   * @param roc_id  ROC identifier.
   * @param bank_id Subbank identifier within the ROC (default 0).
   * @return 0 on success; ERROR if already registered.
   */
  virtual Int_t RegisterROCNumber(const ROCID_t roc_id, const BankID_t bank_id = 0);

  /*! \brief Tell the object that it will decode data from this sub-bank in the ROC currently open for registration
   */
  /**
   * Register a subbank under the current ROC registration.
   * @param bank_id Subbank identifier to register.
   * @return 0 on success; ERROR if no current ROC.
   */
  Int_t RegisterSubbank(const BankID_t bank_id);

  /**
   * Register a marker word within the current ROC/bank context.
   * @param markerword Marker word value.
   * @return 0 on success; ERROR if no current ROC.
   */
  Int_t RegisterMarkerWord(const UInt_t markerword);

  /**
   * Parse and register ROC/bank/marker entries from a map string.
   * @param mapstr Parameter string positioned at a registration line.
   */
  void RegisterRocBankMarker(QwParameterFile &mapstr);

  /**
   * Get the current flat subbank index (based on current ROC/bank).
   * @return Subbank index, or -1 if not found.
   */
  Int_t GetSubbankIndex() const { return GetSubbankIndex(fCurrentROC_ID, fCurrentBank_ID); }
  /**
   * Compute the flat subbank index from ROC and bank IDs.
   * @param roc_id  ROC identifier.
   * @param bank_id Subbank identifier within the ROC.
   * @return Subbank index, or -1 if not found.
   */
  Int_t GetSubbankIndex(const ROCID_t roc_id, const BankID_t bank_id) const;
  void  SetDataLoaded(Bool_t flag){fIsDataLoaded = flag;};

 public:
  void GetMarkerWordList(const ROCID_t roc_id, const BankID_t bank_id, std::vector<UInt_t> &marker)const{
    Int_t rocindex  = FindIndex(fROC_IDs, roc_id);
    if (rocindex>=0){
      Int_t bankindex = FindIndex(fBank_IDs[rocindex],bank_id);
      if (bankindex>=0 && fMarkerWords.at(rocindex).at(bankindex).size()>0){
	std::vector<UInt_t> m = fMarkerWords.at(rocindex).at(bankindex);
	marker.insert(marker.end(), m.begin(), m.end());
      }
    }
  }

 protected:
  template < class T >
    Int_t FindIndex(const std::vector<T> &myvec, const T value) const
    {
      Int_t index = -1;
      for (size_t i=0 ; i < myvec.size(); i++ ){
	if (myvec[i]==value){
	  index=i;
	  break;
	}
      }
      return index;
    };

 protected:

  TString  fSystemName; ///< Name of this subsystem

  UInt_t   fEventTypeMask; ///< Mask of event types

  Bool_t   fIsDataLoaded; ///< Has this subsystem gotten data to be processed?

  std::vector<TString> fDetectorMapsNames; ///< Names of loaded detector map files
  std::map<TString, TString> fDetectorMaps; ///< Map of file name to full path or content
 protected:

  ROCID_t  fCurrentROC_ID; ///< ROC ID that is currently being processed
  BankID_t fCurrentBank_ID; ///< Bank ID (and Marker word) that is currently being processed;

  /// Vector of ROC IDs associated with this subsystem
  std::vector<ROCID_t> fROC_IDs;
  /// Vector of Bank IDs per ROC ID associated with this subsystem
  std::vector< std::vector<BankID_t> > fBank_IDs;
  /// Vector of marker words per ROC & subbank associated with this subsystem
  std::vector< std::vector< std::vector<UInt_t> > > fMarkerWords;

 public:
  std::vector<ROCID_t> GetROCIds() const { return fROC_IDs; }
 protected:

  // Comparison of type
  Bool_t Compare(VQwSubsystem* source) {
    return (typeid(*this) == typeid(*source));
  }

 private:

  // Private constructor (not implemented, will throw linker error on use)
  VQwSubsystem();

}; // class VQwSubsystem
