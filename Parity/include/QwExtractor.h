/*
 * QwExtractor.h
 *
 *  Created on: Nov 22, 2019
 *      Author: cameronc137
 *
 */

#ifndef QWEXTRACTOR_H_
#define QWEXTRACTOR_H_

// Parent Class
#include "VQwDataHandler.h"

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

#endif // QWEXTRACTOR_H_

