#define DEBUG_PULLOUT
#include "KBPadPlane.hh"

#include "TVector2.h"

#include <iostream>
using namespace std;

ClassImp(KBPadPlane)

KBPadPlane::KBPadPlane()
:KBPadPlane("KBPadPlane","TPC pad plane")
{
}

KBPadPlane::KBPadPlane(const char *name, const char *title)
:KBDetectorPlane(name, title)
{
}

void KBPadPlane::Print(Option_t *) const
{
  Int_t numChannels = fChannelArray -> GetEntries();
  auto countPads = 0;
  auto countHits = 0;
  for (auto iChannel = 0; iChannel < numChannels; ++iChannel) {
    auto pad = (KBPad *) fChannelArray -> At(iChannel);
    Int_t numHits = pad -> GetNumHits();
    if (numHits > 0) {
      ++countPads;
      countHits += numHits;
    }
  }

  kb_info << "Containing " << fChannelArray -> GetEntries() << " pads" << endl;
  kb_info << "number of active pads: " << countPads << endl;
  kb_info << "number of hits: " << countHits << endl;
}

KBPad *KBPadPlane::GetPadFast(Int_t idx) { return (KBPad *) fChannelArray -> At(idx); }

KBPad *KBPadPlane::GetPad(Int_t idx)
{
  TObject *obj = nullptr;
  if (idx >= 0 && idx < fChannelArray -> GetEntriesFast())
    obj = fChannelArray -> At(idx);

  return (KBPad *) obj;
}

void KBPadPlane::SetPadArray(TClonesArray *padArray)
{
  TIter iterPads(padArray);
  KBPad *padWithData;
  while ((padWithData = (KBPad *) iterPads.Next())) {
    if (padWithData -> GetPlaneID() != fPlaneID)
      continue;
    auto padID = padWithData -> GetPadID();
    if (padID < 0)
      continue;
    auto pad = GetPadFast(padID);
    pad -> CopyPadData(padWithData);
    pad -> SetActive();
  }
}

void KBPadPlane::SetHitArray(TClonesArray *hitArray)
{
  Int_t numHits = hitArray -> GetEntries();
  for (auto iHit = 0; iHit < numHits; ++iHit)
  {
    auto hit = (KBTpcHit *) hitArray -> At(iHit);
    auto padID = hit -> GetPadID();
    if (padID < 0)
      continue;
    auto pad = GetPadFast(padID);
    pad -> AddHit(hit);
    pad -> SetActive();
  }
}

void KBPadPlane::AddHit(KBTpcHit *hit)
{
  auto pad = GetPadFast(hit -> GetPadID());
  if (hit -> GetHitID() >= 0)
    pad -> AddHit(hit);
}

void KBPadPlane::FillBufferIn(Double_t i, Double_t j, Double_t tb, Double_t val, Int_t trackID)
{
  Int_t id = FindPadID(i, j);
  if (id < 0)
    return; 

  KBPad *pad = (KBPad *) fChannelArray -> At(id);
  if (pad != nullptr)
    pad -> FillBufferIn(tb, val, trackID);
}

void KBPadPlane::FillDataToHist(Option_t *option)
{
  if (fH2Plane == nullptr) {
    kb_info << "Pad plane histogram does not exist! Run GetHist(option) before filling buffer." << endl;
    return;
  }

  TString optionString = TString(option);

  kb_info << "Filling " << optionString << " into pad-plane histogram" << endl;

  KBPad *pad;
  TIter iterPads(fChannelArray);

  if (optionString == "hit") {
    while ((pad = (KBPad *) iterPads.Next())) {
      if (pad -> GetNumHits() == 0) continue;

      auto charge = 0.;
      for (auto iHit = 0; iHit < pad -> GetNumHits(); ++iHit) {
        auto hit = pad -> GetHit(iHit);
        if (charge < hit -> GetCharge()) {
          if (hit -> GetSortValue() > 0)
            charge += hit -> GetCharge();
        }
      }
      fH2Plane -> Fill(pad->GetI(),pad->GetJ(),charge);

    }
  } else if (optionString == "out") {
    while ((pad = (KBPad *) iterPads.Next())) {
      auto buffer = pad -> GetBufferOut();
      Double_t val = *max_element(buffer,buffer+512);
      if (val < 1) val = 0;
      fH2Plane -> Fill(pad->GetI(),pad->GetJ(),val);
    }
  } else if (optionString == "raw") {
    while ((pad = (KBPad *) iterPads.Next())) {
      auto buffer = pad -> GetBufferRaw();
      Double_t val = *max_element(buffer,buffer+512);
      if (val < 1) val = 0;
      fH2Plane -> Fill(pad->GetI(),pad->GetJ(),val);
    }
  } else if (optionString == "in") {
    while ((pad = (KBPad *) iterPads.Next())) {
      auto buffer = pad -> GetBufferIn();
      Double_t val = *max_element(buffer,buffer+512);
      if (val < 1) val = 0;
      fH2Plane -> Fill(pad->GetI(),pad->GetJ(),val);
    }
  }
}

