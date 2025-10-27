/*!
 * \file   QwCombiner.h
 * \brief  Data combiner handler for channel operations
 * \author wdconinc
 * \date   2010-10-22
 */

#pragma once

// Parent Class
#include "VQwDataHandler.h"

/**
 * \class QwCombiner
 * \ingroup QwAnalysis
 * \brief Data handler that forms linear combinations of channels
 *
 * QwCombiner connects to one or more input variables (yields/asymmetries/
 * differences) and computes configured linear combinations using provided
 * sensitivities. It is typically used to derive composite diagnostics or to
 * prepare inputs for higher-level analyses.
 */
class QwCombiner:public VQwDataHandler, public MQwDataHandlerCloneable<QwCombiner>
{
 public:
    typedef std::vector< VQwHardwareChannel* >::iterator Iterator_HdwChan;
    typedef std::vector< VQwHardwareChannel* >::const_iterator ConstIterator_HdwChan;

 public:
    /// \brief Constructor with name
    QwCombiner(const TString& name);

    /// \brief Copy constructor
    QwCombiner(const QwCombiner &source);
    /// Virtual destructor
    ~QwCombiner() override;

    /// \brief Load the channels and sensitivities
    Int_t LoadChannelMap(const std::string& mapfile) override;

    /// \brief Connect to Channels (event only)
    /// \param event Subsystem array with per-MPS yields
    /// \return 0 on success, non-zero on failure
    Int_t ConnectChannels(QwSubsystemArrayParity& event) override;
    /// \brief Connect to Channels (asymmetry/difference only)
    /// \param asym Subsystem array with asymmetries
    /// \param diff Subsystem array with differences
    /// \return 0 on success, non-zero on failure
    Int_t ConnectChannels(QwSubsystemArrayParity& asym,
			  QwSubsystemArrayParity& diff) override;

    void ProcessData() override;

  protected:

    /// Default constructor (Protected for child class access)
    QwCombiner() { };

    /// Error flag mask
    UInt_t fErrorFlagMask;
    const UInt_t* fErrorFlagPointer;

    /// List of channels to use in the combiner
    std::vector< std::vector< EQwHandleType > > fIndependentType;
    std::vector< std::vector< std::string > > fIndependentName;
    std::vector< std::vector< const VQwHardwareChannel* > > fIndependentVar;
    std::vector< std::vector< Double_t > > fSensitivity;


}; // class QwCombiner

// Register this handler with the factory
REGISTER_DATA_HANDLER_FACTORY(QwCombiner);

inline std::ostream& operator<< (std::ostream& stream, const QwCombiner::EQwHandleType& i) {
  switch (i){
  case QwCombiner::kHandleTypeMps:  stream << "mps"; break;
  case QwCombiner::kHandleTypeAsym: stream << "asym"; break;
  case QwCombiner::kHandleTypeDiff: stream << "diff"; break;
  default:           stream << "Unknown";
  }
  return stream;
}
