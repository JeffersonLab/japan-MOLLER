-- Database schema for JAPAN-MOLLER Parity Analysis
-- Aligned to MOLLER parity schema v2.0

SET FOREIGN_KEY_CHECKS=0;

DROP TABLE IF EXISTS event_execution_log;
DROP TABLE IF EXISTS summary_wien_detector_metrics;
DROP TABLE IF EXISTS summary_slug_detector_metrics;
DROP TABLE IF EXISTS beam_modulation;
DROP TABLE IF EXISTS ep_bf_test;
DROP TABLE IF EXISTS ee_bf_test;
DROP TABLE IF EXISTS slow_controls_data;
DROP TABLE IF EXISTS slow_controls_settings;
DROP TABLE IF EXISTS selected_sensitivities;
DROP TABLE IF EXISTS grand_correlator;
DROP TABLE IF EXISTS detector_data;
DROP TABLE IF EXISTS general_errors;
DROP TABLE IF EXISTS analysis;
DROP TABLE IF EXISTS ep_seeds;
DROP TABLE IF EXISTS ee_seeds;
DROP TABLE IF EXISTS data_taking_period;
DROP TABLE IF EXISTS detector;
DROP TABLE IF EXISTS modulation_type;
DROP TABLE IF EXISTS slow_control_detector;
DROP TABLE IF EXISTS detector_type;
DROP TABLE IF EXISTS measurement_type;
DROP TABLE IF EXISTS error_code;
DROP TABLE IF EXISTS good_for;
DROP TABLE IF EXISTS db_schema;

SET FOREIGN_KEY_CHECKS=1;

CREATE TABLE db_schema (
  db_schema_id INT UNSIGNED NOT NULL AUTO_INCREMENT PRIMARY KEY,
  major_release_number CHAR(2) NOT NULL,
  minor_release_number CHAR(2) NOT NULL,
  point_release_number CHAR(4) NOT NULL,
  time TIMESTAMP NOT NULL,
  script_name TEXT NULL,
  INDEX idx_schema_time (time)
);

CREATE TABLE good_for (
  good_for_id INT UNSIGNED NOT NULL AUTO_INCREMENT PRIMARY KEY,
  description TEXT NULL
);

CREATE TABLE error_code (
  error_code_id INT UNSIGNED NOT NULL AUTO_INCREMENT PRIMARY KEY,
  description TEXT NOT NULL
);

CREATE TABLE measurement_type (
  measurement_type_id CHAR(3) NOT NULL PRIMARY KEY,
  units VARCHAR(50) NOT NULL,
  title VARCHAR(255) NOT NULL
);

CREATE TABLE detector_type (
  detector_type_id INT UNSIGNED NOT NULL AUTO_INCREMENT PRIMARY KEY,
  detector VARCHAR(50) NOT NULL,
  title VARCHAR(255) NOT NULL,
  INDEX idx_detector_type_name (detector)
);

CREATE TABLE slow_control_detector (
  slow_control_detector_id INT UNSIGNED NOT NULL AUTO_INCREMENT PRIMARY KEY,
  name VARCHAR(255) NOT NULL UNIQUE,
  units VARCHAR(50) NOT NULL,
  comment TEXT NOT NULL,
  INDEX idx_sc_detector_name (name)
);

CREATE TABLE modulation_type (
  modulation_type_id INT UNSIGNED NOT NULL AUTO_INCREMENT PRIMARY KEY,
  type VARCHAR(255) NULL
);

CREATE TABLE detector (
  detector_id INT UNSIGNED NOT NULL AUTO_INCREMENT PRIMARY KEY,
  detector_type_id INT UNSIGNED NOT NULL,
  name VARCHAR(255) NOT NULL UNIQUE,
  units VARCHAR(255) NULL,
  description TEXT NULL,
  INDEX idx_detector_name (name),
  INDEX idx_detector_type_enum (detector_type_id),
  CONSTRAINT fk_detector_type
    FOREIGN KEY (detector_type_id) REFERENCES detector_type(detector_type_id)
);

