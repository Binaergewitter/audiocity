/**********************************************************************

  Audacity: A Digital Audio Editor

  LadspaEffect.h

  Dominic Mazzoni

**********************************************************************/

class wxSlider;
class wxStaticText;
class wxTextCtrl;
class wxCheckBox;

class NumericTextCtrl;

#include <wx/dynlib.h> // member variable
#include <wx/event.h> // to inherit
#include <wx/weakref.h>

#include "EffectInterface.h"
#include "ModuleInterface.h"
#include "PluginInterface.h"

#include "ladspa.h"
#include "SampleFormat.h"

#define LADSPAEFFECTS_VERSION wxT("1.0.0.0")
/* i18n-hint: abbreviates "Linux Audio Developer's Simple Plugin API"
   (Application programming interface)
 */
#define LADSPAEFFECTS_FAMILY XO("LADSPA")

///////////////////////////////////////////////////////////////////////////////
//
// LadspaEffect
//
///////////////////////////////////////////////////////////////////////////////

class LadspaEffectMeter;

class LadspaEffect final : public wxEvtHandler,
                     public EffectUIClientInterface
{
public:
   LadspaEffect(const wxString & path, int index);
   virtual ~LadspaEffect();

   // ComponentInterface implementation

   PluginPath GetPath() const override;
   ComponentInterfaceSymbol GetSymbol() const override;
   VendorSymbol GetVendor() const override;
   wxString GetVersion() const override;
   TranslatableString GetDescription() const override;

   // EffectDefinitionInterface implementation

   EffectType GetType() override;
   EffectFamilySymbol GetFamily() override;
   bool IsInteractive() override;
   bool IsDefault() override;
   bool SupportsRealtime() override;
   bool SupportsAutomation() override;

   bool GetAutomationParameters(CommandParameters & parms) override;
   bool SetAutomationParameters(CommandParameters & parms) override;

   bool LoadUserPreset(const RegistryPath & name) override;
   bool SaveUserPreset(const RegistryPath & name) override;

   RegistryPaths GetFactoryPresets() const override;
   bool LoadFactoryPreset(int id) override;
   bool LoadFactoryDefaults() override;

   // EffectProcessor implementation

   unsigned GetAudioInCount() override;
   unsigned GetAudioOutCount() override;

   int GetMidiInCount() override;
   int GetMidiOutCount() override;

   void SetSampleRate(double rate) override;
   size_t SetBlockSize(size_t maxBlockSize) override;
   size_t GetBlockSize() const override;

   sampleCount GetLatency() override;
   size_t GetTailSize() override;

   bool ProcessInitialize(EffectSettings &settings,
      sampleCount totalLen, ChannelNames chanMap) override;
   bool ProcessFinalize() override;
   size_t ProcessBlock(EffectSettings &settings,
      const float *const *inBlock, float *const *outBlock, size_t blockLen)
      override;

   bool RealtimeInitialize(EffectSettings &settings) override;
   bool RealtimeAddProcessor(EffectSettings &settings,
      unsigned numChannels, float sampleRate) override;
   bool RealtimeFinalize(EffectSettings &settings) noexcept override;
   bool RealtimeSuspend() override;
   bool RealtimeResume() noexcept override;
   bool RealtimeProcessStart(EffectSettings &settings) override;
   size_t RealtimeProcess(int group,  EffectSettings &settings,
      const float *const *inbuf, float *const *outbuf, size_t numSamples)
      override;
   bool RealtimeProcessEnd(EffectSettings &) noexcept override;

   int ShowClientInterface(
      wxWindow &parent, wxDialog &dialog, bool forceModal) override;

   // EffectUIClientInterface implementation

   bool SetHost(EffectHostInterface *host) override;
   std::unique_ptr<EffectUIValidator> PopulateUI(
      ShuttleGui &S, EffectSettingsAccess &access) override;
   bool IsGraphicalUI() override;
   bool ValidateUI(EffectSettings &) override;
   bool CloseUI() override;

   bool CanExportPresets() override;
   void ExportPresets() override;
   void ImportPresets() override;

   bool HasOptions() override;
   void ShowOptions() override;

   // LadspaEffect implementation

private:
   bool Load();
   void Unload();

   bool LoadParameters(const RegistryPath & group);
   bool SaveParameters(const RegistryPath & group);

   LADSPA_Handle InitInstance(float sampleRate);
   void FreeInstance(LADSPA_Handle handle);

   void OnCheckBox(wxCommandEvent & evt);
   void OnSlider(wxCommandEvent & evt);
   void OnTextCtrl(wxCommandEvent & evt);
   void RefreshControls(bool outputOnly = false);

private:

   wxString mPath;
   int mIndex;
   EffectHostInterface *mHost;

   wxDynamicLibrary mLib;
   const LADSPA_Descriptor *mData;

   wxString pluginName;

   bool mReady;

   LADSPA_Handle mMaster;

   double mSampleRate;
   size_t mBlockSize;

   bool mInteractive;

   unsigned mAudioIns;
   ArrayOf<unsigned long> mInputPorts;

   unsigned mAudioOuts;
   ArrayOf<unsigned long> mOutputPorts;

   int mNumInputControls;
   Floats mInputControls;
   int mNumOutputControls;
   Floats mOutputControls;

   bool mUseLatency;
   int mLatencyPort;
   bool mLatencyDone;

   // Realtime processing
   std::vector<LADSPA_Handle> mSlaves;

   NumericTextCtrl *mDuration;
   wxWeakRef<wxDialog> mDialog;
   wxWindow *mParent;
   ArrayOf<wxSlider*> mSliders;
   ArrayOf<wxTextCtrl*> mFields;
   ArrayOf<wxStaticText*> mLabels;
   ArrayOf<wxCheckBox*> mToggles;
   ArrayOf<LadspaEffectMeter *> mMeters;

   DECLARE_EVENT_TABLE()

   friend class LadspaEffectsModule;
};

///////////////////////////////////////////////////////////////////////////////
//
// LadspaEffectsModule
//
///////////////////////////////////////////////////////////////////////////////

class LadspaEffectsModule final : public ModuleInterface
{
public:
   LadspaEffectsModule();
   virtual ~LadspaEffectsModule();

   // ComponentInterface implementation

   PluginPath GetPath() const override;
   ComponentInterfaceSymbol GetSymbol() const override;
   VendorSymbol GetVendor() const override;
   wxString GetVersion() const override;
   TranslatableString GetDescription() const override;

   // ModuleInterface implementation

   bool Initialize() override;
   void Terminate() override;
   EffectFamilySymbol GetOptionalFamilySymbol() override;

   const FileExtensions &GetFileExtensions() override;
   FilePath InstallPath() override;

   bool AutoRegisterPlugins(PluginManagerInterface & pm) override;
   PluginPaths FindPluginPaths(PluginManagerInterface & pm) override;
   unsigned DiscoverPluginsAtPath(
      const PluginPath & path, TranslatableString &errMsg,
      const RegistrationCallback &callback)
         override;

   bool IsPluginValid(const PluginPath & path, bool bFast) override;

   std::unique_ptr<ComponentInterface>
      CreateInstance(const PluginPath & path) override;

   // LadspaEffectModule implementation

   FilePaths GetSearchPaths();
};

