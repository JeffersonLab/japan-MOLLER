-- Database schema for JAPAN-MOLLER Parity Analysis
-- Generated from QwParitySSQLS.h SSQLS definitions

-- Drop tables if they exist (in reverse dependency order)
DROP TABLE IF EXISTS beam_optics;
DROP TABLE IF EXISTS modulation_type;
DROP TABLE IF EXISTS bf_test;
DROP TABLE IF EXISTS seeds;
DROP TABLE IF EXISTS slow_controls_strings;
DROP TABLE IF EXISTS slow_controls_data;
DROP TABLE IF EXISTS sc_detector;
DROP TABLE IF EXISTS slow_controls_settings;
DROP TABLE IF EXISTS lumi_errors;
DROP TABLE IF EXISTS lumi_data;
DROP TABLE IF EXISTS lumi_detector;
DROP TABLE IF EXISTS md_errors;
DROP TABLE IF EXISTS md_data;
DROP TABLE IF EXISTS main_detector;
DROP TABLE IF EXISTS monitor;
DROP TABLE IF EXISTS slope_type;
DROP TABLE IF EXISTS error_code;
DROP TABLE IF EXISTS measurement_type;
DROP TABLE IF EXISTS beam_errors;
DROP TABLE IF EXISTS beam;
DROP TABLE IF EXISTS lumi_slope;
DROP TABLE IF EXISTS md_slope;
DROP TABLE IF EXISTS general_errors;
DROP TABLE IF EXISTS parameter_files;
DROP TABLE IF EXISTS analysis;
DROP TABLE IF EXISTS runlet;
DROP TABLE IF EXISTS run;
DROP TABLE IF EXISTS run_quality;
DROP TABLE IF EXISTS good_for;
DROP TABLE IF EXISTS db_schema;

-- Create db_schema table to track schema versions
CREATE TABLE db_schema (
    db_schema_id INT UNSIGNED NOT NULL AUTO_INCREMENT PRIMARY KEY,
    major_release_number CHAR(2) NOT NULL,
    minor_release_number CHAR(2) NOT NULL,
    point_release_number CHAR(4) NOT NULL,
    time TIMESTAMP NOT NULL,
    script_name TEXT NULL
);

-- Reference tables for run quality and good_for flags
CREATE TABLE good_for (
    good_for_id INT UNSIGNED NOT NULL AUTO_INCREMENT PRIMARY KEY,
    type TEXT NULL
);

CREATE TABLE run_quality (
    run_quality_id INT UNSIGNED NOT NULL AUTO_INCREMENT PRIMARY KEY,
    type TEXT NULL
);

-- Run and analysis tracking tables
CREATE TABLE run (
    run_id INT UNSIGNED NOT NULL AUTO_INCREMENT PRIMARY KEY,
    run_number INT UNSIGNED NOT NULL,
    slug INT UNSIGNED NOT NULL,
    wien_slug INT UNSIGNED NOT NULL,
    injector_slug INT UNSIGNED NOT NULL,
    run_type TEXT NULL,
    start_time DATETIME NULL,
    end_time DATETIME NULL,
    n_mps INT UNSIGNED NOT NULL,
    n_qrt INT UNSIGNED NOT NULL,
    comment TEXT NULL
);

CREATE TABLE runlet (
    runlet_id INT UNSIGNED NOT NULL AUTO_INCREMENT PRIMARY KEY,
    run_id INT UNSIGNED NOT NULL,
    run_number INT UNSIGNED NOT NULL,
    segment_number INT UNSIGNED NULL,
    full_run ENUM('0','1') NOT NULL,  -- ENUM conversion from sql_enum
    start_time DATETIME NULL,
    end_time DATETIME NULL,
    first_mps INT UNSIGNED NOT NULL,
    last_mps INT UNSIGNED NOT NULL,
    comment TEXT NULL
);

-- Seeds table for random number generation
CREATE TABLE seeds (
    seed_id INT UNSIGNED NOT NULL AUTO_INCREMENT PRIMARY KEY,
    first_run_id INT UNSIGNED NOT NULL,
    last_run_id INT UNSIGNED NOT NULL,
    seed TEXT NULL,
    comment TEXT NULL
);

-- Main analysis table with extensive metadata
CREATE TABLE analysis (
    analysis_id INT UNSIGNED NOT NULL AUTO_INCREMENT PRIMARY KEY,
    runlet_id INT UNSIGNED NOT NULL,
    seed_id INT UNSIGNED NOT NULL,
    time DATETIME NULL,
    bf_checksum TEXT NULL,
    beam_mode ENUM('0','1','2','3') NOT NULL,  -- ENUM conversion, values may need adjustment
    n_mps INT UNSIGNED NOT NULL,
    n_qrt INT UNSIGNED NOT NULL,
    first_event INT UNSIGNED NULL,
    last_event INT UNSIGNED NULL,
    segment INT NULL,
    slope_calculation ENUM('0','1') NULL,  -- ENUM conversion
    slope_correction ENUM('0','1') NULL,   -- ENUM conversion
    root_version TEXT NOT NULL,
    root_file_time TEXT NOT NULL,
    root_file_host TEXT NOT NULL,
    root_file_user TEXT NOT NULL,
    analyzer_name TEXT NOT NULL,
    analyzer_argv TEXT NOT NULL,
    analyzer_svn_rev TEXT NOT NULL,
    analyzer_svn_lc_rev TEXT NOT NULL,
    analyzer_svn_url TEXT NOT NULL,
    roc_flags TEXT NOT NULL
);

