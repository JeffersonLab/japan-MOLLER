message("-- Looking for MySQL and MySQL++ libraries.")

find_path(MYSQLPP_INCLUDE_PATH mysql++.h $ENV{MYSQLPP_INC_DIR}
          /usr/include/mysql++/
          /usr/local/Cellar/mysql++/3.2.3_1/include/mysql++/)

find_path(MYSQL_INCLUDE_PATH mysql.h /usr/include/mysql/
          /usr/local/Cellar/mysql/5.7.22/include/mysql/)

if(MYSQL_INCLUDE_PATH AND MYSQLPP_INCLUDE_PATH)
  set(MYSQLPP_INCLUDE_DIR ${MYSQLPP_INCLUDE_PATH} ${MYSQL_INCLUDE_PATH})
endif(MYSQL_INCLUDE_PATH AND MYSQLPP_INCLUDE_PATH)

find_library(
  MYSQLPP_LIBRARIES mysqlpp $ENV{MYSQLPP_LIB_DIR} /usr/lib/mysql++/
  /usr/local/Cellar/mysql++/3.2.3_1/lib
  /group/qweak/QwAnalysis/Linux_CentOS6.5-x86_64/MySQL++/local/lib)

if(MYSQLPP_INCLUDE_DIR AND MYSQLPP_LIBRARIES)
  set(MYSQLPP_FOUND TRUE)
  message("--   Found MySQL++ library at ${MYSQLPP_LIBRARIES}")
else(MYSQLPP_INCLUDE_DIR AND MYSQLPP_LIBRARIES)
  set(MYSQLPP_FOUND FALSE)
endif(MYSQLPP_INCLUDE_DIR AND MYSQLPP_LIBRARIES)

if(MYSQLPP_FIND_REQUIRED)
  if(MYSQLPP_FOUND)

  else(MYSQLPP_FOUND)
    message(FATAL_ERROR "Could not find mysqlpp")
  endif(MYSQLPP_FOUND)
else(MYSQLPP_FIND_REQUIRED)
  # MySQL++ is not required, but we need to know if we found it.
  if(MYSQLPP_FOUND)

  else(MYSQLPP_FOUND)
    message("--   Could not find MySQL++; database support is DISABLED.")
  endif(MYSQLPP_FOUND)
endif(MYSQLPP_FIND_REQUIRED)

mark_as_advanced(MYSQLPP_LIBRARIES MYSQLPP_INCLUDE_DIR)
