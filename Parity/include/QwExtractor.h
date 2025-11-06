/*!
 * \file   QwExtractor.h
 * \brief  Data extraction handler for output processing
 * \author cameronc137
 * \date   2019-11-22
 */

#pragma once

// Parent Class
#include "VQwDataHandler.h"

/**
 * \class QwExtractor
 * \ingroup QwAnalysis_BL
 * \brief Data handler that extracts values into output trees/files
 *
 * Connects to a source subsystem array and publishes selected values
 * (event-level) to ROOT trees for downstream analysis.
 */
class QwExtractor:public VQwDataHandler, public MQwDataHandlerCloneable<QwExtractor>
{
 public:
    /// \brief Constructor with name
    QwExtractor(const TString& name);

    /// \brief Copy constructor
    QwExtractor(const QwExtractor &source);
    /// Virtual destructor
    ~QwExtractor() override;

    Int_t LoadChannelMap(const std::string& mapfile) override;
    /**
     * \brief Connect to channels (event-only extraction).
     * @param event Subsystem array providing per-MPS yields to extract.
     * @return 0 on success; non-zero on failure.
     */
    Int_t ConnectChannels(QwSubsystemArrayParity& event) override;
    void ConstructTreeBranches(
        QwRootFile *treerootfile,
        const std::string& treeprefix = "",
        const std::string& branchprefix = "") override;
    void ProcessData() override;
    void SetPointer(QwSubsystemArrayParity *ptr){fSourcePointer = ptr;};
    void FillTreeBranches(QwRootFile *treerootfile) override;

  protected:
    /// Default constructor (Protected for child class access)

    /// Error flag mask
    UInt_t fErrorFlagMask;
    const UInt_t* fErrorFlagPointer;
    Int_t fLocalFlag = 0;

    const QwSubsystemArrayParity* fSourcePointer;
    QwSubsystemArrayParity* fSourceCopy;
    //TTree* fTree;

  private:
    // Default constructor
    QwExtractor();

}; // class QwExtractor

// Register this handler with the factory
REGISTER_DATA_HANDLER_FACTORY(QwExtractor);

