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
///

#ifndef TRACKINGITSU_INCLUDE_TIMEFRAME_H_
#define TRACKINGITSU_INCLUDE_TIMEFRAME_H_

#include <array>
#include <vector>
#include <utility>
#include <numeric>
#include <cassert>
#include <gsl/gsl>
#include <numeric>
#include <iostream>
#include <algorithm>

#include "DataFormatsITS/TrackITS.h"

#include "ITStracking/Cell.h"
#include "ITStracking/Cluster.h"
#include "ITStracking/Configuration.h"
#include "ITStracking/Constants.h"
#include "ITStracking/ClusterLines.h"
#include "ITStracking/Definitions.h"
#include "ITStracking/Road.h"
#include "ITStracking/Tracklet.h"
#include "ITStracking/IndexTableUtils.h"

#include "SimulationDataFormat/MCCompLabel.h"
#include "SimulationDataFormat/MCTruthContainer.h"

#include "ReconstructionDataFormats/Vertex.h"

namespace o2
{

namespace itsmft
{
class Cluster;
class CompClusterExt;
class TopologyDictionary;
class ROFRecord;
} // namespace itsmft

namespace its
{
using Vertex = o2::dataformats::Vertex<o2::dataformats::TimeStamp<int>>;

struct lightVertex {
  lightVertex(float x, float y, float z, std::array<float, 6> rms2, int cont, float avgdis2, int stamp);
  float mX;
  float mY;
  float mZ;
  std::array<float, 6> mRMS2;
  float mAvgDistance2;
  int mContributors;
  int mTimeStamp;
};

class TimeFrame
{
 public:
  TimeFrame(int nLayers = 7);
  const Vertex& getPrimaryVertex(const int) const;
  gsl::span<const Vertex> getPrimaryVertices(int tf) const;
  gsl::span<const Vertex> getPrimaryVertices(int romin, int romax) const;
  gsl::span<const std::vector<MCCompLabel>> getPrimaryVerticesLabels(const int rof) const;
  gsl::span<const std::array<float, 2>> getPrimaryVerticesXAlpha(int tf) const;
  void fillPrimaryVerticesXandAlpha();
  int getPrimaryVerticesNum(int rofID = -1) const;
  void addPrimaryVertices(const std::vector<Vertex>& vertices);
  void addPrimaryVertices(const gsl::span<const Vertex>& vertices);
  void addPrimaryVertices(const std::vector<lightVertex>&);
  void removePrimaryVerticesInROf(const int rofId);
  int loadROFrameData(const o2::itsmft::ROFRecord& rof, gsl::span<const itsmft::Cluster> clusters,
                      const dataformats::MCTruthContainer<MCCompLabel>* mcLabels = nullptr);

  int loadROFrameData(gsl::span<o2::itsmft::ROFRecord> rofs,
                      gsl::span<const itsmft::CompClusterExt> clusters,
                      gsl::span<const unsigned char>::iterator& pattIt,
                      const itsmft::TopologyDictionary* dict,
                      const dataformats::MCTruthContainer<MCCompLabel>* mcLabels = nullptr);

  int getTotalClusters() const;
  bool empty() const;

  int getSortedIndex(int rof, int layer, int i) const;
  int getSortedStartIndex(const int, const int) const;
  int getNrof() const;

  void resetBeamXY(const float x, const float y, const float w = 0);
  void setBeamPosition(const float x, const float y, const float s2, const float base = 50.f, const float systematic = 0.f)
  {
    isBeamPositionOverridden = true;
    resetBeamXY(x, y, s2 / std::sqrt(base * base + systematic));
  }

  float getBeamX() const;
  float getBeamY() const;

  float getMinR(int layer) const { return mMinR[layer]; }
  float getMaxR(int layer) const { return mMaxR[layer]; }
  float getMSangle(int layer) const { return mMSangles[layer]; }
  float getPhiCut(int layer) const { return mPhiCuts[layer]; }
  float getPositionResolution(int layer) const { return mPositionResolution[layer]; }