CREATE TABLE data_taking_period (
  period_id INT UNSIGNED NOT NULL AUTO_INCREMENT PRIMARY KEY,
  period_type VARCHAR(50) NULL,
  parent_id INT UNSIGNED NULL,
  run_number INT UNSIGNED NOT NULL,
  segment_number SMALLINT NULL,
  good_for_id INT UNSIGNED NOT NULL,
  data_quality VARCHAR(50) NOT NULL,
  slug_number SMALLINT NOT NULL,
  wien_number SMALLINT NOT NULL,
  full_run BOOLEAN NOT NULL DEFAULT TRUE,
  start_time TIMESTAMP NULL,
  end_time TIMESTAMP NULL,
  tz_offset VARCHAR(32) NULL,
  first_minipulses INT UNSIGNED NULL,
  last_minipulses INT UNSIGNED NULL,
  num_minipulses BIGINT UNSIGNED NOT NULL,
  num_quartets BIGINT UNSIGNED NOT NULL,
  comment TEXT NULL,
  UNIQUE KEY uq_period_run_segment (run_number, segment_number),
  INDEX idx_period_type_hierarchy (period_type, parent_id),
  INDEX idx_period_slugs (slug_number, wien_number),
  INDEX idx_period_times (start_time, end_time),
  CONSTRAINT fk_period_parent
    FOREIGN KEY (parent_id) REFERENCES data_taking_period(period_id),
  CONSTRAINT fk_period_good_for
    FOREIGN KEY (good_for_id) REFERENCES good_for(good_for_id)
);

CREATE TABLE ee_seeds (
  ee_seed_id INT UNSIGNED NOT NULL AUTO_INCREMENT PRIMARY KEY,
  first_period_id INT UNSIGNED NOT NULL,
  last_period_id INT UNSIGNED NOT NULL,
  seed TEXT NULL,
  comment TEXT NULL,
  CONSTRAINT fk_ee_seeds_first_period
    FOREIGN KEY (first_period_id) REFERENCES data_taking_period(period_id),
  CONSTRAINT fk_ee_seeds_last_period
    FOREIGN KEY (last_period_id) REFERENCES data_taking_period(period_id)
);

CREATE TABLE ep_seeds (
  ep_seed_id INT UNSIGNED NOT NULL AUTO_INCREMENT PRIMARY KEY,
  first_period_id INT UNSIGNED NOT NULL,
  last_period_id INT UNSIGNED NOT NULL,
  seed TEXT NULL,
  comment TEXT NULL,
  CONSTRAINT fk_ep_seeds_first_period
    FOREIGN KEY (first_period_id) REFERENCES data_taking_period(period_id),
  CONSTRAINT fk_ep_seeds_last_period
    FOREIGN KEY (last_period_id) REFERENCES data_taking_period(period_id)
);

CREATE TABLE analysis (
  analysis_id INT UNSIGNED NOT NULL AUTO_INCREMENT PRIMARY KEY,
  period_id INT UNSIGNED NOT NULL,
  ee_seed_id INT UNSIGNED NOT NULL,
  ep_seed_id INT UNSIGNED NOT NULL,
  time TIMESTAMP NULL,
  ee_bf_checksum VARCHAR(64) NULL,
  ep_bf_checksum VARCHAR(64) NULL,
  beam_mode VARCHAR(50) NULL,
  regression_status BOOLEAN NOT NULL DEFAULT FALSE,
  rntuple_enabled BOOLEAN NOT NULL DEFAULT FALSE,
  root_version VARCHAR(50) NOT NULL,
  root_file_time VARCHAR(100) NOT NULL,
  root_file_host VARCHAR(100) NOT NULL,
  root_file_user VARCHAR(100) NOT NULL,
  analyzer_name VARCHAR(100) NOT NULL,
  analyzer_argv TEXT NOT NULL,
  analyzer_svn_rev VARCHAR(50) NOT NULL,
  analyzer_svn_lc_rev VARCHAR(50) NOT NULL,
  analyzer_svn_url TEXT NOT NULL,
  roc_flags TEXT NOT NULL,
  git_commit_sha CHAR(40) NULL,
  docker_image_digest VARCHAR(100) NULL,
  parameter_files JSON NULL,
  INDEX idx_analysis_period_perf (analysis_id, period_id),
  CONSTRAINT fk_analysis_period
    FOREIGN KEY (period_id) REFERENCES data_taking_period(period_id),
  CONSTRAINT fk_analysis_ee_seed
    FOREIGN KEY (ee_seed_id) REFERENCES ee_seeds(ee_seed_id),
  CONSTRAINT fk_analysis_ep_seed
    FOREIGN KEY (ep_seed_id) REFERENCES ep_seeds(ep_seed_id)
);