-- Parameter files tracking
CREATE TABLE parameter_files (
    parameter_file_id INT UNSIGNED NOT NULL AUTO_INCREMENT PRIMARY KEY,
    analysis_id INT UNSIGNED NOT NULL,
    filename TEXT NOT NULL
);

-- Error tracking tables
CREATE TABLE error_code (
    error_code_id TINYINT UNSIGNED NOT NULL AUTO_INCREMENT PRIMARY KEY,
    quantity TEXT NOT NULL
);

CREATE TABLE general_errors (
    general_errors_id INT UNSIGNED NOT NULL AUTO_INCREMENT PRIMARY KEY,
    analysis_id INT UNSIGNED NOT NULL,
    error_code_id TINYINT UNSIGNED NOT NULL,
    n INT UNSIGNED NOT NULL
);

-- Detector and measurement type tables
CREATE TABLE measurement_type (
    measurement_type_id CHAR(3) NOT NULL PRIMARY KEY,  -- CHAR field as primary key
    units TEXT NOT NULL,
    title TEXT NOT NULL
);

CREATE TABLE slope_type (
    slope_type_id INT UNSIGNED NOT NULL AUTO_INCREMENT PRIMARY KEY,
    slope TEXT NOT NULL,
    units TEXT NOT NULL,
    title TEXT NOT NULL
);

CREATE TABLE monitor (
    monitor_id INT UNSIGNED NOT NULL AUTO_INCREMENT PRIMARY KEY,
    quantity TEXT NOT NULL,
    title TEXT NOT NULL
);

CREATE TABLE main_detector (
    main_detector_id INT UNSIGNED NOT NULL AUTO_INCREMENT PRIMARY KEY,
    quantity TEXT NOT NULL,
    title TEXT NOT NULL
);

CREATE TABLE lumi_detector (
    lumi_detector_id INT UNSIGNED NOT NULL AUTO_INCREMENT PRIMARY KEY,
    quantity TEXT NOT NULL,
    title TEXT NOT NULL
);

-- Data tables
CREATE TABLE md_data (
    md_data_id INT UNSIGNED NOT NULL AUTO_INCREMENT PRIMARY KEY,
    analysis_id INT UNSIGNED NOT NULL,
    main_detector_id INT UNSIGNED NULL,
    measurement_type_id CHAR(3) NOT NULL,
    subblock TINYINT UNSIGNED NOT NULL,
    n INT UNSIGNED NOT NULL,
    value FLOAT NOT NULL,
    error FLOAT NOT NULL
);

CREATE TABLE md_errors (
    md_errors_id INT UNSIGNED NOT NULL AUTO_INCREMENT PRIMARY KEY,
    analysis_id INT UNSIGNED NOT NULL,
    main_detector_id INT UNSIGNED NOT NULL,
    error_code_id TINYINT UNSIGNED NOT NULL,
    n INT UNSIGNED NOT NULL
);

CREATE TABLE lumi_data (
    lumi_data_id INT UNSIGNED NOT NULL AUTO_INCREMENT PRIMARY KEY,
    analysis_id INT UNSIGNED NOT NULL,
    lumi_detector_id INT UNSIGNED NULL,
    measurement_type_id CHAR(3) NOT NULL,
    subblock TINYINT UNSIGNED NOT NULL,
    n INT UNSIGNED NOT NULL,
    value FLOAT NOT NULL,
    error FLOAT NOT NULL
);

CREATE TABLE lumi_errors (
    lumi_errors_id INT UNSIGNED NOT NULL AUTO_INCREMENT PRIMARY KEY,
    analysis_id INT UNSIGNED NOT NULL,
    lumi_detector_id INT UNSIGNED NOT NULL,
    error_code_id TINYINT UNSIGNED NOT NULL,
    n INT UNSIGNED NOT NULL
);

CREATE TABLE beam (
    beam_id INT UNSIGNED NOT NULL AUTO_INCREMENT PRIMARY KEY,
    analysis_id INT UNSIGNED NOT NULL,
    monitor_id INT UNSIGNED NOT NULL,
    measurement_type_id CHAR(3) NOT NULL,
    subblock TINYINT UNSIGNED NOT NULL,
    n INT UNSIGNED NOT NULL,
    value FLOAT NOT NULL,
    error FLOAT NOT NULL
);