Int_t KBPadPlane::GetNumPads() { return GetNChannels(); }

void KBPadPlane::SetPlaneK(Double_t k) { fPlaneK = k; }
Double_t KBPadPlane::GetPlaneK() { return fPlaneK; }

void KBPadPlane::Clear(Option_t *)
{
  KBPad *pad;
  TIter iterPads(fChannelArray);
  while ((pad = (KBPad *) iterPads.Next())) {
    pad -> Clear();
  }
}

Int_t KBPadPlane::FindChannelID(Double_t i, Double_t j) { return FindPadID(i,j); }

void KBPadPlane::ResetEvent()
{
  KBPad *pad;
  TIter iterPads(fChannelArray);
  while ((pad = (KBPad *) iterPads.Next())) {
    pad -> LetGo();
  }

  fFreePadIdx = 0;
}

void KBPadPlane::ResetHitMap()
{
  KBPad *pad;
  TIter iterPads(fChannelArray);
  while ((pad = (KBPad *) iterPads.Next())) {
    pad -> ClearHits();
    pad -> LetGo();
  }

  fFreePadIdx = 0;
}

KBTpcHit *KBPadPlane::PullOutNextFreeHit()
{
  if (fFreePadIdx == fChannelArray -> GetEntriesFast() - 1)
    return nullptr;

  auto pad = (KBPad *) fChannelArray -> At(fFreePadIdx);
  auto hit = pad -> PullOutNextFreeHit();
  if (hit == nullptr) {
    fFreePadIdx++;
    return PullOutNextFreeHit();
  }

  return hit;
}

void KBPadPlane::PullOutNeighborHits(KBHitArray *hits, KBHitArray *neighborHits)
{
  auto numHits = hits -> GetEntries();
  for (auto iHit=0; iHit<numHits; ++iHit){
    auto hit = (KBTpcHit *) hits -> GetHit(iHit);
    auto pad = (KBPad *) fChannelArray -> At(hit -> GetPadID());
    auto neighbors = pad -> GetNeighborPadArray();
    for (auto neighbor : *neighbors) {
      neighbor -> PullOutHits(neighborHits);
    }
  }
}

void KBPadPlane::PullOutNeighborHits(Double_t x, Double_t y, Int_t range, KBHitArray *neighborHits)
{
  vector<KBPad *> neighborsUsed;
  vector<KBPad *> neighborsTemp;
  vector<KBPad *> neighborsNew;

  Int_t id = FindPadID(x,y);
  if (id < 0)
    return;

  auto pad = (KBPad *) fChannelArray -> At(id);

  neighborsTemp.push_back(pad);
  pad -> Grab();

  while (range >= 0) {
    neighborsNew.clear();
    GrabNeighborPads(&neighborsTemp, &neighborsNew);

    for (auto neighbor : neighborsTemp)
      neighborsUsed.push_back(neighbor);
    neighborsTemp.clear();

    for (auto neighbor : neighborsNew) {
      neighbor -> PullOutHits(neighborHits);
      neighborsTemp.push_back(neighbor);
    }
    range--;
  }

  for (auto neighbor : neighborsUsed)
    neighbor -> LetGo();

  for (auto neighbor : neighborsNew)
    neighbor -> LetGo();
}

void KBPadPlane::PullOutNeighborHits(vector<KBTpcHit*> *hits, vector<KBTpcHit*> *neighborHits)
{
  for (auto hit : *hits) {
    auto pad = (KBPad *) fChannelArray -> At(hit -> GetPadID());
    auto neighbors = pad -> GetNeighborPadArray();
    for (auto neighbor : *neighbors)
      neighbor -> PullOutHits(neighborHits);
  }
}