CREATE TABLE general_errors (
  general_errors_id BIGINT UNSIGNED NOT NULL AUTO_INCREMENT PRIMARY KEY,
  analysis_id INT UNSIGNED NOT NULL,
  error_code_id INT UNSIGNED NOT NULL,
  num_events BIGINT UNSIGNED NOT NULL,
  CONSTRAINT fk_gen_err_analysis
    FOREIGN KEY (analysis_id) REFERENCES analysis(analysis_id),
  CONSTRAINT fk_gen_err_code
    FOREIGN KEY (error_code_id) REFERENCES error_code(error_code_id)
);

CREATE TABLE detector_data (
  detector_data_id BIGINT UNSIGNED NOT NULL AUTO_INCREMENT PRIMARY KEY,
  detector_type_id INT UNSIGNED NOT NULL,
  analysis_id INT UNSIGNED NOT NULL,
  detector_id INT UNSIGNED NULL,
  measurement_type_id CHAR(3) NOT NULL,
  error_code_id INT UNSIGNED NOT NULL,
  error_code_n TINYINT UNSIGNED NOT NULL,
  sub_block SMALLINT NOT NULL,
  num_minipulses BIGINT UNSIGNED NOT NULL,
  value FLOAT NOT NULL,
  error FLOAT NOT NULL,
  INDEX idx_det_data_lookup (analysis_id, detector_id, measurement_type_id, sub_block),
  INDEX idx_det_data_type_fk (detector_type_id),
  INDEX idx_det_data_err_fk (error_code_id),
  INDEX idx_det_data_measure_fk (measurement_type_id),
  CONSTRAINT fk_det_data_type
    FOREIGN KEY (detector_type_id) REFERENCES detector_type(detector_type_id),
  CONSTRAINT fk_det_data_analysis
    FOREIGN KEY (analysis_id) REFERENCES analysis(analysis_id),
  CONSTRAINT fk_det_data_detector
    FOREIGN KEY (detector_id) REFERENCES detector(detector_id),
  CONSTRAINT fk_det_data_meas_type
    FOREIGN KEY (measurement_type_id) REFERENCES measurement_type(measurement_type_id),
  CONSTRAINT fk_det_data_err
    FOREIGN KEY (error_code_id) REFERENCES error_code(error_code_id)
);

CREATE TABLE grand_correlator (
  correlator_id INT UNSIGNED NOT NULL AUTO_INCREMENT PRIMARY KEY,
  analysis_id INT UNSIGNED NOT NULL,
  matrix JSON NOT NULL,
  CONSTRAINT fk_grand_corr_analysis
    FOREIGN KEY (analysis_id) REFERENCES analysis(analysis_id)
);

