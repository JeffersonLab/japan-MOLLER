/*!
 * \file   QwDBInterface.cc
 * \brief  Database interface implementation for QwIntegrationPMT and subsystems
 * \author wdconinc, jhlee
 * \date   2010-12-14
 */

#include "QwDBInterface.h"

// Qweak headers
#ifdef __USE_DATABASE__
#include "QwParitySchema.h"
#include "QwParitySchemaRow.h"
#include "QwParityDB.h"
#endif // __USE_DATABASE__

std::map<TString, TString> QwDBInterface::fPrefix;

TString QwDBInterface::DetermineMeasurementTypeID(TString type, TString suffix,
						  Bool_t forcediffs)
{
  if (fPrefix.empty()){
    fPrefix["yield"]      = "y";
    fPrefix["difference"] = "d";
    fPrefix["asymmetry"]  = "a";
    fPrefix["asymmetry1"] = "a12";
    fPrefix["asymmetry2"] = "aeo";
  }
  TString measurement_type("");
  if (fPrefix.count(type)==1){
    measurement_type = fPrefix[type];
    if (measurement_type[0] == 'a' &&
	(forcediffs
	 || (suffix == "p" || suffix == "a"
	     || suffix == "m"))	){
      //  Change the type to difference for position,
      //  angle, or slope asymmetry variables.
      measurement_type[0] = 'd';
    } else if (measurement_type[0] == 'y') {
      measurement_type += suffix;
    }
  }
  QwDebug << "\n"
	  << type << ", " << suffix
	  << " \'" <<  measurement_type.Data() << "\'" << QwLog::endl;
  return measurement_type;
}

#ifdef __USE_DATABASE__
void QwDBInterface::SetDetectorID(QwParityDB *db, const TString& detector_type)
{
  fDetectorType = detector_type;
  fDeviceId = db->GetDetectorID(fDeviceName.Data());
}

QwDBInterface::EQwDBIDataTableType QwDBInterface::SetDetectorID(QwParityDB *db)
{
  // Legacy compatibility wrapper that auto-detects type
  // Try to find in unified detector table
  fDeviceId = db->GetDetectorID(fDeviceName.Data(), kFALSE);
  if (fDeviceId != 0) {
    // Auto-detect detector type based on naming convention
    // Note: In production, this should query the detector table to get actual detector_type
    if (fDeviceName.Contains("bcm", TString::kIgnoreCase) || 
        fDeviceName.Contains("bpm", TString::kIgnoreCase) ||
        fDeviceName.Contains("clock", TString::kIgnoreCase) ||
        fDeviceName.Contains("energy", TString::kIgnoreCase)) {
      fDetectorType = "beam";
    } else if (fDeviceName.Contains("lumi", TString::kIgnoreCase)) {
      fDetectorType = "lumi";
    } else if (fDeviceName.Contains("bkg", TString::kIgnoreCase) ||
               fDeviceName.Contains("background", TString::kIgnoreCase)) {
      fDetectorType = "bkg";
    } else {
      fDetectorType = "md";  // default to main detector
    }
    return kQwDBI_DetectorTable;
  }
  return kQwDBI_OtherTable;
}
#endif // __USE_DATABASE__

template <class T>
T QwDBInterface::TypedDBClone()
{
  T row(0);
  return row;
}


#ifdef __USE_DATABASE__
/// Specifications of the templated function
/// template \verbatim<class T>\endverbatim inline T QwDBInterface::TypedDBClone();

// New unified detector_data_row specialization
template<> QwParitySchema::detector_data_row
QwDBInterface::TypedDBClone<QwParitySchema::detector_data_row>() {
  QwParitySchema::DetectorData DetectorData;
  QwParitySchema::detector_data_row row;
  
  row[DetectorData.analysisId]       = fAnalysisId;
  row[DetectorData.detectorId]       = fDeviceId;
  row[DetectorData.detectorTypeId]  = fDetectorType.Data();  // "md", "beam", "lumi", "bkg"
  row[DetectorData.measureTypeId]   = fMeasurementTypeId;
  row[DetectorData.subblock]          = fSubblock;
  row[DetectorData.n]                 = fN;
  row[DetectorData.value]             = fValue;
  row[DetectorData.error]             = fError;
  
  // Set default values for new fields not in old schema
  // TODO: slug_id should be obtained from analysis context (run.slug)
  row[DetectorData.slugId]           = 0;
  // error_code fields default to 0 (no error)
  row[DetectorData.errorCodeId]     = 0;
  row[DetectorData.errorCodeN]      = 0;
  
  return row;
}

// Note: Legacy types md_data_row, lumi_data_row, beam_row now all map to detector_data_row type
// (row<DetectorData>), so there's only one template specialization above.
#endif // __USE_DATABASE__




// QwErrDBInterface

#ifdef __USE_DATABASE__
void QwErrDBInterface::SetDetectorID(QwParityDB *db, const TString& detector_type)
{
  fDetectorType = detector_type;
  fDeviceId = db->GetDetectorID(fDeviceName.Data());
}
#endif // __USE_DATABASE__
template <class T>
T QwErrDBInterface::TypedDBClone()
{
  T row(0);
  return row;
}
#ifdef __USE_DATABASE__
// Simplified error interface TypedDBClone implementations

// New unified detector_data_row specialization for errors
template<> QwParitySchema::detector_data_row
QwErrDBInterface::TypedDBClone<QwParitySchema::detector_data_row>() {
  QwParitySchema::DetectorData DetectorData;
  QwParitySchema::detector_data_row row;
  
  row[DetectorData.analysisId]       = fAnalysisId;
  row[DetectorData.detectorId]       = fDeviceId;
  row[DetectorData.detectorTypeId]  = fDetectorType.Data();
  row[DetectorData.measureTypeId]   = "";  // Not applicable for errors
  row[DetectorData.subblock]          = 0;   // Not applicable for errors
  row[DetectorData.n]                 = fN;
  row[DetectorData.value]             = 0.0; // Not applicable for errors
  row[DetectorData.error]             = 0.0; // Not applicable for errors
  
  // Error tracking fields
  row[DetectorData.slugId]           = 0;  // TODO: Get from analysis context
  row[DetectorData.errorCodeId]     = fErrorCodeId;
  row[DetectorData.errorCodeN]      = fN;  // Error count
  
  return row;
}

// Note: Legacy error types md_errors_row, lumi_errors_row, beam_errors_row now all map to detector_data_row type
// (row<DetectorData>), so there's only one template specialization above.

template<> QwParitySchema::general_errors_row
QwErrDBInterface::TypedDBClone<QwParitySchema::general_errors_row>() {
  QwParitySchema::GeneralErrors GeneralErrors;
  QwParitySchema::general_errors_row row;
  row[GeneralErrors.analysisId]         = fAnalysisId;
  row[GeneralErrors.errorCodeId]       = fErrorCodeId;
  row[GeneralErrors.n]                   = fN;
  return row;
}
#endif // __USE_DATABASE__
