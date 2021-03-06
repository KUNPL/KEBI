#ifndef KBPARAMETERCONTAINER
#define KBPARAMETERCONTAINER

#include "KBGlobal.hh"
#include "TObjArray.h"
#include "TNamed.h"
#include "TParameter.h"
#include "KBVector3.hh"

/**
 * List of parameters <[parameter name], [parameter values]>
 * parameter type features Bool_t, Int_t, Double_t, TString, KBVector3::Axis.
 *
 * Structure of parameter file should be list of : [name] [type_initial] [value]
 * Each elemets are divided by space.
 * Comments are used by #.
 *
 * @param name Name of parameter file with no space
 * @param type_initial
 *   - b for Bool_t
 *   - i for Int_t
 *   - d for Double_t
 *   - s for TString
 *   - a for KBVector3::Axis
 * @param value Value of parameter. TString value do not accept space.
 *
 * ex)\n
 * \#example parameter file\n
 * worldSize    d  1000   # [mm]\n
 * nTbs         i  512\n
 * specialFile  s  /path/to/specialFile.dat  \#special\n 
 *
 * ============================================================================
 *
 * With fDebugMode true, KBParameterContainer will not terminate at attempt of
 * getting non-existing paramter, but print message and create empty parameter.
 *
*/

class KBParameterContainer : public TObjArray
{
  public:
    KBParameterContainer(Bool_t debug = false);
    KBParameterContainer(const char *parName, Bool_t debug = false); ///< Constructor with input parameter file name
    virtual ~KBParameterContainer() {}

    void SaveAs(const char *filename, Option_t *option = "") const;

    void SetDebugMode(Bool_t val = true);

    virtual void Print(Option_t *option ="") const;

    /**
     * Add parameter by given fileName.
     * If fileName does not include path, file will be searched in path/to/KEBI/input.
     *
     * fileName will also be registered as parameter. 
     * If parNameForFile is not set, parameter name will be set as 
     * INPUT_PARAMETER_FILE[fNumInputFiles]
    */
    virtual Int_t AddFile(TString fileName, TString parNameForFile = "");
    virtual Int_t AddPar(KBParameterContainer *parc, TString parNameForFile = "");
    Int_t GetNumInputFiles(); ///< Get number of input parameter files

    Bool_t SetPar(TString name, Bool_t val, Bool_t overwrite = false);       ///< Set Bool_t   type parameter with given name
    Bool_t SetPar(TString name, Int_t val, Bool_t overwrite = false);        ///< Set Int_t    type parameter with given name
    Bool_t SetPar(TString name, Double_t val, Bool_t overwrite = false);     ///< Set Double_t type parameter with given name
    Bool_t SetPar(TString name, TString val, Bool_t overwrite = false);      ///< Set TString  type parameter with given name
    Bool_t SetPar(TString name, const char* val, Bool_t overwrite = false);  ///< Set TString  type parameter with given name
    Bool_t SetParColor(TString name, TString valColor, Bool_t overwrite = false);

    Bool_t   GetParBool(TString name);   ///< Get Bool_t   type parameter by given name. Terminate if (parameter does-not-exist && fDebugMode == false).
    Int_t    GetParInt(TString name);    ///< Get Int_t    type parameter by given name. Terminate if (parameter does-not-exist && fDebugMode == false).
    Double_t GetParDouble(TString name); ///< Get Double_t type parameter by given name. Terminate if (parameter does-not-exist && fDebugMode == false).
    TString  GetParString(TString name); ///< Get TString  type parameter by given name. Terminate if (parameter does-not-exist && fDebugMode == false).
    KBVector3::Axis  GetParAxis(TString name);   ///< Get KBVector3::Axis  type parameter by given name. Terminate if (parameter does-not-exist && fDebugMode == false).

    Int_t GetParWidth(TString name) { return GetParInt(name); }
    Int_t GetParColor(TString name) { return GetParInt(name); }
    Double_t GetParSize(TString name) { return GetParDouble(name); }

    Bool_t CheckPar(TString name);

    void ReplaceEnvironmentVariable(TString &val);

  private:
    Bool_t fDebugMode = false;
    Int_t fNumInputFiles = 0;

  ClassDef(KBParameterContainer, 1)
};

#endif
