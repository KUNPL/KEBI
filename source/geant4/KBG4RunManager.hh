#ifndef KBG4RUNMANAGER_HH
#define KBG4RUNMANAGER_HH

#define g4_info KBLog("KBG4RunManager",__FUNCTION__,0,2)

#include "G4RunManager.hh"
#include "G4VPhysicalVolume.hh"
#include "KBMCDataManager.hh"
#include "KBG4RunMessenger.hh"
#include "KBGear.hh"
#include "TString.h"
#include "globals.hh"

class KBG4RunMessenger;

class KBG4RunManager : public G4RunManager, public KBGear
{
  public:
    KBG4RunManager();
    virtual ~KBG4RunManager();

    virtual void Initialize();
    void Run(G4int argc=0, char **argv=nullptr, const G4String &type="");

    void SetSensitiveDetector(G4VPhysicalVolume *physicalVolume);

    KBParameterContainer *GetSensitiveDetectors();
    KBParameterContainer *GetProcessTable();

  private:
    void SetGeneratorFile(TString value);
    void SetOutputFile(TString value);

    KBMCDataManager *fData;
    KBParameterContainer *fSensitiveDetectors;
    KBParameterContainer *fProcessTable;
};

#endif