  gsl::span<Cluster> getClustersOnLayer(int rofId, int layerId);
  gsl::span<const Cluster> getClustersOnLayer(int rofId, int layerId) const;
  gsl::span<const Cluster> getClustersPerROFrange(int rofMin, int range, int layerId) const;
  gsl::span<const Cluster> getUnsortedClustersOnLayer(int rofId, int layerId) const;
  gsl::span<const int> getROframesClustersPerROFrange(int rofMin, int range, int layerId) const;
  gsl::span<const int> getROframeClusters(int layerId) const;
  gsl::span<const int> getNClustersROFrange(int rofMin, int range, int layerId) const;
  gsl::span<const int> getIndexTablePerROFrange(int rofMin, int range, int layerId) const;
  gsl::span<int> getIndexTable(int rofId, int layerId);
  std::vector<int>& getIndexTableWhole(int layerId) { return mIndexTables[layerId]; }
  const std::vector<TrackingFrameInfo>& getTrackingFrameInfoOnLayer(int layerId) const;

  const TrackingFrameInfo& getClusterTrackingFrameInfo(int layerId, const Cluster& cl) const;
  const gsl::span<const MCCompLabel> getClusterLabels(int layerId, const Cluster& cl) const;
  const gsl::span<const MCCompLabel> getClusterLabels(int layerId, const int clId) const;
  int getClusterExternalIndex(int layerId, const int clId) const;

  std::vector<MCCompLabel>& getTrackletsLabel(int layer) { return mTrackletLabels[layer]; }
  std::vector<MCCompLabel>& getCellsLabel(int layer) { return mCellLabels[layer]; }

  bool hasMCinformation() const;
  void initialise(const int iteration, const TrackingParameters& trkParam, const int maxLayers = 7);
  void resetRofPV()
  {
    mPrimaryVertices.clear();
    mROframesPV.resize(1, 0);
  };

  bool isClusterUsed(int layer, int clusterId) const;
  void markUsedCluster(int layer, int clusterId);

  std::vector<std::vector<Tracklet>>& getTracklets();
  std::vector<std::vector<int>>& getTrackletsLookupTable();

  std::vector<std::vector<Cluster>>& getClusters();
  std::vector<std::vector<Cluster>>& getUnsortedClusters();
  int getClusterROF(int iLayer, int iCluster);
  std::vector<std::vector<Cell>>& getCells();
  std::vector<std::vector<int>>& getCellsLookupTable();
  std::vector<std::vector<std::vector<int>>>& getCellsNeighbours();
  std::vector<Road<5>>& getRoads();
  std::vector<TrackITSExt>& getTracks(int rof) { return mTracks[rof]; }
  std::vector<MCCompLabel>& getTracksLabel(const int rof) { return mTracksLabel[rof]; }
  std::vector<MCCompLabel>& getLinesLabel(const int rof) { return mLinesLabels[rof]; }
  std::vector<std::vector<MCCompLabel>>& getVerticesLabels() { return mVerticesLabels; }

  int getNumberOfClusters() const;
  int getNumberOfCells() const;
  int getNumberOfTracklets() const;
  int getNumberOfTracks() const;

  bool checkMemory(unsigned long max) { return getArtefactsMemory() < max; }
  unsigned long getArtefactsMemory();
  int getROfCutClusterMult() const { return mCutClusterMult; };
  int getROfCutVertexMult() const { return mCutVertexMult; };
  int getROfCutAllMult() const { return mCutClusterMult + mCutVertexMult; }

  // Vertexer
  void computeTrackletsScans(const int nThreads = 1);
  int& getNTrackletsROf(int tf, int combId);
  std::vector<Line>& getLines(int tf);
  std::vector<ClusterLines>& getTrackletClusters(int tf);
  gsl::span<const Tracklet> getFoundTracklets(int rofId, int combId) const;
  gsl::span<Tracklet> getFoundTracklets(int rofId, int combId);
  gsl::span<const MCCompLabel> getLabelsFoundTracklets(int rofId, int combId) const;
  gsl::span<int> getNTrackletsCluster(int rofId, int combId);
  uint32_t getTotalTrackletsTF(const int iLayer) { return mTotalTracklets[iLayer]; }
  int getTotalClustersPerROFrange(int rofMin, int range, int layerId) const;
  std::array<float, 2>& getBeamXY() { return mBeamPos; }
  // \Vertexer

  void initialiseRoadLabels();
  void setRoadLabel(int i, const unsigned long long& lab, bool fake);
  const unsigned long long& getRoadLabel(int i) const;
  bool isRoadFake(int i) const;

  void setMultiplicityCutMask(const std::vector<bool>& cutMask) { mMultiplicityCutMask = cutMask; }