CREATE TABLE selected_sensitivities (
  sensitivity_id INT UNSIGNED NOT NULL AUTO_INCREMENT PRIMARY KEY,
  analysis_id INT UNSIGNED NOT NULL,
  independant_detector_id INT UNSIGNED NOT NULL,
  dependant_detector_id INT UNSIGNED NOT NULL,
  measurement_type_id CHAR(3) NOT NULL,
  per_pair_good_count INT UNSIGNED NOT NULL,
  slope FLOAT NOT NULL,
  error FLOAT NOT NULL,
  CONSTRAINT fk_sens_analysis
    FOREIGN KEY (analysis_id) REFERENCES analysis(analysis_id),
  CONSTRAINT fk_sens_indep_det
    FOREIGN KEY (independant_detector_id) REFERENCES detector(detector_id),
  CONSTRAINT fk_sens_dep_det
    FOREIGN KEY (dependant_detector_id) REFERENCES detector(detector_id),
  CONSTRAINT fk_sens_meas_type
    FOREIGN KEY (measurement_type_id) REFERENCES measurement_type(measurement_type_id)
);

CREATE TABLE slow_controls_settings (
  slow_controls_settings_id INT UNSIGNED NOT NULL AUTO_INCREMENT PRIMARY KEY,
  period_id INT UNSIGNED NOT NULL,
  slow_helicity_plate ENUM('out','in','unknown') NULL,
  passive_helicity_plate ENUM('out','in','unknown') NULL,
  wien_reversal ENUM('indeterminate','normal','reverse','transverse_vertical','transverse_horizontal') NULL,
  precession_reversal ENUM('CCW','CW','reverse','normal') NULL,
  helicity_window_length INT NULL,
  charge_feedback ENUM('off','on','unknown') NULL,
  position_feedback ENUM('off','on','unknown') NULL,
  target_position TEXT NULL,
  INDEX idx_sc_settings_lookup (period_id, slow_helicity_plate),
  CONSTRAINT fk_sc_settings_period
    FOREIGN KEY (period_id) REFERENCES data_taking_period(period_id)
);

CREATE TABLE slow_controls_data (
  slow_controls_data_id BIGINT UNSIGNED NOT NULL AUTO_INCREMENT PRIMARY KEY,
  period_id INT UNSIGNED NOT NULL,
  slow_control_detector_id INT UNSIGNED NOT NULL,
  num_events BIGINT UNSIGNED NOT NULL,
  value FLOAT NOT NULL,
  error FLOAT NOT NULL,
  min_value FLOAT NOT NULL,
  max_value FLOAT NOT NULL,
  INDEX idx_sc_data_period_det (period_id, slow_control_detector_id),
  CONSTRAINT fk_sc_data_period
    FOREIGN KEY (period_id) REFERENCES data_taking_period(period_id),
  CONSTRAINT fk_sc_data_detector
    FOREIGN KEY (slow_control_detector_id) REFERENCES slow_control_detector(slow_control_detector_id)
);

CREATE TABLE ee_bf_test (
  ee_bf_test_id INT UNSIGNED NOT NULL AUTO_INCREMENT PRIMARY KEY,
  analysis_id INT UNSIGNED NOT NULL,
  test_number SMALLINT NULL,
  test_value FLOAT NULL,
  CONSTRAINT fk_ee_bf_test_analysis
    FOREIGN KEY (analysis_id) REFERENCES analysis(analysis_id)
);

CREATE TABLE ep_bf_test (
  ep_bf_test_id INT UNSIGNED NOT NULL AUTO_INCREMENT PRIMARY KEY,
  analysis_id INT UNSIGNED NOT NULL,
  test_number SMALLINT NULL,
  test_value FLOAT NULL,
  CONSTRAINT fk_ep_bf_test_analysis
    FOREIGN KEY (analysis_id) REFERENCES analysis(analysis_id)
);

CREATE TABLE beam_modulation (
  beam_mod_id BIGINT UNSIGNED NOT NULL AUTO_INCREMENT PRIMARY KEY,
  analysis_id INT UNSIGNED NOT NULL,
  monitor_id INT UNSIGNED NOT NULL,
  modulation_type_id INT UNSIGNED NOT NULL,
  num_events BIGINT UNSIGNED NOT NULL,
  amplitude FLOAT NOT NULL,
  phase FLOAT NOT NULL,
  offset FLOAT NOT NULL,
  a_error FLOAT NOT NULL,
  p_error FLOAT NOT NULL,
  o_error FLOAT NOT NULL,
  gof_para FLOAT NOT NULL,
  INDEX idx_beam_modulation_lookup (analysis_id, monitor_id),
  CONSTRAINT fk_beam_mod_analysis
    FOREIGN KEY (analysis_id) REFERENCES analysis(analysis_id),
  CONSTRAINT fk_beam_mod_monitor
    FOREIGN KEY (monitor_id) REFERENCES detector(detector_id),
  CONSTRAINT fk_beam_mod_type
    FOREIGN KEY (modulation_type_id) REFERENCES modulation_type(modulation_type_id)
);