CREATE TABLE beam_errors (
    beam_errors_id INT UNSIGNED NOT NULL AUTO_INCREMENT PRIMARY KEY,
    analysis_id INT UNSIGNED NOT NULL,
    monitor_id INT UNSIGNED NOT NULL,
    error_code_id TINYINT UNSIGNED NOT NULL,
    n INT UNSIGNED NOT NULL
);

-- Slope data tables
CREATE TABLE md_slope (
    md_slope_id INT UNSIGNED NOT NULL AUTO_INCREMENT PRIMARY KEY,
    analysis_id INT UNSIGNED NOT NULL,
    slope_type_id INT UNSIGNED NOT NULL,
    measurement_type_id CHAR(3) NOT NULL,
    main_detector_id INT UNSIGNED NOT NULL,
    subblock TINYINT UNSIGNED NOT NULL,
    n INT UNSIGNED NOT NULL,
    value FLOAT NOT NULL,
    error FLOAT NOT NULL
);

CREATE TABLE lumi_slope (
    lumi_slope_id INT UNSIGNED NOT NULL AUTO_INCREMENT PRIMARY KEY,
    analysis_id INT UNSIGNED NOT NULL,
    slope_type_id INT UNSIGNED NOT NULL,
    measurement_type_id CHAR(3) NOT NULL,
    lumi_detector_id INT UNSIGNED NOT NULL,
    subblock TINYINT UNSIGNED NOT NULL,
    n INT UNSIGNED NOT NULL,
    value FLOAT NOT NULL,
    error FLOAT NOT NULL
);

-- Slow controls tables
CREATE TABLE slow_controls_settings (
    slow_controls_settings_id INT UNSIGNED NOT NULL AUTO_INCREMENT PRIMARY KEY,
    runlet_id INT UNSIGNED NOT NULL,
    slow_helicity_plate ENUM('0','1') NULL,     -- ENUM conversion, values may need adjustment
    passive_helicity_plate ENUM('0','1') NULL,  -- ENUM conversion, values may need adjustment
    wien_reversal ENUM('0','1') NULL,           -- ENUM conversion, values may need adjustment
    precession_reversal ENUM('0','1') NULL,     -- ENUM conversion, values may need adjustment
    helicity_length INT UNSIGNED NULL,
    charge_feedback ENUM('0','1') NULL,         -- ENUM conversion, values may need adjustment
    position_feedback ENUM('0','1') NULL,       -- ENUM conversion, values may need adjustment
    qtor_current FLOAT NULL,
    target_position TEXT NULL
);

CREATE TABLE sc_detector (
    sc_detector_id INT UNSIGNED NOT NULL AUTO_INCREMENT PRIMARY KEY,
    name TEXT NOT NULL,
    units TEXT NOT NULL,
    comment TEXT NOT NULL
);

CREATE TABLE slow_controls_data (
    slow_controls_data_id INT UNSIGNED NOT NULL AUTO_INCREMENT PRIMARY KEY,
    runlet_id INT UNSIGNED NOT NULL,
    sc_detector_id INT UNSIGNED NOT NULL,
    n INT UNSIGNED NOT NULL,
    value FLOAT NOT NULL,
    error FLOAT NOT NULL,
    min_value FLOAT NOT NULL,
    max_value FLOAT NOT NULL
);

CREATE TABLE slow_controls_strings (
    slow_controls_strings_id INT UNSIGNED NOT NULL AUTO_INCREMENT PRIMARY KEY,
    runlet_id INT UNSIGNED NOT NULL,
    sc_detector_id INT UNSIGNED NOT NULL,
    value TEXT NOT NULL
);

-- Beam test and modulation tables
CREATE TABLE bf_test (
    bf_test_id INT UNSIGNED NOT NULL AUTO_INCREMENT PRIMARY KEY,
    analysis_id INT UNSIGNED NOT NULL,
    test_number INT UNSIGNED NULL,
    test_value FLOAT NULL
);

CREATE TABLE modulation_type (
    modulation_type_id INT UNSIGNED NOT NULL AUTO_INCREMENT PRIMARY KEY,
    type TEXT NULL
);

CREATE TABLE beam_optics (
    beam_optics_id INT UNSIGNED NOT NULL AUTO_INCREMENT PRIMARY KEY,
    analysis_id INT UNSIGNED NOT NULL,
    monitor_id INT UNSIGNED NOT NULL,
    modulation_type_id INT UNSIGNED NOT NULL,
    n INT UNSIGNED NOT NULL,
    amplitude FLOAT NOT NULL,
    phase FLOAT NOT NULL,
    offset FLOAT NOT NULL,
    a_error FLOAT NOT NULL,
    p_error FLOAT NOT NULL,
    o_error FLOAT NOT NULL,
    gof_para FLOAT NOT NULL
);
