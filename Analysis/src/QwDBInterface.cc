/*
 * QwDBInterface.cc
 *
 *  Created on: Dec 14, 2010
 *      Author: wdconinc
 *      Author: jhlee
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
void QwDBInterface::SetMonitorID(QwParityDB *db)
{
  fDeviceId = db->GetMonitorID(fDeviceName.Data());
}

void QwDBInterface::SetMainDetectorID(QwParityDB *db)
{
  fDeviceId = db->GetMainDetectorID(fDeviceName.Data());
}

void QwDBInterface::SetLumiDetectorID(QwParityDB *db)
{
  fDeviceId = db->GetLumiDetectorID(fDeviceName.Data());
}

QwDBInterface::EQwDBIDataTableType QwDBInterface::SetDetectorID(QwParityDB *db)
{
  fDeviceId = db->GetMonitorID(fDeviceName.Data(),kFALSE);
  if (fDeviceId!=0) return kQwDBI_BeamTable;
  fDeviceId = db->GetMainDetectorID(fDeviceName.Data(),kFALSE);
  if (fDeviceId!=0) return kQwDBI_MDTable;
  fDeviceId = db->GetLumiDetectorID(fDeviceName.Data(),kFALSE);
  if (fDeviceId!=0) return kQwDBI_LumiTable;
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
template<> QwParitySchema::md_data_row
QwDBInterface::TypedDBClone<QwParitySchema::md_data_row>() {
  QwParitySchema::md_data md_data;
  QwParitySchema::md_data_row row;
  row[md_data.analysis_id]         = fAnalysisId;
  row[md_data.main_detector_id]    = fDeviceId;
  row[md_data.measurement_type_id] = fMeasurementTypeId;
  row[md_data.subblock]            = fSubblock;
  row[md_data.n]                   = fN;
  row[md_data.value]               = fValue;
  row[md_data.error]               = fError;
  return row;
}

template<> QwParitySchema::lumi_data_row
QwDBInterface::TypedDBClone<QwParitySchema::lumi_data_row>() {
  QwParitySchema::lumi_data lumi_data;
  QwParitySchema::lumi_data_row row;
  row[lumi_data.analysis_id]         = fAnalysisId;
  row[lumi_data.lumi_detector_id]    = fDeviceId;
  row[lumi_data.measurement_type_id] = fMeasurementTypeId;
  row[lumi_data.subblock]            = fSubblock;
  row[lumi_data.n]                   = fN;
  row[lumi_data.value]               = fValue;
  row[lumi_data.error]               = fError;
  return row;
}

template<> QwParitySchema::beam_row
QwDBInterface::TypedDBClone<QwParitySchema::beam_row>() {
  QwParitySchema::beam beam;
  QwParitySchema::beam_row row;
  row[beam.analysis_id]         = fAnalysisId;
  row[beam.monitor_id]          = fDeviceId;
  row[beam.measurement_type_id] = fMeasurementTypeId;
  row[beam.subblock]            = fSubblock;
  row[beam.n]                   = fN;
  row[beam.value]               = fValue;
  row[beam.error]               = fError;
  return row;
}
#endif // __USE_DATABASE__




// QwErrDBInterface 

#ifdef __USE_DATABASE__
void QwErrDBInterface::SetMonitorID(QwParityDB *db)
{
  fDeviceId = db->GetMonitorID(fDeviceName.Data());
}

void QwErrDBInterface::SetMainDetectorID(QwParityDB *db)
{
  fDeviceId = db->GetMainDetectorID(fDeviceName.Data());
}

void QwErrDBInterface::SetLumiDetectorID(QwParityDB *db)
{
  fDeviceId = db->GetLumiDetectorID(fDeviceName.Data());
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
template<> QwParitySchema::md_errors_row
QwErrDBInterface::TypedDBClone<QwParitySchema::md_errors_row>() {
  QwParitySchema::md_errors_row row;
  QwParitySchema::md_errors table;
  row[table.analysis_id]         = fAnalysisId;
  row[table.main_detector_id]    = fDeviceId;
  row[table.error_code_id]       = fErrorCodeId;
  row[table.n]                   = fN;
  return row;
}

template<> QwParitySchema::lumi_errors_row
QwErrDBInterface::TypedDBClone<QwParitySchema::lumi_errors_row>() {
  QwParitySchema::lumi_errors_row row;
  QwParitySchema::lumi_errors table;
  row[table.analysis_id]         = fAnalysisId;
  row[table.lumi_detector_id]    = fDeviceId;
  row[table.error_code_id]       = fErrorCodeId;
  row[table.n]                   = fN;
  return row;
}

template<> QwParitySchema::beam_errors_row
QwErrDBInterface::TypedDBClone<QwParitySchema::beam_errors_row>() {
  QwParitySchema::beam_errors_row row;
  QwParitySchema::beam_errors table;
  row[table.analysis_id]         = fAnalysisId;
  row[table.monitor_id]          = fDeviceId;
  row[table.error_code_id]       = fErrorCodeId;
  row[table.n]                   = fN;
  return row;
}

template<> QwParitySchema::general_errors_row
QwErrDBInterface::TypedDBClone<QwParitySchema::general_errors_row>() {
  QwParitySchema::general_errors_row row;
  QwParitySchema::general_errors table;
  row[table.analysis_id]         = fAnalysisId;
  row[table.error_code_id]       = fErrorCodeId;
  row[table.n]                   = fN;
  return row;
}
#endif // __USE_DATABASE__