CREATE TABLE summary_slug_detector_metrics (
  slug_number SMALLINT NOT NULL,
  slow_helicity_plate_status VARCHAR(10) NOT NULL,
  detector_name TEXT NULL,
  detector_id INT UNSIGNED NOT NULL,
  detector_type_id INT UNSIGNED NOT NULL,
  measurement_type_id CHAR(3) NOT NULL,
  weighted_avg_value DOUBLE NOT NULL,
  weighted_avg_error DOUBLE NOT NULL,
  total_runlets_aggregated INT UNSIGNED NOT NULL,
  total_minipulses_aggregated BIGINT UNSIGNED NOT NULL,
  min_observed_value FLOAT NOT NULL,
  max_observed_value FLOAT NOT NULL,
  last_updated_period TIMESTAMP NULL,
  computed_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
  PRIMARY KEY (slug_number, slow_helicity_plate_status, detector_id, measurement_type_id, detector_type_id),
  CONSTRAINT fk_summary_slug_detector
    FOREIGN KEY (detector_id) REFERENCES detector(detector_id),
  CONSTRAINT fk_summary_slug_det_type
    FOREIGN KEY (detector_type_id) REFERENCES detector_type(detector_type_id),
  CONSTRAINT fk_summary_slug_meas_type
    FOREIGN KEY (measurement_type_id) REFERENCES measurement_type(measurement_type_id)
);

CREATE TABLE summary_wien_detector_metrics (
  wien_number SMALLINT NOT NULL,
  wien_reversal ENUM('indeterminate','normal','reverse','transverse_vertical','transverse_horizontal') NOT NULL,
  detector_name TEXT NULL,
  slow_helicity_plate_status VARCHAR(10) NOT NULL,
  detector_id INT UNSIGNED NOT NULL,
  detector_type_id INT UNSIGNED NOT NULL,
  measurement_type_id CHAR(3) NOT NULL,
  weighted_avg_value DOUBLE NOT NULL,
  weighted_avg_error DOUBLE NOT NULL,
  total_slugs_aggregated SMALLINT UNSIGNED NOT NULL,
  total_runlets_aggregated INT UNSIGNED NOT NULL,
  total_minipulses_aggregated BIGINT UNSIGNED NOT NULL,
  min_observed_value FLOAT NOT NULL,
  max_observed_value FLOAT NOT NULL,
  last_updated_period TIMESTAMP NULL,
  computed_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
  PRIMARY KEY (wien_number, slow_helicity_plate_status, wien_reversal, detector_id, measurement_type_id, detector_type_id),
  INDEX idx_summary_wien_lookup (wien_number, slow_helicity_plate_status, wien_reversal),
  CONSTRAINT fk_summary_wien_detector
    FOREIGN KEY (detector_id) REFERENCES detector(detector_id),
  CONSTRAINT fk_summary_wien_det_type
    FOREIGN KEY (detector_type_id) REFERENCES detector_type(detector_type_id),
  CONSTRAINT fk_summary_wien_meas_type
    FOREIGN KEY (measurement_type_id) REFERENCES measurement_type(measurement_type_id)
);

CREATE TABLE event_execution_log (
  log_id INT UNSIGNED NOT NULL AUTO_INCREMENT PRIMARY KEY,
  event_name VARCHAR(100) NOT NULL,
  execution_time TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
  status VARCHAR(20) NOT NULL,
  message TEXT NULL
);