  int hasBogusClusters() const { return std::accumulate(mBogusClusters.begin(), mBogusClusters.end(), 0); }

  void setBz(float bz) { mBz = bz; }
  float getBz() const { return mBz; }

  template <typename... T>
  void addClusterToLayer(int layer, T&&... args);
  template <typename... T>
  void addTrackingFrameInfoToLayer(int layer, T&&... args);
  void addClusterExternalIndexToLayer(int layer, const int idx);

  void resizeVectors(int nLayers);

  /// Debug and printing
  void checkTrackletLUTs();
  void printROFoffsets();
  void printNClsPerROF();
  void printVertices();
  void printTrackletLUTonLayer(int i);
  void printCellLUTonLayer(int i);
  void printTrackletLUTs();
  void printCellLUTs();

  IndexTableUtils mIndexTableUtils;

  bool mIsGPU = false;
  std::vector<std::vector<Cluster>> mClusters;
  std::vector<std::vector<TrackingFrameInfo>> mTrackingFrameInfo;
  std::vector<std::vector<int>> mClusterExternalIndices;
  std::vector<std::vector<int>> mROframesClusters;
  const dataformats::MCTruthContainer<MCCompLabel>* mClusterLabels = nullptr;
  std::array<std::vector<int>, 2> mNTrackletsPerCluster; // TODO: remove in favour of mNTrackletsPerROf
  std::vector<std::vector<int>> mNClustersPerROF;
  std::vector<std::vector<int>> mIndexTables;
  std::vector<std::vector<int>> mTrackletsLookupTable;
  std::vector<std::vector<unsigned char>> mUsedClusters;
  int mNrof = 0;
  std::vector<int> mROframesPV = {0};
  std::vector<Vertex> mPrimaryVertices;

 private:
  float mBz = 5.;
  int mBeamPosWeight = 0;
  std::array<float, 2> mBeamPos = {0.f, 0.f};
  bool isBeamPositionOverridden = false;
  std::vector<float> mMinR;
  std::vector<float> mMaxR;
  std::vector<float> mMSangles;
  std::vector<float> mPhiCuts;
  std::vector<float> mPositionResolution;
  std::vector<bool> mMultiplicityCutMask;
  std::vector<std::array<float, 2>> mPValphaX; /// PV x and alpha for track propagation
  std::vector<std::vector<Cluster>> mUnsortedClusters;
  std::vector<std::vector<MCCompLabel>> mTrackletLabels;
  std::vector<std::vector<MCCompLabel>> mCellLabels;
  std::vector<std::vector<Cell>> mCells;
  std::vector<std::vector<int>> mCellsLookupTable;
  std::vector<std::vector<std::vector<int>>> mCellsNeighbours;
  std::vector<Road<5>> mRoads;
  std::vector<std::vector<MCCompLabel>> mTracksLabel;
  std::vector<std::vector<TrackITSExt>> mTracks;
  std::vector<int> mBogusClusters; /// keep track of clusters with wild coordinates

  std::vector<std::vector<Tracklet>> mTracklets;

  std::vector<std::pair<unsigned long long, bool>> mRoadLabels;
  int mCutClusterMult;
  int mCutVertexMult;

