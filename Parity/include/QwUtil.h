/*!
 * \file   QwUtil.h
 * \brief  Helper functions and utilities for analysis operations
 * \author Ole Hansen
 * \date   2023-08-16
 */

#pragma once

#include <algorithm>
#include <iterator>

// Copy C-style array a to b
template<typename T>
void QwCopyArray( const T& a, T& b ) {
  for (size_t i=0; i<a.size(); i++){
    b.at(i).CopyFrom(a.at(i));
  }
}
