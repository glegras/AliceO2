// Copyright 2019-2020 CERN and copyright holders of ALICE O2.
// See https://alice-o2.web.cern.ch/copyright for details of the copyright holders.
// All rights not expressly granted are reserved.
//
// This software is distributed under the terms of the GNU General Public
// License v3 (GPL Version 3), copied verbatim in the file "COPYING".
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

/// \file LtrCalibData.h
/// \brief calibration data from laser track calibration
///
/// This class holds the calibration output data of CalibLaserTracks
///
/// \author Jens Wiechula, Jens.Wiechula@ikf.uni-frankfurt.de

#ifndef AliceO2_TPC_LtrCalibData_H_
#define AliceO2_TPC_LtrCalibData_H_

#include <fairlogger/Logger.h>
#include <Rtypes.h>

namespace o2::tpc
{

struct LtrCalibData {
  size_t processedTFs{};               ///< number of processed TFs with laser track candidates
  uint64_t firstTime{};                ///< first time stamp of processed TFs
  uint64_t lastTime{};                 ///< last time stamp of processed TFs
  long creationTime{};                 ///< time of creation
  float dvCorrectionA{};               ///< drift velocity correction factor A-Side (inverse multiplicative)
  float dvCorrectionC{};               ///< drift velocity correction factor C-Side (inverse multiplicative)
  float dvOffsetA{};                   ///< drift velocity trigger offset A-Side
  float dvOffsetC{};                   ///< drift velocity trigger offset C-Side
  float refVDrift{};                   ///< reference vdrift for which factor was extracted
  uint16_t nTracksA{};                 ///< number of tracks used for A-Side fit
  uint16_t nTracksC{};                 ///< number of tracks used for C-Side fit
  std::vector<uint16_t> matchedLtrIDs; ///< list of matched laser track IDs

  float getDriftVCorrection() const { return 0.5f * (dvCorrectionA + dvCorrectionC); }

  // renormalize reference and correction either to provided new reference (if >0) or to correction 1 wrt current reference
  void normalize(float newVRef = 0.f)
  {
    if (refVDrift == 0.) {
      LOG(error) << "LtrCalibData data has no reference";
      return;
    }
    if (getDriftVCorrection() == 0) {
      LOGP(error, "Drift correction is 0: dvCorrectionA={}, dvCorrectionC={}, nTracksA={}, nTracksC={}", dvCorrectionA, dvCorrectionC, nTracksA, nTracksC);
      return;
    }
    if (newVRef == 0.) {
      newVRef = refVDrift / getDriftVCorrection();
    }
    float fact = newVRef / refVDrift;
    refVDrift = newVRef;
    dvCorrectionA *= fact;
    dvCorrectionC *= fact;
  }

  float getT0A() const { return (250.f * (1.f - dvCorrectionA) - dvOffsetA) / refVDrift; }
  float getT0C() const { return (250.f * (1.f - dvCorrectionC) + dvOffsetC) / refVDrift; }
  float getZOffsetA() const { return (250.f * (1.f - dvCorrectionA) - dvOffsetA) / dvCorrectionA; }
  float getZOffsetC() const { return (250.f * (1.f - dvCorrectionC) + dvOffsetC) / dvCorrectionC; }

  void reset()
  {
    processedTFs = 0;
    firstTime = 0;
    lastTime = 0;
    creationTime = 0;
    dvCorrectionA = 0;
    dvCorrectionC = 0;
    dvOffsetA = 0;
    dvOffsetC = 0;
    nTracksA = 0;
    nTracksC = 0;
    refVDrift = 0;
    matchedLtrIDs.clear();
  }

  ClassDefNV(LtrCalibData, 2);
};

} // namespace o2::tpc
#endif