void KBPadPlane::PullOutNeighborHits(TVector2 p, Int_t range, vector<KBTpcHit*> *neighborHits)
{
  PullOutNeighborHits(p.X(), p.Y(), range, neighborHits);
}

void KBPadPlane::PullOutNeighborHits(Double_t x, Double_t y, Int_t range, vector<KBTpcHit*> *neighborHits)
{
  vector<KBPad *> neighborsUsed;
  vector<KBPad *> neighborsTemp;
  vector<KBPad *> neighborsNew;

  Int_t id = FindPadID(x,y);
  if (id < 0)
    return;

  auto pad = (KBPad *) fChannelArray -> At(id);

  neighborsTemp.push_back(pad);
  pad -> Grab();

  while (range >= 0) {
    neighborsNew.clear();
    GrabNeighborPads(&neighborsTemp, &neighborsNew);

    for (auto neighbor : neighborsTemp)
      neighborsUsed.push_back(neighbor);
    neighborsTemp.clear();

    for (auto neighbor : neighborsNew) {
      neighbor -> PullOutHits(neighborHits);
      neighborsTemp.push_back(neighbor);
    }
    range--;
  }

  for (auto neighbor : neighborsUsed)
    neighbor -> LetGo();

  for (auto neighbor : neighborsNew)
    neighbor -> LetGo();
}

void KBPadPlane::GrabNeighborPads(vector<KBPad*> *pads, vector<KBPad*> *neighborPads)
{
  for (auto pad : *pads) {
    auto neighbors = pad -> GetNeighborPadArray();
    for (auto neighbor : *neighbors) {
      if (neighbor -> IsGrabed())
        continue;
      neighborPads -> push_back(neighbor);
      neighbor -> Grab();
    }
  }
}

TObjArray *KBPadPlane::GetPadArray() { return fChannelArray; }

bool KBPadPlane::PadPositionChecker(bool checkCorners)
{
  kb_info << "Number of pads: " << fChannelArray -> GetEntries() << endl;

  Int_t countM1 = 0;
  Int_t countBad = 0;

  KBPad *pad;
  TIter iterPads(fChannelArray);
  while ((pad = (KBPad *) iterPads.Next())) {
    if (pad -> GetPadID() == -1) {
      ++countM1;
      continue;
    }
    auto center0 = pad -> GetPosition();
    auto padID0 = pad -> GetPadID();
    auto padID1 = FindPadID(center0.X(),center0.Y());

    if (padID1 != padID0) {
      auto pad1 = (KBPad *) fChannelArray -> At(padID1);
      auto center1 = pad1 -> GetPosition();
      kb_warning << "Bad! Pad:" << padID0 << "(" << center0.X() << "," << center0.Y() << "|" << pad -> GetSection() << "," << pad -> GetRow() << "," << pad -> GetLayer() << ")"
                 << " --> Pad:" << padID1 << "(" << center1.X() << "," << center1.Y() << "|" << pad1-> GetSection() << "," << pad1-> GetRow() << "," << pad1-> GetLayer() << ")" << endl;
      ++countBad;
    }
    if (checkCorners) {
      for (auto corner : *(pad->GetPadCorners())) {
        auto pos = 0.1*center0 + 0.9*corner;
        padID1 = FindPadID(pos.X(),pos.Y());
        if (padID1 != padID0) {
          auto pad1 = (KBPad *) fChannelArray -> At(padID1);
          auto center1 = pad1 -> GetPosition();
          kb_info << "     Corner(" << pos.X() << "," << pos.Y() << ")"
                  << " --> Pad:" << padID1 << "(" << center1.X() << "," << center1.Y() << "|" << pad1-> GetSection() << "," << pad1-> GetRow() << "," << pad1-> GetLayer() << ")" << endl;
          ++countBad;
        }
      }
    }
  }

  kb_info << "=================== Number of 'id = -1' pads: " << countM1 << endl;

  if (countBad > 0) {
    kb_warning << "=================== Bad pad position exist!!!" << endl;
    kb_warning << "=================== Number of bad pads: " << countBad << endl;
    return false;
  }

  kb_info << "=================== All pads are good!" << endl;
  return true;
}