  // Vertexer
  std::vector<std::vector<int>> mNTrackletsPerROf;
  std::vector<std::vector<Line>> mLines;
  std::vector<std::vector<ClusterLines>> mTrackletClusters;
  std::vector<std::vector<int>> mTrackletsIndexROf;
  std::vector<std::vector<MCCompLabel>> mLinesLabels;
  std::vector<std::vector<MCCompLabel>> mVerticesLabels;
  std::array<uint32_t, 2> mTotalTracklets = {0, 0};
  // \Vertexer
};

inline const Vertex& TimeFrame::getPrimaryVertex(const int vertexIndex) const { return mPrimaryVertices[vertexIndex]; }

inline gsl::span<const Vertex> TimeFrame::getPrimaryVertices(int rof) const
{
  const int start = mROframesPV[rof];
  const int stop_idx = rof >= mNrof - 1 ? mNrof : rof + 1;
  int delta = mMultiplicityCutMask[rof] ? mROframesPV[stop_idx] - start : 0; // return empty span if Rof is excluded
  return {&mPrimaryVertices[start], static_cast<gsl::span<const Vertex>::size_type>(delta)};
}

inline gsl::span<const std::vector<MCCompLabel>> TimeFrame::getPrimaryVerticesLabels(const int rof) const
{
  const int start = mROframesPV[rof];
  const int stop_idx = rof >= mNrof - 1 ? mNrof : rof + 1;
  int delta = mMultiplicityCutMask[rof] ? mROframesPV[stop_idx] - start : 0; // return empty span if Rof is excluded
  return {&mVerticesLabels[start], static_cast<gsl::span<const Vertex>::size_type>(delta)};
}

inline gsl::span<const Vertex> TimeFrame::getPrimaryVertices(int romin, int romax) const
{
  return {&mPrimaryVertices[mROframesPV[romin]], static_cast<gsl::span<const Vertex>::size_type>(mROframesPV[romax + 1] - mROframesPV[romin])};
}

inline gsl::span<const std::array<float, 2>> TimeFrame::getPrimaryVerticesXAlpha(int rof) const
{
  const int start = mROframesPV[rof];
  const int stop_idx = rof >= mNrof - 1 ? mNrof : rof + 1;
  int delta = mMultiplicityCutMask[rof] ? mROframesPV[stop_idx] - start : 0; // return empty span if Rof is excluded
  return {&(mPValphaX[start]), static_cast<gsl::span<const std::array<float, 2>>::size_type>(delta)};
}

inline int TimeFrame::getPrimaryVerticesNum(int rofID) const
{
  return rofID < 0 ? mPrimaryVertices.size() : mROframesPV[rofID + 1] - mROframesPV[rofID];
}

inline bool TimeFrame::empty() const { return getTotalClusters() == 0; }

inline int TimeFrame::getSortedIndex(int rof, int layer, int index) const { return mROframesClusters[layer][rof] + index; }

inline int TimeFrame::getSortedStartIndex(const int rof, const int layer) const { return mROframesClusters[layer][rof]; }

inline int TimeFrame::getNrof() const { return mNrof; }

inline void TimeFrame::resetBeamXY(const float x, const float y, const float w)
{
  mBeamPos[0] = x;
  mBeamPos[1] = y;
  mBeamPosWeight = w;
}

inline float TimeFrame::getBeamX() const { return mBeamPos[0]; }

inline float TimeFrame::getBeamY() const { return mBeamPos[1]; }

inline gsl::span<const int> TimeFrame::getROframeClusters(int layerId) const
{
  return {&mROframesClusters[layerId][0], static_cast<gsl::span<const int>::size_type>(mROframesClusters[layerId].size())};
}

inline gsl::span<Cluster> TimeFrame::getClustersOnLayer(int rofId, int layerId)
{
  if (rofId < 0 || rofId >= mNrof) {
    return gsl::span<Cluster>();
  }
  int startIdx{mROframesClusters[layerId][rofId]};
  return {&mClusters[layerId][startIdx], static_cast<gsl::span<Cluster>::size_type>(mROframesClusters[layerId][rofId + 1] - startIdx)};
}

inline gsl::span<const Cluster> TimeFrame::getClustersOnLayer(int rofId, int layerId) const
{
  if (rofId < 0 || rofId >= mNrof) {
    return gsl::span<const Cluster>();
  }
  int startIdx{mROframesClusters[layerId][rofId]};
  return {&mClusters[layerId][startIdx], static_cast<gsl::span<Cluster>::size_type>(mROframesClusters[layerId][rofId + 1] - startIdx)};
}

inline gsl::span<const Cluster> TimeFrame::getClustersPerROFrange(int rofMin, int range, int layerId) const
{
  if (rofMin < 0 || rofMin >= mNrof) {
    return gsl::span<const Cluster>();
  }
  int startIdx{mROframesClusters[layerId][rofMin]}; // First cluster of rofMin
  int endIdx{mROframesClusters[layerId][std::min(rofMin + range, mNrof)]};
  return {&mClusters[layerId][startIdx], static_cast<gsl::span<Cluster>::size_type>(endIdx - startIdx)};
}

inline gsl::span<const int> TimeFrame::getROframesClustersPerROFrange(int rofMin, int range, int layerId) const
{
  int chkdRange{std::min(range, mNrof - rofMin)};
  return {&mROframesClusters[layerId][rofMin], static_cast<gsl::span<int>::size_type>(chkdRange)};
}

inline gsl::span<const int> TimeFrame::getNClustersROFrange(int rofMin, int range, int layerId) const
{
  int chkdRange{std::min(range, mNrof - rofMin)};
  return {&mNClustersPerROF[layerId][rofMin], static_cast<gsl::span<int>::size_type>(chkdRange)};
}

inline int TimeFrame::getTotalClustersPerROFrange(int rofMin, int range, int layerId) const
{
  int startIdx{rofMin}; // First cluster of rofMin
  int endIdx{std::min(rofMin + range, mNrof)};
  return mROframesClusters[layerId][endIdx] - mROframesClusters[layerId][startIdx];
}

inline gsl::span<const int> TimeFrame::getIndexTablePerROFrange(int rofMin, int range, int layerId) const
{
  const int iTableSize{mIndexTableUtils.getNphiBins() * mIndexTableUtils.getNzBins() + 1};
  int chkdRange{std::min(range, mNrof - rofMin)};
  return {&mIndexTables[layerId][rofMin * iTableSize], static_cast<gsl::span<int>::size_type>(chkdRange * iTableSize)};
}

inline int TimeFrame::getClusterROF(int iLayer, int iCluster)
{
  return std::lower_bound(mROframesClusters[iLayer].begin(), mROframesClusters[iLayer].end(), iCluster + 1) - mROframesClusters[iLayer].begin() - 1;
}

inline gsl::span<const Cluster> TimeFrame::getUnsortedClustersOnLayer(int rofId, int layerId) const
{
  if (rofId < 0 || rofId >= mNrof) {
    return gsl::span<const Cluster>();
  }
  int startIdx{mROframesClusters[layerId][rofId]};
  return {&mUnsortedClusters[layerId][startIdx], static_cast<gsl::span<Cluster>::size_type>(mROframesClusters[layerId][rofId + 1] - startIdx)};
}

inline const std::vector<TrackingFrameInfo>& TimeFrame::getTrackingFrameInfoOnLayer(int layerId) const
{
  return mTrackingFrameInfo[layerId];
}

inline const TrackingFrameInfo& TimeFrame::getClusterTrackingFrameInfo(int layerId, const Cluster& cl) const
{
  return mTrackingFrameInfo[layerId][cl.clusterId];
}

inline const gsl::span<const MCCompLabel> TimeFrame::getClusterLabels(int layerId, const Cluster& cl) const
{
  return getClusterLabels(layerId, cl.clusterId);
}

inline const gsl::span<const MCCompLabel> TimeFrame::getClusterLabels(int layerId, int clId) const
{
  return mClusterLabels->getLabels(mClusterExternalIndices[layerId][clId]);
}

inline int TimeFrame::getClusterExternalIndex(int layerId, const int clId) const
{
  return mClusterExternalIndices[layerId][clId];
}

inline gsl::span<int> TimeFrame::getIndexTable(int rofId, int layer)
{
  if (rofId < 0 || rofId >= mNrof) {
    return gsl::span<int>();
  }
  return {&mIndexTables[layer][rofId * (mIndexTableUtils.getNphiBins() * mIndexTableUtils.getNzBins() + 1)],
          static_cast<gsl::span<int>::size_type>(mIndexTableUtils.getNphiBins() * mIndexTableUtils.getNzBins() + 1)};
}

inline std::vector<Line>& TimeFrame::getLines(int rof)
{
  return mLines[rof];
}

inline std::vector<ClusterLines>& TimeFrame::getTrackletClusters(int rof)
{
  return mTrackletClusters[rof];
}

template <typename... T>
void TimeFrame::addClusterToLayer(int layer, T&&... values)
{
  mUnsortedClusters[layer].emplace_back(std::forward<T>(values)...);
}

template <typename... T>
void TimeFrame::addTrackingFrameInfoToLayer(int layer, T&&... values)
{
  mTrackingFrameInfo[layer].emplace_back(std::forward<T>(values)...);
}

inline void TimeFrame::addClusterExternalIndexToLayer(int layer, const int idx)
{
  mClusterExternalIndices[layer].push_back(idx);
}

inline bool TimeFrame::hasMCinformation() const
{
  return mClusterLabels;
}

inline bool TimeFrame::isClusterUsed(int layer, int clusterId) const
{
  return mUsedClusters[layer][clusterId];
}

inline void TimeFrame::markUsedCluster(int layer, int clusterId) { mUsedClusters[layer][clusterId] = true; }

inline std::vector<std::vector<Tracklet>>& TimeFrame::getTracklets()
{
  return mTracklets;
}

inline std::vector<std::vector<int>>& TimeFrame::getTrackletsLookupTable()
{
  return mTrackletsLookupTable;
}

inline void TimeFrame::initialiseRoadLabels()
{
  mRoadLabels.clear();
  mRoadLabels.resize(mRoads.size());
}

inline void TimeFrame::setRoadLabel(int i, const unsigned long long& lab, bool fake)
{
  mRoadLabels[i].first = lab;
  mRoadLabels[i].second = fake;
}

inline const unsigned long long& TimeFrame::getRoadLabel(int i) const
{
  return mRoadLabels[i].first;
}

inline gsl::span<int> TimeFrame::getNTrackletsCluster(int rofId, int combId)
{
  if (rofId < 0 || rofId >= mNrof) {
    return gsl::span<int>();
  }
  auto startIdx{mROframesClusters[1][rofId]};
  return {&mNTrackletsPerCluster[combId][startIdx], static_cast<gsl::span<int>::size_type>(mROframesClusters[1][rofId + 1] - startIdx)};
}

inline int& TimeFrame::getNTrackletsROf(int rof, int combId)
{
  return mNTrackletsPerROf[combId][rof];
}

inline bool TimeFrame::isRoadFake(int i) const
{
  return mRoadLabels[i].second;
}

inline std::vector<std::vector<Cluster>>& TimeFrame::getClusters()
{
  return mClusters;
}

inline std::vector<std::vector<Cluster>>& TimeFrame::getUnsortedClusters()
{
  return mUnsortedClusters;
}

inline std::vector<std::vector<Cell>>& TimeFrame::getCells() { return mCells; }

inline std::vector<std::vector<int>>& TimeFrame::getCellsLookupTable()
{
  return mCellsLookupTable;
}

inline std::vector<std::vector<std::vector<int>>>& TimeFrame::getCellsNeighbours()
{
  return mCellsNeighbours;
}

inline std::vector<Road<5>>& TimeFrame::getRoads() { return mRoads; }

inline gsl::span<Tracklet> TimeFrame::getFoundTracklets(int rofId, int combId)
{
  if (rofId < 0 || rofId >= mNrof) {
    return gsl::span<Tracklet>();
  }
  auto startIdx{mNTrackletsPerROf[combId][rofId]};
  return {&mTracklets[combId][startIdx], static_cast<gsl::span<Tracklet>::size_type>(mNTrackletsPerROf[combId][rofId + 1] - startIdx)};
}

inline gsl::span<const Tracklet> TimeFrame::getFoundTracklets(int rofId, int combId) const
{
  if (rofId < 0 || rofId >= mNrof) {
    return gsl::span<const Tracklet>();
  }
  auto startIdx{mNTrackletsPerROf[combId][rofId]};
  return {&mTracklets[combId][startIdx], static_cast<gsl::span<Tracklet>::size_type>(mNTrackletsPerROf[combId][rofId + 1] - startIdx)};
}

inline gsl::span<const MCCompLabel> TimeFrame::getLabelsFoundTracklets(int rofId, int combId) const
{
  if (rofId < 0 || rofId >= mNrof || !hasMCinformation()) {
    return gsl::span<const MCCompLabel>();
  }
  auto startIdx{mNTrackletsPerROf[combId][rofId]};
  return {&mTrackletLabels[combId][startIdx], static_cast<gsl::span<Tracklet>::size_type>(mNTrackletsPerROf[combId][rofId + 1] - startIdx)};
}

inline int TimeFrame::getNumberOfClusters() const
{
  int nClusters = 0;
  for (auto& layer : mClusters) {
    nClusters += layer.size();
  }
  return nClusters;
}

inline int TimeFrame::getNumberOfCells() const
{
  int nCells = 0;
  for (auto& layer : mCells) {
    nCells += layer.size();
  }
  return nCells;
}

inline int TimeFrame::getNumberOfTracklets() const
{
  int nTracklets = 0;
  for (auto& layer : mTracklets) {
    nTracklets += layer.size();
  }
  return nTracklets;
}

inline int TimeFrame::getNumberOfTracks() const
{
  int nTracks = 0;
  for (auto& t : mTracks) {
    nTracks += t.size();
  }
  return nTracks;
}

} // namespace its
} // namespace o2

#endif /* TRACKINGITSU_INCLUDE_TimeFrame_H_ */
