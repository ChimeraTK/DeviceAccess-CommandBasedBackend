// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

// Each of these will take in a std::string of the hexidecimal representation of data, and return the hex representation
// of the checksum.
std::string CheckSum8(std::string binData);
std::string CheckSum32(std::string binData);
std::string CheckSumSha256(std::string binData);
std::string CheckSumCrcCcit(std::string binData);

// The common denominator is a list of values
// ok, make your life easier:
//   initial validation: See if there's a constant odd number of characters, and if so, throw logic_error
//   at run time, if the number of characters is odd, throw a runtime error.
//   so no left/right packign mess
//   and no nesting: inital validation prevents all insertions inside other start-stop phrases.

// How to get the bytes from binData? leftPad? RighPad? throw?

// CheckSum(std::string hexData, CheckSumEnum whichCheckSum);

//////////////////////////////////
// Decision: do one-pass only. Then it's easier to check too. Save nested checksums for a later implementation
// ok, so the register info is going to have a vector of some sort of checksum thing T[]
// Startup time
//   screen the validity of the pattern,
//   There is no execution order.
//
// Run time:
// first we inja the data into the string.
// next, we want to locathe the places where all the checksums start and end locations are to go. This is to produce a
// bunch of substrings Then we process them through the corresponding checksum. Then we put the resulting checksums back
// through inja to get the result

//////////////////////////////////
// Multi-pass design
// Startup time
//   screen the validity of the pattern,
//   determin the order of execution. For now, just make the user do it, but leave space for reordering in the form of
//   an array of indicies. may determin whether to do a single pass or a multi-pass.
//
// Run time:
// first we inja the data into the string.
// next, we want to locathe the places where all the checksums start and end locations are to go. This is to produce a listing of
// We fill in the first one, levaing all others blank. We
// we need the order in which to insert the checksums, and to make sure they make sense:
//   the checksum cannot be within its own start-end range.
//   Why not just make the user do it, such that 0 is the location of the first one to be evaluated, 1 is the next one, etc.

// Then we're locating where the
// We can then reinterpret the checksums as processors, that take data in the process, and itteratively apply it.

// and then, we want to have a checksum comparison, so in addition to CsStart.0, CsEnd.0, Cs.0, we need CsCheckStart.0,
// CsCheckEnd.0 So we only use Cs.0 on write, and only use CsCheck on readback. CsStart..CsEnd cannot contain a Cs.0,
// CsCheckStart, or Cs.CheckEnd.