bool KBPadPlane::PadNeighborChecker()
{
  kb_info << "Number of pads: " << fChannelArray -> GetEntries() << endl;

  auto distMax = 0.;
  KBPad *pad0 = 0;
  KBPad *pad1 = 0;

  vector<Int_t> ids0Neighbors;
  vector<Int_t> ids1Neighbors;
  vector<Int_t> ids2Neighbors;
  vector<Int_t> ids3Neighbors;
  vector<Int_t> ids4Neighbors;
  vector<Int_t> ids5Neighbors;
  vector<Int_t> ids6Neighbors;
  vector<Int_t> ids7Neighbors;
  vector<Int_t> ids8Neighbors;
  vector<Int_t> ids9Neighbors;

  KBPad *pad;
  TIter iterPads(fChannelArray);
  while ((pad = (KBPad *) iterPads.Next())) {
    auto pos = pad -> GetPosition();
    auto padID = pad -> GetPadID();
    auto neighbors = pad -> GetNeighborPadArray();
    auto numNeighbors = neighbors -> size();
         if (numNeighbors == 0) ids0Neighbors.push_back(pad -> GetPadID());
    else if (numNeighbors == 1) ids1Neighbors.push_back(pad -> GetPadID());
    else if (numNeighbors == 2) ids2Neighbors.push_back(pad -> GetPadID());
    else if (numNeighbors == 3) ids3Neighbors.push_back(pad -> GetPadID());
    else if (numNeighbors == 4) ids4Neighbors.push_back(pad -> GetPadID());
    else if (numNeighbors == 5) ids5Neighbors.push_back(pad -> GetPadID());
    else if (numNeighbors == 6) ids6Neighbors.push_back(pad -> GetPadID());
    else if (numNeighbors == 7) ids7Neighbors.push_back(pad -> GetPadID());
    else if (numNeighbors == 8) ids8Neighbors.push_back(pad -> GetPadID());
    else                        ids9Neighbors.push_back(pad -> GetPadID());
    for (auto nb : *neighbors) {
      auto padIDnb = nb -> GetPadID();
      auto posnb = nb -> GetPosition();
      auto neighbors2 = nb -> GetNeighborPadArray();
      auto neighborToEachOther = false;
      for (auto nb2 : *neighbors2) {
        if (padID == nb2 -> GetPadID()) {
          neighborToEachOther = true;
          break;
        }
      }
      if (!neighborToEachOther)
        kb_info << "Pad:" << padID << " and Pad:" << padIDnb << " are not neighbor to each other!" << endl;
      auto dx = pos.X() - posnb.X();
      auto dy = pos.Y() - posnb.Y();
      auto dist = sqrt(dx*dx + dy*dy);
      if (dist > distMax) {
        distMax = dist;
        pad0 = pad;
        pad1 = nb;
      }
    }
  }

  kb_info << "=================== Maximum distance between neighbor pads: " << distMax << endl;
  kb_info << "               1 --> Pad:" << pad0->GetPadID() << "(" << pad0->GetPosition().X() << "," << pad0->GetPosition().Y() 
          << "|" << pad0-> GetSection() << "," << pad0 -> GetRow() << "," << pad0 -> GetLayer() << ")" << endl;
  kb_info << "               2 --> Pad:" << pad1->GetPadID() << "(" << pad1->GetPosition().X() << "," << pad1->GetPosition().Y()
          << "|" << pad1-> GetSection() << "," << pad1 -> GetRow() << "," << pad1 -> GetLayer() << ")" << endl;


  kb_info << "No. of pads with 0 neighbors = " << ids0Neighbors.size() << endl;
  for (auto id : ids0Neighbors) {
    cout << id << ", ";
  }
  cout << endl;

  kb_info << "No. of pads with 1 neighbors = " << ids1Neighbors.size() << endl;
  kb_info << "No. of pads with 2 neighbors = " << ids2Neighbors.size() << endl;
  kb_info << "No. of pads with 3 neighbors = " << ids3Neighbors.size() << endl;
  kb_info << "No. of pads with 4 neighbors = " << ids4Neighbors.size() << endl;
  kb_info << "No. of pads with 5 neighbors = " << ids5Neighbors.size() << endl;
  kb_info << "No. of pads with 6 neighbors = " << ids6Neighbors.size() << endl;
  kb_info << "No. of pads with 7 neighbors = " << ids7Neighbors.size() << endl;
  kb_info << "No. of pads with 8 neighbors = " << ids8Neighbors.size() << endl;
  kb_info << "No. of pads with > neighbors = " << ids9Neighbors.size() << endl;

  return true;
}
